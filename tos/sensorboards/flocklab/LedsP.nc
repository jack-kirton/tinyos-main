// $Id: LedsP.nc 2375 2013-04-19 08:35:27Z walserc $
/**
 * @author Roman Lim <lim@tik.ee.ethz.ch>
 *
 * @section LICENSE
 *
 * Copyright (c) 2013 ETH Zurich.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, LOSS OF USE, DATA,
 * OR PROFITS) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/*                                                                      tab:4
 * "Copyright (c) 2000-2005 The Regents of the University  of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 */

/**
 * The implementation of the standard LED mote abstraction.
 *
 * @author Joe Polastre
 * @author Philip Levis
 * @date   March 21, 2005
 */
module LedsP {
  provides {
    interface Init;
    interface Leds;
  }
  uses {
    interface GeneralIO as Led0;
    interface GeneralIO as Led1;
    interface GeneralIO as Led2;
  }
}
implementation {
#if defined(PLATFORM_IRIS) || defined(PLATFORM_MICAZ) || defined(PLATFORM_MICA2)
#define	SET clr
#define	CLR set
#define GETLED(X) !call Led##X.get()
#else
#define	SET set
#define	CLR clr
#define GETLED(X) call Led##X.get()
#endif
  
  command error_t Init.init() {
    atomic {
      call Led0.makeOutput();
      call Led1.makeOutput();
      call Led2.makeOutput();
      call Led0.CLR();
      call Led1.CLR();
      call Led2.CLR();
    }
    return SUCCESS;
  }

  async command void Leds.led0On() {
    call Led0.SET();
  }

  async command void Leds.led0Off() {
    call Led0.CLR();
  }

  async command void Leds.led0Toggle() {
    call Led0.toggle();
  }

  async command void Leds.led1On() {
    call Led1.SET();
  }

  async command void Leds.led1Off() {
    call Led1.CLR();
  }

  async command void Leds.led1Toggle() {
    call Led1.toggle();
  }

  async command void Leds.led2On() {
    call Led2.SET();
  }

  async command void Leds.led2Off() {
    call Led2.CLR();
  }

  async command void Leds.led2Toggle() {
    call Led2.toggle();
  }

  async command uint8_t Leds.get() {
    uint8_t rval;
    atomic {
      rval = 0;
      if (GETLED(0)) {
        rval |= LEDS_LED0;
      }
      if (GETLED(1)) {
        rval |= LEDS_LED1;
      }
      if (GETLED(2)) {
        rval |= LEDS_LED2;
      }
    return rval;
  }
}

  async command void Leds.set(uint8_t val) {
    atomic {
      if (val & LEDS_LED0) {
        call Leds.led0On();
      }
      else {
        call Leds.led0Off();
      }
      if (val & LEDS_LED1) {
        call Leds.led1On();
      }
      else {
        call Leds.led1Off();
      }
      if (val & LEDS_LED2) {
        call Leds.led2On();
      }
      else {
        call Leds.led2Off();
      }
    }
  }
}
