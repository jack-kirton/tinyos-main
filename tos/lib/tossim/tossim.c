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
  std::replace(realName.begin(), realName.end(), '.', '$');

  if (sim_mote_get_variable_info(mote, realName.c_str(), &ptr, &len) == 0) {
    data = new uint8_t[len + 1];
    data[len] = 0;
  }
  else {
    data = NULL;
    ptr = NULL;
  }
}

Variable::~Variable() {
  delete[] data;
}

void Variable::update() {
  if (data != nullptr && ptr != nullptr) {
    // Copy the data from the variable (ptr) into our local data (data).
    memcpy(data, ptr, len);
  }
}

variable_string_t Variable::getData() {
  variable_string_t str;
  if (data != nullptr && ptr != nullptr) {
    str.ptr = data;
    str.type = format.c_str();
    str.len = len;
    str.isArray = isArray;

    update();
  }
  else {
    str.ptr = const_cast<char*>("<no such variable>");
    str.type = "<no such variable>";
    str.len = strlen("<no such variable>");
    str.isArray = 0;
  }
  return str;
}

Mote::Mote(nesc_app_t* n) : app(n) {
}

Mote::~Mote(){
  for (auto iter = varTable.begin(), end = varTable.end(); iter != end; ++iter)
  {
    delete iter->second;
  }
}

unsigned long Mote::id() noexcept {
  return nodeID;
}

long long int Mote::euid() noexcept {
  return sim_mote_euid(nodeID);
}

void Mote::setEuid(long long int val) noexcept {
  sim_mote_set_euid(nodeID, val);
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

Variable* Mote::getVariable(const char* name_cstr) {
  Variable* var;

  std::string name(name_cstr);

  auto find = varTable.find(name);

  if (find == varTable.end()) {
    const char* typeStr = "";
    bool isArray = false;
    // Could hash this for greater efficiency,
    // but that would either require transformation
    // in Tossim class or a more complex typemap.
    if (app != NULL) {
      for (unsigned int i = 0; i < app->numVariables; i++) {
        if (name == app->variableNames[i]) {
          typeStr = app->variableTypes[i];
          isArray = app->variableArray[i];
          break;
        }
      }
    }

    var = new Variable(name, typeStr, isArray, nodeID);

    varTable.emplace(std::move(name), var);
  }
  else {
    var = find->second;
  }

  return var;
}

void Mote::addNoiseTraceReading(int val) {
  sim_noise_trace_add(nodeID, (char)val);
}

void Mote::createNoiseModel() {
  sim_noise_create_model(nodeID);
}

int Mote::generateNoise(int when) {
  return static_cast<int>(sim_noise_generate(nodeID, when));
}

Tossim::Tossim(nesc_app_t* n)
  : app(n)
  , motes(TOSSIM_MAX_NODES, NULL)
{
  init();
}

void Tossim::free_motes()
{
  for (auto iter = motes.begin(), end = motes.end(); iter != end; ++iter)
  {
    delete *iter;
  }
}

Tossim::~Tossim() {
  free_motes();
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
    // TODO: log an error, asked for an invalid node
    return NULL;
  }
  else {
    if (motes[nodeID] == NULL) {
      motes[nodeID] = new Mote(app);
      if (nodeID == TOSSIM_MAX_NODES) {
        motes[nodeID]->setID(0xffff);
      }
      else {
        motes[nodeID]->setID(nodeID);
      }
    }
    return motes[nodeID];
  }
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

void Tossim::randomSeed(int seed) {
  return sim_random_seed(seed);
}

typedef struct handle_python_event_data {
  handle_python_event_data(Tossim* tossim, std::function<bool(double)> provided_event_callback)
    : self(tossim)
    , event_callback(std::move(provided_event_callback))
  {
  }

  Tossim* const self;

  const std::function<bool(double)> event_callback;
} handle_python_event_data_t;

static void handle_python_event(void* void_event)
{
  sim_event_t* event = static_cast<sim_event_t*>(void_event);

  handle_python_event_data_t* data = static_cast<handle_python_event_data_t*>(event->data);

  data->event_callback(data->self->timeInSeconds());

  delete data;

  // Set to NULL to avoid a double free from TOSSIM trying to clean up the sim event
  event->data = NULL;

  python_event_called = false;
}

void Tossim::register_event_callback(std::function<bool(double)> callback, double event_time) {
  sim_register_event(
    static_cast<sim_time_t>(event_time * ticksPerSecond()),
    &handle_python_event,
    new handle_python_event_data_t(this, std::move(callback))
  );
}

bool Tossim::runNextEvent() {
  return sim_run_next_event();
}

unsigned int Tossim::runAllEvents(std::function<bool(double)> continue_events, std::function<void (unsigned int)> callback) {
  unsigned int event_count = 0;
  while (continue_events(timeInSeconds()))
  {
    if (!runNextEvent())
    {
      break;
    }

    // Only call the callback if there is something for it to process
    if (sim_log_test_flag()) {
      callback(event_count);
    }

    event_count += 1;
  }

  return event_count;
}

unsigned int Tossim::runAllEventsWithMaxTime(double end_time, std::function<bool()> continue_events, std::function<void (unsigned int)> callback) {
  const long long int end_time_ticks = (long long int)ceil(end_time * ticksPerSecond());
  unsigned int event_count = 0;
  bool process_callback = true;

  // We can skip calling the continue_events predicate if no log info was outputted, or no python callback occurred
  while (sim_time() < end_time_ticks && ((!process_callback && !python_event_called) || continue_events()))
  {
    // Reset the python event called flag as we have no handled it
    python_event_called = false;

    if (!runNextEvent())
    {
      break;
    }

    process_callback = sim_log_test_flag();

    // Only call the callback if there is something for it to process
    if (process_callback) {
      callback(event_count);
    }

    event_count += 1;
  }

  return event_count;
}

MAC* Tossim::mac() {
  return new MAC();
}

Radio* Tossim::radio() {
  return new Radio();
}

Packet* Tossim::newPacket() {
  return new Packet();
}
