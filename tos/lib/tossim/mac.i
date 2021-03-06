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
 * SWIG interface specification for a media access control algorithm
 * and physical data rate in TOSSIM. This file defines the MAC object
 * which is exported to
 * the python scripting interface. This particular MAC is CSMA.
 * Changing the MAC abstraction requires changing or replacing this
 * file and rerunning generate-swig.bash in lib/tossim. Note that
 * this abstraction does not represent an actual MAC implementation,
 * instead merely a set of configuration constants that a CSMA MAC
 * implementation might use. The default values model the standard
 * TinyOS CC2420 stack. Most times (rxtxDelay, etc.) are in terms
 * of symbols. E.g., an rxTxDelay of 32 means 32 symbol times. This
 * value can be translated into real time with the symbolsPerSec()
 * call.
 *
 * Note that changing this file only changes the Python interface:
 * you must also change the underlying TOSSIM code so Python
 * has the proper functions to call. Look at mac.h, mac.c, and
 * sim_mac.c.
 *
 * @author Philip Levis
 * @date   Dec 10 2005
 */

%module TOSSIMMAC

%{
#include <mac.h>
%}

class MAC {
 public:
  MAC();
  ~MAC();

  int initHigh();
  int initLow();
  int high() const;
  int low() const;
  int symbolsPerSec() const;
  int bitsPerSymbol() const;
  int preambleLength() const; // in symbols
  int exponentBase() const;
  int maxIterations() const;
  int minFreeSamples() const;
  int rxtxDelay() const;
  int ackTime() const; // in symbols
  
  void setInitHigh(int val);
  void setInitLow(int val);
  void setHigh(int val);
  void setLow(int val);
  void setSymbolsPerSec(int val);
  void setBitsBerSymbol(int val);
  void setPreambleLength(int val);
  void setExponentBase(int val);
  void setMaxIterations(int val);
  void setMinFreeSamples(int val);
  void setRxtxDelay(int val);
  void setAckTime(int val);
};
