
#include "printf.h"

configuration UartPrintfC {
}
implementation {
  components UartPrintfP;

  components MainC;
  MainC.SoftwareInit -> UartPrintfP;

  components PlatformSerialC;
  UartPrintfP.UartControl -> PlatformSerialC;
  UartPrintfP.UartStream -> PlatformSerialC;

  components PutcharC;
  PutcharC.Putchar -> UartPrintfP;

  components new PrintfQueueC(uint8_t, PRINTF_BUFFER_SIZE) as QueueC;
  UartPrintfP.Queue -> QueueC;
}
