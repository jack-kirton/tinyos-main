// $Id: TargetLedsP.nc 2375 2013-04-19 08:35:27Z walserc $
/**
 * The implementation of the standard LED mote abstraction.
 *
 * @author Roman Lim
 */
module TargetLedsP {
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
#if defined(PLATFORM_IRIS) || defined(PLATFORM_MICAZ) || defined(PLATFORM_MICA2) || defined(PLATFORM_TELOSB) || defined(PLATFORM_OPAL)
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
