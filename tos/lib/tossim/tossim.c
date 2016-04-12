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

Variable::Variable(const char* name, const char* formatStr, bool array, int which) {
  format = strdup(formatStr);
  isArray = array;
  mote = which;
  
  size_t sLen = strlen(name);
  realName = strndup(name, sLen);

  std::replace(realName, realName + sLen, '.', '$');

  //printf("Creating %s realName: %s format: '%s' %s\n", name, realName, formatStr, array? "[]":"");

  if (sim_mote_get_variable_info(mote, realName, &ptr, &len) == 0) {
    data = (uint8_t*)malloc(len + 1);
    data[len] = 0;
  }
  else {
    //fprintf(stderr, "Could not find variable %s\n", realName);
    data = NULL;
    ptr = NULL;
  }
  //printf("Allocated variable %s\n", realName);
}

Variable::~Variable() {
  //fprintf(stderr, "Freeing variable %s\n", realName);
  free(data);
  free(realName);
  free(format);
}

/* This is the sdbm algorithm, taken from
   http://www.cs.yorku.ca/~oz/hash.html -pal */
static unsigned int tossim_hash(const void* key) {
  const char* str = (const char*)key;
  unsigned int hashVal = 0;
  int c;
  
  while ((c = *str++))
    hashVal = c + (hashVal << 6) + (hashVal << 16) - hashVal;
  
  return hashVal;
}

static int tossim_hash_eq(const void* key1, const void* key2) {
  return strcmp((const char*)key1, (const char*)key2) == 0;
}


variable_string_t Variable::getData() {
  variable_string_t str;
  if (data != NULL && ptr != NULL) {
    str.ptr = data;
    str.type = format;
    str.len = len;
    str.isArray = isArray;
    memcpy(data, ptr, len);
    //printf("Getting '%s' %s %d %s\n", format, isArray? "[]":"", len, realName);
  }
  else {
    str.ptr = (char*)"<no such variable>";
    str.type = "<no such variable>";
    str.len = strlen("<no such variable>");
    str.isArray = 0;
  }
  return str;
}

Mote::Mote(nesc_app_t* n) : app(n) {
  varTable = create_hashtable(128, tossim_hash, tossim_hash_eq);
}

static void delete_Variable(void* voidptr)
{
  Variable* var = static_cast<Variable*>(voidptr);
  delete var;
}

Mote::~Mote(){
  hashtable_destroy(varTable, &delete_Variable);
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

Variable* Mote::getVariable(const char* name) {
  const char* typeStr = "";
  bool isArray = false;
  Variable* var;

  var = (Variable*)hashtable_search(varTable, name);
  if (var == NULL) {
    // Could hash this for greater efficiency,
    // but that would either require transformation
    // in Tossim class or a more complex typemap.
    if (app != NULL) {
      for (unsigned int i = 0; i < app->numVariables; i++) {
        if (strcmp(name, app->variableNames[i]) == 0) {
          typeStr = app->variableTypes[i];
          isArray = app->variableArray[i];
          break;
        }
      }
    }
    //printf("Getting variable %s of type %s %s\n", name, typeStr, isArray? "[]" : "");
    var = new Variable(name, typeStr, isArray, nodeID);
    hashtable_insert(varTable, strdup(name), var);
  }

  return var;
}

void Mote::addNoiseTraceReading(int val) {
  sim_noise_trace_add(id(), (char)val);
}

void Mote::createNoiseModel() {
  sim_noise_create_model(id());
}

int Mote::generateNoise(int when) {
  return static_cast<int>(sim_noise_generate(id(), when));
}

Tossim::Tossim(nesc_app_t* n) : app(n) {
  motes = NULL;
  init();
}

void Tossim::free_motes()
{
  if (motes != NULL)
  {
    for (size_t i = 0; i != (TOSSIM_MAX_NODES + 1); ++i)
    {
      if (motes[i] != NULL)
        delete motes[i];
    }
    free(motes);
    motes = NULL;
  }
}

Tossim::~Tossim() {
  free_motes();
  sim_end();
}

void Tossim::init() {
  sim_init();
  free_motes();
  motes = (Mote**)calloc(TOSSIM_MAX_NODES + 1, sizeof(Mote*));
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
  sim_print_now(timeBuf, 256);
  return timeBuf;
}

void Tossim::setTime(long long int val) noexcept {
  sim_set_time(val);
}

Mote* Tossim::currentNode() noexcept {
  return getNode(sim_node());
}

Mote* Tossim::getNode(unsigned long nodeID) noexcept {
  if (nodeID > TOSSIM_MAX_NODES) {
    nodeID = TOSSIM_MAX_NODES;
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
  int event_count = 0;
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
  int event_count = 0;
  bool process_callback = true;
  while (timeInSeconds() < end_time && (!process_callback || continue_events()))
  {
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


JavaRandom::JavaRandom(long long int seed) noexcept : _has_next_gaussian(false) {
  setSeed(seed);
}

void JavaRandom::setSeed(long long int seed) noexcept {
  _seed = (seed ^ 0x5DEECE66DLL) & ((1LL << 48) - 1);
  _has_next_gaussian = false;
}
long long int JavaRandom::getSeed() const noexcept {
  return _seed;
}

long long int JavaRandom::next(int bits) noexcept {
  if (bits < 1) {
    bits = 1;
  }
  if (bits > 32) {
    bits = 32;
  }

  _seed = (_seed * 0x5deece66dLL + 0xbLL) & ((1LL << 48) - 1);

  long long int retval = _seed >> (48 - bits);

  return retval;
}

double JavaRandom::nextDouble() noexcept {
  return ((next(26) << 27) + next(27)) / (double)(1LL << 53);
}
double JavaRandom::nextGaussian() noexcept {
  if (!_has_next_gaussian) {
    double v1, v2, s;
    do {
      v1 = 2 * nextDouble() - 1;
      v2 = 2 * nextDouble() - 1;
      s = v1 * v1 + v2 * v2;
    } while (s >= 1 || s == 0);
    double multiplier = sqrt(-2 * log(s) / s);
    _next_gaussian = v2 * multiplier;
    _has_next_gaussian = true;
    return v1 * multiplier;
  }
  else {
    _has_next_gaussian = false;
    return _next_gaussian;
  }
}
