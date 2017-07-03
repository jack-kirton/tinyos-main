// $Id: TargetPlatformLedsC.nc 2375 2013-04-19 08:35:27Z walserc $
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
/* "Copyright (c) 2000-2005 The Regents of the University of California.  
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement
 * is hereby granted, provided that the above copyright notice, the following
 * two paragraphs and the author appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY
 * OF CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 */

/**
 * @author Joe Polastre
 */

configuration TargetPlatformLedsC {
  provides interface GeneralIO as Led0;
  provides interface GeneralIO as Led1;
  provides interface GeneralIO as Led2;
  uses interface Init;
}
implementation
{
	
#if defined(PLATFORM_TINYNODE184) || defined(PLATFORM_TINYNODE)
  components 
  HplMsp430GeneralIOC as GeneralIOC, 
    new Msp430GpioC() as Led0Impl,
    new Msp430GpioC() as Led1Impl,
    new Msp430GpioC() as Led2Impl;

  Led0 = Led0Impl;
  Led0Impl -> GeneralIOC.Port15;

  Led1 = Led1Impl;
  Led1Impl -> GeneralIOC.Port23;

  Led2 = Led2Impl;
  Led2Impl -> GeneralIOC.Port24;
#endif
#if defined(PLATFORM_TELOSB)
  components 
  HplMsp430GeneralIOC as GeneralIOC, 
    new Msp430GpioC() as Led0Impl,
    new Msp430GpioC() as Led1Impl,
    new Msp430GpioC() as Led2Impl;

  Led0 = Led0Impl;
  Led0Impl -> GeneralIOC.Port54;

  Led1 = Led1Impl;
  Led1Impl -> GeneralIOC.Port55;

  Led2 = Led2Impl;
  Led2Impl -> GeneralIOC.Port56;
#endif
#if defined(PLATFORM_IRIS) || defined(PLATFORM_MICAZ) || defined(PLATFORM_MICA2) 
  components 
  HplAtm128GeneralIOC;

  Led0 = HplAtm128GeneralIOC.PortA2;

  Led1 = HplAtm128GeneralIOC.PortA1;

  Led2 = HplAtm128GeneralIOC.PortA0;

#endif
#if defined(PLATFORM_OPAL)
  components 
  HplSam3uGeneralIOC;

  Led0 = HplSam3uGeneralIOC.PioC6;

  Led1 = HplSam3uGeneralIOC.PioC7;

  Led2 = HplSam3uGeneralIOC.PioC8;

#endif
  
  components MainC;
  Init = MainC.SoftwareInit;
}
