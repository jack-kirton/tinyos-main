/*
 * Copyright (c) 2005 Stanford University. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the copyright holder nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Implementation of TOSSIM C++ classes. Generally just directly
 * call their C analogues.
 *
 * @author Philip Levis
 * @date   Nov 22 2005
 */

// $Id: tossim.c,v 1.7 2010-06-29 22:07:51 scipio Exp $

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include <algorithm>
#include <stdexcept>

#include <tossim.h>
#include <sim_tossim.h>
#include <sim_mote.h>
#include <sim_event_queue.h>
#include <sim_noise.h>

#include <mac.c>
#include <radio.c>
#include <packet.c>

uint16_t TOS_NODE_ID = 1;

static bool python_event_called = false;

Variable::Variable(const std::string& name, const char* formatStr, bool array, int which)
  : realName(name)
  , format(formatStr)

  , mote(which)
  , isArray(array)
{
  // Names can come in two formats:
  // nongeneric: "ActiveMessageAddressC$addr"
  // generic: "/*AlarmCounterMilliP.Atm128AlarmAsyncC.Atm128AlarmAsyncP*/Atm128AlarmAsyncP$0$set"
  // We need to change the "." to "$" in parts after the /*...*/ part

  size_t last_slash_pos = realName.find_last_of('/');
  if (last_slash_pos == std::string::npos)
  {
    last_slash_pos = 0;
  }

  std::replace(realName.begin() + last_slash_pos, realName.end(), '.', '$');

  if (sim_mote_get_variable_info(mote, realName.c_str(), &ptr, &len) == 0) {
    data.reset(new uint8_t[len + 1]);
    data[len] = 0;
  }
  else {
    data = nullptr;
    ptr = nullptr;
  }
}

Variable::~Variable() {
}

void Variable::update() {
  if (data != nullptr && ptr != nullptr) {
    // Copy the data from the variable (ptr) into our local data (data).
    memcpy(data.get(), ptr, len);
  }
}

variable_string_t Variable::getData() {
  variable_string_t str;
  if (data != nullptr && ptr != nullptr) {
    str.ptr = data.get();
    str.type = format.c_str();
    str.len = len;
    str.isArray = isArray;

    update();
  }
  else {
    str.ptr = const_cast<char*>("<no such variable>");
    str.type = "<no such variable>";
    str.len = strlen("<no such variable>");
    str.isArray = false;
  }
  return str;
}

Mote::Mote(const NescApp* n) : app(n) {
}

Mote::~Mote() {
}

unsigned long Mote::id() const noexcept {
  return nodeID;
}

long long int Mote::euid() const noexcept {
  return sim_mote_euid(nodeID);
}

void Mote::setEuid(long long int val) noexcept {
  sim_mote_set_euid(nodeID, val);
}

long long int Mote::tag() const noexcept {
  return sim_mote_tag(nodeID);
}

void Mote::setTag(long long int val) noexcept {
  sim_mote_set_tag(nodeID, val);
}

long long int Mote::bootTime() const noexcept {
  return sim_mote_start_time(nodeID);
}

void Mote::bootAtTime(long long int time) noexcept {
  sim_mote_set_start_time(nodeID, time);
  sim_mote_enqueue_boot_event(nodeID);
}

bool Mote::isOn() noexcept {
  return sim_mote_is_on(nodeID);
}

void Mote::turnOff() noexcept {
  sim_mote_turn_off(nodeID);
}

void Mote::turnOn() noexcept {
  sim_mote_turn_on(nodeID);
}

void Mote::setID(unsigned long val) noexcept {
  nodeID = val;
}

std::shared_ptr<Variable> Mote::getVariable(const char* name_cstr) {
  std::shared_ptr<Variable> var;

  std::string name(name_cstr);

  auto find = varTable.find(name);

  if (find == varTable.end()) {
    const char* typeStr = "";
    bool isArray = false;
    // Could hash this for greater efficiency,
    // but that would either require transformation
    // in Tossim class or a more complex typemap.
    if (app != nullptr) {
      for (unsigned int i = 0; i < app->numVariables; i++) {
        if (name == app->variableNames[i]) {
          typeStr = app->variableTypes[i].c_str();
          isArray = app->variableArray[i];
          break;
        }
      }
    }

    var = std::make_shared<Variable>(name, typeStr, isArray, nodeID);

    varTable.emplace(std::move(name), var);
  }
  else {
    var = find->second;
  }

  return var;
}

void Mote::reserveNoiseTraces(size_t num_traces) {
  sim_noise_reserve(nodeID, num_traces);
}

void Mote::addNoiseTraceReading(int val) {
  sim_noise_trace_add(nodeID, static_cast<char>(val));
}

void Mote::createNoiseModel() {
  sim_noise_create_model(nodeID);
}

int Mote::generateNoise(int when) {
  return static_cast<int>(sim_noise_generate(nodeID, when));
}


Tossim::Tossim(NescApp n)
  : app(std::move(n))
  , motes(TOSSIM_MAX_NODES)
  , duration_started(false)
{
  init();
}

Tossim::~Tossim() {
  sim_end();
}

void Tossim::init(){
  sim_init();
}

long long int Tossim::time() const noexcept {
  return sim_time();
}

double Tossim::timeInSeconds() const noexcept {
  return time() / static_cast<double>(ticksPerSecond());
}

long long int Tossim::ticksPerSecond() noexcept {
  return sim_ticks_per_sec();
}

const char* Tossim::timeStr() noexcept {
  sim_print_now(timeBuf, 128);
  return timeBuf;
}

void Tossim::setTime(long long int val) noexcept {
  sim_set_time(val);
}

Mote* Tossim::currentNode() noexcept {
  return getNode(sim_node());
}

Mote* Tossim::getNode(unsigned long nodeID) noexcept {
  if (nodeID >= TOSSIM_MAX_NODES) {
    throw std::runtime_error("Asked for an invalid node id. You may need to increase the maximum number of nodes.");
  }

  if (motes[nodeID] == nullptr) {
    motes[nodeID].reset(new Mote(&app));

    if (nodeID == TOSSIM_MAX_NODES) {
      nodeID = 0xFFFF;
    }

    motes[nodeID]->setID(nodeID);
  }
  return motes[nodeID].get();
}

void Tossim::setCurrentNode(unsigned long nodeID) noexcept {
  sim_set_node(nodeID);
}

void Tossim::addChannel(const char* channel, FILE* file) {
  sim_add_channel(channel, file);
}

bool Tossim::removeChannel(const char* channel, FILE* file) {
  return sim_remove_channel(channel, file);
}

typedef struct handle_tossim_callback_data {
  handle_tossim_callback_data(std::function<void(const char*, size_t)> provided_callback)
    : callback(std::move(provided_callback))
  {
  }

  const std::function<void(const char*, size_t)> callback;
} handle_tossim_callback_data_t;

static void handle_tossim_callback(void* void_data, const char* line, size_t line_length)
{
  handle_tossim_callback_data_t* data = static_cast<handle_tossim_callback_data_t*>(void_data);

  data->callback(line, line_length);
}

void Tossim::addCallback(const char* channel, std::function<void(const char*, size_t)> callback) {
  sim_add_callback(channel, &handle_tossim_callback, new handle_tossim_callback_data_t(std::move(callback)));
}

void Tossim::randomSeed(int seed) {
  return sim_random_seed(seed);
}

typedef struct handle_python_event_data {
  handle_python_event_data(Tossim* tossim, std::function<void(double)> provided_event_callback)
    : self(tossim)
    , event_callback(std::move(provided_event_callback))
  {
  }

  Tossim* const self;

  const std::function<void(double)> event_callback;
} handle_python_event_data_t;

static void handle_python_event(void* void_event)
{
  sim_event_t* event = static_cast<sim_event_t*>(void_event);

  std::unique_ptr<handle_python_event_data_t> data(static_cast<handle_python_event_data_t*>(event->data));

  // Set to nullptr to avoid a double free from TOSSIM trying to clean up the sim event
  event->data = nullptr;

  python_event_called = true;

  data->event_callback(data->self->timeInSeconds());
}

void Tossim::register_event_callback(std::function<void(double)> callback, double event_time) {
  sim_register_event(
    static_cast<sim_time_t>(event_time * ticksPerSecond()),
    &handle_python_event,
    new handle_python_event_data_t(this, std::move(callback))
  );
}

bool Tossim::runNextEvent() {
  return sim_run_next_event();
}

void Tossim::triggerRunDurationStart() {
  if (!duration_started)
  {
    duration_started = true;
    duration_started_at = sim_time();
  }
}

long long int Tossim::runAllEventsWithTriggeredMaxTime(
  double duration,
  double duration_upper_bound,
  std::function<bool()> continue_events)
{
  const long long int duration_ticks = static_cast<long long int>(ceil(duration * ticksPerSecond()));
  const long long int duration_upper_bound_ticks = static_cast<long long int>(ceil(duration_upper_bound * ticksPerSecond()));
  long long int event_count = 0;
  bool process_callback = true;

  // We can skip calling the continue_events predicate if no log info was outputted, or no python callback occurred
  while (
      (!duration_started || sim_time() < (duration_started_at + duration_ticks)) &&
      (sim_time() < duration_upper_bound_ticks) &&
      ((!process_callback && !python_event_called) || continue_events())
    )
  {
    // Reset the python event called flag as we have no handled it
    python_event_called = false;

    if (!runNextEvent())
    {
      // Use negative to signal no more events
      event_count = -event_count;
      break;
    }

    process_callback = sim_log_test_flag();

    event_count += 1;
  }

  return event_count;
}

long long int Tossim::runAllEventsWithTriggeredMaxTimeAndCallback(
    double duration,
    double duration_upper_bound,
    std::function<bool()> continue_events,
    std::function<void(long long int)> callback)
{
  const long long int duration_ticks = static_cast<long long int>(ceil(duration * ticksPerSecond()));
  const long long int duration_upper_bound_ticks = static_cast<long long int>(ceil(duration_upper_bound * ticksPerSecond()));
  long long int event_count = 0;
  bool process_callback = true;

  // We can skip calling the continue_events predicate if no log info was outputted, or no python callback occurred
  while (
      (!duration_started || sim_time() < (duration_started_at + duration_ticks)) &&
      (sim_time() < duration_upper_bound_ticks) &&
      ((!process_callback && !python_event_called) || continue_events())
    )
  {
    // Reset the python event called flag as we have no handled it
    python_event_called = false;

    if (!runNextEvent())
    {
      // Use negative to signal no more events
      event_count = -event_count;
      break;
    }

    process_callback = sim_log_test_flag();

    if (process_callback) {
      callback(event_count);
    }

    event_count += 1;
  }

  return event_count;
}

std::shared_ptr<MAC> Tossim::mac() {
  return std::make_shared<MAC>();
}

std::shared_ptr<Radio> Tossim::radio() {
  return std::make_shared<Radio>();
}

std::shared_ptr<Packet> Tossim::newPacket() {
  return std::make_shared<Packet>();
}
