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

typedef struct variable_string {
  const char* type;
  void* ptr;
  int len;
  bool isArray;
} variable_string_t;

typedef struct nesc_app {
  unsigned int numVariables;
  const char** variableNames;
  const char** variableTypes;
  bool* variableArray;
} nesc_app_t;

class Variable {
 public:
  Variable(const char* name, const char* format, bool array, int mote);
  ~Variable();
  variable_string_t getData();
  
 private:
  char* realName;
  char* format;
  void* ptr;
  uint8_t* data;
  size_t len;
  int mote;
  bool isArray;
};

class Mote {
 public:
  Mote(nesc_app_t* app);
  ~Mote();

  unsigned long id() noexcept;
  
  long long int euid() noexcept;
  void setEuid(long long int id) noexcept;

  long long int bootTime() const noexcept;
  void bootAtTime(long long int time) noexcept;

  bool isOn() noexcept;
  void turnOff() noexcept;
  void turnOn() noexcept;
  void setID(unsigned long id) noexcept;  

  void addNoiseTraceReading(int val);
  void createNoiseModel();
  int generateNoise(int when);
  
  Variable* getVariable(const char* name);
  
 private:
  unsigned long nodeID;
  nesc_app_t* app;
  struct hashtable* varTable;
};

class Tossim {
 public:
  Tossim(nesc_app_t* app);
  ~Tossim();
  
  void init();
  
  long long int time() const noexcept;
  double timeInSeconds() const noexcept;
  static long long int ticksPerSecond() noexcept;
  const char* timeStr() noexcept;
  void setTime(long long int time) noexcept;
  
  Mote* currentNode() noexcept;
  Mote* getNode(unsigned long nodeID) noexcept;
  void setCurrentNode(unsigned long nodeID) noexcept;

  void addChannel(const char* channel, FILE* file);
  bool removeChannel(const char* channel, FILE* file);
  void randomSeed(int seed);

  void register_event_callback(std::function<bool(double)> callback, double time);
  
  bool runNextEvent();
  unsigned int runAllEvents(std::function<bool(double)> continue_events, std::function<void (unsigned int)> callback);
  unsigned int runAllEventsWithMaxTime(double end_time, std::function<bool()> continue_events, std::function<void (unsigned int)> callback);

  MAC* mac();
  Radio* radio();
  Packet* newPacket();

private:
  void free_motes();

 private:
  nesc_app_t* app;
  Mote** motes;
  char timeBuf[128];
};

#endif // TOSSIM_H_INCLUDED
