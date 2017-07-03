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
 * Declaration of C++ objects representing TOSSIM abstractions.
 * Used to generate Python objects.
 *
 * @author Philip Levis
 * @date   Nov 22 2005
 */

// $Id: tossim.h,v 1.6 2010-06-29 22:07:51 scipio Exp $

#ifndef TOSSIM_H_INCLUDED
#define TOSSIM_H_INCLUDED

//#include <stdint.h>
#include <memory.h>
#include <tos.h>
#include <mac.h>
#include <radio.h>
#include <packet.h>
#include <hashtable.h>

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

typedef struct variable_string {
  const char* type;
  void* ptr;
  int len;
  bool isArray;
} variable_string_t;

class NescApp {
public:
    std::unordered_map<std::string, std::tuple<bool, std::string>> variables;
};

class Variable {
 public:
  Variable(const std::string& name, const std::string& format, bool array, int mote);
  ~Variable();

  variable_string_t getData();
  bool setData(const void* new_data, size_t length);

  template <typename T>
  inline bool setData(T new_data)
  {
    return setData(&new_data, sizeof(new_data));
  }

  const std::string& getFormat() const { return format; }
  void* getPtr() const { return ptr; }
  size_t getLen() const { return len; }

 private:
  void update();
  
 private:
  std::string realName;
  std::string format;
  void* ptr;
  std::unique_ptr<uint8_t[]> data;
  size_t len;
  int mote;
  bool isArray;
};

class Mote {
 public:
  Mote(const NescApp* app);
  ~Mote();

  unsigned long id() const noexcept;
  
  long long int euid() const noexcept;
  void setEuid(long long int id) noexcept;

  long long int tag() const noexcept;
  void setTag(long long int tag) noexcept;

  long long int bootTime() const noexcept;
  void bootAtTime(long long int time) noexcept;

  bool isOn() noexcept;
  void turnOff() noexcept;
  void turnOn() noexcept;
  void setID(unsigned long id) noexcept;  

  void reserveNoiseTraces(size_t num_traces);
  void addNoiseTraceReading(int val);
  void createNoiseModel();
  int generateNoise(int when);
  
  std::shared_ptr<Variable> getVariable(const char* name_cstr);

 private:
  unsigned long nodeID;
  const NescApp* app;
  std::unordered_map<std::string, std::shared_ptr<Variable>> varTable;
};

class Tossim {
 public:
  Tossim(NescApp app, bool should_free=true);
  ~Tossim();
  
  void init();
  
  long long int time() const noexcept;
  double timeInSeconds() const noexcept;
  static long long int ticksPerSecond() noexcept;
  std::string timeStr() const;
  void setTime(long long int time) noexcept;
  
  Mote* currentNode();
  Mote* getNode(unsigned long nodeID);
  void setCurrentNode(unsigned long nodeID) noexcept;

  void addChannel(const char* channel, FILE* file);
  bool removeChannel(const char* channel, FILE* file);
  void addCallback(const char* channel, std::function<void(const char*, size_t)> callback);

  void randomSeed(int seed);

  void register_event_callback(std::function<void(double)> callback, double time);
  
  bool runNextEvent();

  void triggerRunDurationStart();

  long long int runAllEventsWithTriggeredMaxTime(
    double duration,
    double duration_upper_bound,
    std::function<bool()> continue_events);
  long long int runAllEventsWithTriggeredMaxTimeAndCallback(
    double duration,
    double duration_upper_bound,
    std::function<bool()> continue_events,
    std::function<void(long long int)> callback);

  MAC& mac();
  Radio& radio();
  std::shared_ptr<Packet> newPacket();

 private:
  const NescApp app;

  std::vector<std::unique_ptr<Mote>> motes;

  MAC _mac;
  Radio _radio;

  long long int duration_started_at;
  bool duration_started;

  bool should_free;
};

#endif // TOSSIM_H_INCLUDED
