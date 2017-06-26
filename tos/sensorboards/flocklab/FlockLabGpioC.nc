// $Id: FlockLabGpioC.nc 2375 2013-04-19 08:35:27Z walserc $
/**
 * @author Roman Lim <lim@tik.ee.ethz.ch>
 * @author Philipp Sommer <philipp.sommer@csiro.au> (Opal port)
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

configuration FlockLabGpioC {
	provides {
		interface GpioInterrupt as SIG1INT;
		interface GpioInterrupt as SIG2INT;
		interface GeneralIO as SIG1;
		interface GeneralIO as SIG2;
		interface GeneralIO as FLOCKLAB_INT1;
		interface GeneralIO as FLOCKLAB_INT2;
	}
}
implementation {
	
	components MainC;
	components FlockLabGpioP;
	MainC.SoftwareInit -> FlockLabGpioP;
	
#if defined(PLATFORM_TINYNODE184) || defined(PLATFORM_TINYNODE)
	components HplMsp430GeneralIOC;
	components HplMsp430InterruptC;
	
	components new Msp430GpioC() as SIG1Gpio;
	SIG1Gpio.HplGeneralIO->HplMsp430GeneralIOC.Port12;
	SIG1 = SIG1Gpio;
	
	components new Msp430GpioC() as SIG2Gpio;
	SIG2Gpio.HplGeneralIO->HplMsp430GeneralIOC.Port13;
	SIG2 = SIG2Gpio;

	components new Msp430GpioC() as INT1Gpio;
	INT1Gpio.HplGeneralIO->HplMsp430GeneralIOC.Port56; // 40 = normal gpio, 56 = 32khz clock output
	FLOCKLAB_INT1 = INT1Gpio;

	components new Msp430GpioC() as INT2Gpio;
	INT2Gpio.HplGeneralIO->HplMsp430GeneralIOC.Port41;
	FLOCKLAB_INT2 = INT2Gpio;

	components new Msp430InterruptC() as SIG1INTInt;
	SIG1INTInt.HplInterrupt->HplMsp430InterruptC.Port12;
	SIG1INT = SIG1INTInt;
	
	components new Msp430InterruptC() as SIG2INTInt;
	SIG2INTInt.HplInterrupt->HplMsp430InterruptC.Port13;
	SIG2INT = SIG2INTInt;
	
	FlockLabGpioP.SIG1 -> SIG1Gpio;
	FlockLabGpioP.SIG2 -> SIG2Gpio;
	FlockLabGpioP.FLOCKLAB_INT1 -> INT1Gpio;
	FlockLabGpioP.FLOCKLAB_INT2 -> INT2Gpio;
#endif
#if defined(PLATFORM_TELOSB) || defined(PLATFORM_TELOSA)
	components HplMsp430GeneralIOC;
	components HplMsp430InterruptC;
	
	components new Msp430GpioC() as SIG1Gpio;
	SIG1Gpio.HplGeneralIO->HplMsp430GeneralIOC.Port27;
	SIG1 = SIG1Gpio;
	
	components new Msp430GpioC() as SIG2Gpio;
	SIG2Gpio.HplGeneralIO->HplMsp430GeneralIOC.Port23;
	SIG2 = SIG2Gpio;

	components new Msp430GpioC() as INT1Gpio;
	INT1Gpio.HplGeneralIO->HplMsp430GeneralIOC.Port60;
	FLOCKLAB_INT1 = INT1Gpio;

	components new Msp430GpioC() as INT2Gpio;
	INT2Gpio.HplGeneralIO->HplMsp430GeneralIOC.Port61;
	FLOCKLAB_INT2 = INT2Gpio;

	components new Msp430InterruptC() as SIG1INTInt;
	SIG1INTInt.HplInterrupt->HplMsp430InterruptC.Port27;
	SIG1INT = SIG1INTInt;
	
	components new Msp430InterruptC() as SIG2INTInt;
	SIG2INTInt.HplInterrupt->HplMsp430InterruptC.Port23;
	SIG2INT = SIG2INTInt;
	
	FlockLabGpioP.SIG1 -> SIG1Gpio;
	FlockLabGpioP.SIG2 -> SIG2Gpio;
	FlockLabGpioP.FLOCKLAB_INT1 -> INT1Gpio;
	FlockLabGpioP.FLOCKLAB_INT2 -> INT2Gpio;
#endif
#if defined(PLATFORM_IRIS) || defined(PLATFORM_MICAZ) || defined(PLATFORM_MICA2)
	components HplAtm128GeneralIOC;
	components HplAtm128InterruptC;
	
	SIG1 = HplAtm128GeneralIOC.PortE4;
	
	SIG2 = HplAtm128GeneralIOC.PortE5;

	FLOCKLAB_INT1 = HplAtm128GeneralIOC.PortC1;
	
	FLOCKLAB_INT2 = HplAtm128GeneralIOC.PortC2;

	components new Atm128GpioInterruptC() as SIG1INTInt;
	SIG1INTInt.Atm128Interrupt->HplAtm128InterruptC.Int4;
	SIG1INT = SIG1INTInt;
	
	components new Atm128GpioInterruptC() as SIG2INTInt;
	SIG2INTInt.Atm128Interrupt->HplAtm128InterruptC.Int5;
	SIG2INT = SIG2INTInt;
	
	FlockLabGpioP.SIG1 -> HplAtm128GeneralIOC.PortE4;
	FlockLabGpioP.SIG2 -> HplAtm128GeneralIOC.PortE5;
	FlockLabGpioP.FLOCKLAB_INT1 -> HplAtm128GeneralIOC.PortC1;
	FlockLabGpioP.FLOCKLAB_INT2 -> HplAtm128GeneralIOC.PortC2;
#endif
#if defined(PLATFORM_OPAL)
	components HplSam3uGeneralIOC;
	
	SIG1 = HplSam3uGeneralIOC.PioA1;
	
	SIG2 = HplSam3uGeneralIOC.PioA30;

	FLOCKLAB_INT1 = HplSam3uGeneralIOC.PioA28;
	
	FLOCKLAB_INT2 = HplSam3uGeneralIOC.PioA29;

	SIG1INT = HplSam3uGeneralIOC.InterruptPioA1;
	
	SIG2INT = HplSam3uGeneralIOC.InterruptPioA30;
	
	FlockLabGpioP.SIG1 -> HplSam3uGeneralIOC.PioA1;
	FlockLabGpioP.SIG2 -> HplSam3uGeneralIOC.PioA30;
	FlockLabGpioP.FLOCKLAB_INT1 -> HplSam3uGeneralIOC.PioA28;
	FlockLabGpioP.FLOCKLAB_INT2 -> HplSam3uGeneralIOC.PioA29;
#endif		
}
