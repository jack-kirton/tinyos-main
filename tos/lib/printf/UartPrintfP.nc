/*
 * Copyright (c) 2006 Washington University in St. Louis.
 * All rights reserved.
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
 * - Neither the name of the copyright holders nor the names of
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
 
 /*
 * Copyright (c) 2007 Stanford University.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * - Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * - Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in the
 *   documentation and/or other materials provided with the
 *   distribution.
 * - Neither the name of the Stanford University nor the names of
 *   its contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL STANFORD
 * UNIVERSITY OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * This is the PrintfP component.  It provides the printf service for printing
 * data over the serial interface using the standard c-style printf command.  
 * Data printed using printf are buffered and only sent over the serial line after
 * the buffer is half full or an explicit call to printfflush() is made.  This 
 * buffer has a maximum size of 250 bytes at present.  This component is wired
 * to a shadowed MainC component so that printf statements can be made anywhere 
 * throughout your code, so long as you include the "printf.h" header file in 
 * every file you wish to use it.  Take a look at the printf tutorial (lesson 15)
 * for more details.
 *
 * The printf service is currently only available for msp430 based motes 
 * (i.e. telos, eyes) and atmega128x based motes (i.e. mica2, micaz, iris).  On the
 * atmega platforms, avr-libc version 1.4 or above must be used.
 */
 
/**
 * @author Kevin Klues <klueska@cs.stanford.edu>
 * @date September 18, 2007
 */

#include "printf.h"

module UartPrintfP {
  provides {
    interface Init;
    interface Putchar;
    interface StdControl;
  }
  uses {
    interface PrintfQueue<uint8_t> as Queue;
    interface UartStream;
    interface StdControl as UartControl;
  }
}
implementation {
  
  enum {
    S_STARTED,
    S_FLUSHING,
  };

  uint8_t buffer[PRINTF_BUFFER_SIZE];
  uint16_t buffer_size;

  uint8_t state = S_STARTED;

  command error_t Init.init()
  {
    atomic {
      state = S_STARTED;
    }
    buffer_size = 0;
    return call StdControl.start();
  }

  command error_t StdControl.start()
  {
    return call UartControl.start();
  }

  command error_t StdControl.stop()
  {
    return call UartControl.stop();
  }
  
  async event void UartStream.receivedByte(uint8_t data) {}
    
  async event void UartStream.receiveDone(uint8_t* rbuf, uint16_t len, error_t error) {}

  task void retrySend() {
    if(call UartStream.send(buffer, buffer_size) != SUCCESS)
      post retrySend();
  }
  
  task void sendNext() {
    uint16_t i;

    atomic {
      buffer_size = (call Queue.size() < sizeof(buffer)) ? call Queue.size() : sizeof(buffer);
      for(i=0; i<buffer_size; i++)
      {
        buffer[i] = call Queue.dequeue();
      }
    }

    if(call UartStream.send(buffer, buffer_size) != SUCCESS)
      post retrySend();  
  }
  
  int printfflush() @C() @spontaneous() {
    atomic {
      if(state == S_FLUSHING)
        return SUCCESS;
      if(call Queue.empty())
        return FAIL;
      state = S_FLUSHING;
      post sendNext();
    }
    return SUCCESS;
  }

  async event void UartStream.sendDone(uint8_t* sbuf, uint16_t len, error_t error)
  {
    if(error == SUCCESS) {
      atomic {
        if(call Queue.size() > 0) {
          post sendNext();
        }
        else {
          state = S_STARTED;
        }
      } 
    }
    else {
      post retrySend();
    }
  } 

#undef putchar
  command int Putchar.putchar (int c)
  {
    atomic {
      if (state == S_STARTED && call Queue.size() >= (call Queue.maxSize() / 2)) {
        state = S_FLUSHING;
        post sendNext();
      }

      if (call Queue.enqueue(c) == SUCCESS)
      {
        // Flush if newline
        if (c == '\n' && state == S_STARTED)
        {
          state = S_FLUSHING;
          post sendNext();
        }

        // On success the character written is returned
        return c;
      }
      else
      {
        // On failure EOF (-1) is returned
        return -1;
      }
    }
  }
}
