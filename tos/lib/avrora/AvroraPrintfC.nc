#include "AvroraPrint.h"

#ifndef AVRORA_MAX_BUFFER_SIZE
#define AVRORA_MAX_BUFFER_SIZE 256
#endif

module AvroraPrintfC
{
	provides interface AvroraPrintf;
}

implementation
{
	void avrora_printf(const char* fmt, ...)  __attribute__((noinline)) @C() @spontaneous()
	{
		char buf[AVRORA_MAX_BUFFER_SIZE];

		va_list ap;
		va_start(ap, fmt);
		vsnprintf(buf, sizeof(buf), fmt, ap);

		call AvroraPrintf.print_str(buf);

		va_end(ap);
	}

	command void AvroraPrintf.print_char(char c)
	{
		printChar(c);
	}

	command void AvroraPrintf.print_int8(int8_t i)
	{
		printInt8(i);
	}

	command void AvroraPrintf.print_int16(int16_t i)
	{
		printInt16(i);
	}

	command void AvroraPrintf.print_int32(int32_t i)
	{
		printInt32(i);
	}

	command void AvroraPrintf.print_str(const char * const s)
	{
		printStr(s);
	}

	command void AvroraPrintf.print_hex_buf(const uint8_t *b, uint16_t len)
	{
		printHexBuf(b, len);
	}

	command void AvroraPrintf.print_hex8(uint8_t i)
	{
		printHex8(i);
	}

	command void AvroraPrintf.print_hex16(uint16_t i)
	{
		printHex16(i);
	}

	command void AvroraPrintf.print_hex32(uint32_t i)
	{
		printHex32(i);
	}
}
