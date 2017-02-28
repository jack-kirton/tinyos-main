#ifndef AVRORA_PRINTF_H
#define AVRORA_PRINTF_H

#if defined (_H_msp430hardware_h) || defined (_H_atmega128hardware_H)
#	include <stdio.h>
#else
#	error "Require platform with stdio.h"
#endif

void avrora_printf(const char* fmt, ...) __attribute__ ((__format__(printf, 1, 2)));

#define simdbg(name, fmtstr, ...) do { avrora_printf("%s:D:%" PRIu16 ":%lu:" fmtstr, name, TOS_NODE_ID, call LocalTime.get(), ##__VA_ARGS__); } while (FALSE)
#define simdbg_clear(name, fmtstr, ...) do { avrora_printf(fmtstr, ##__VA_ARGS__); } while (FALSE)
#define simdbgerror(name, fmtstr, ...) do { avrora_printf("%s:E:%" PRIu16 ":%lu:" fmtstr, name, TOS_NODE_ID, call LocalTime.get(), ##__VA_ARGS__); } while (FALSE)
#define simdbgerror_clear(name, fmtstr, ...) do { avrora_printf(fmtstr, ##__VA_ARGS__); } while (FALSE)

#endif // AVRORA_PRINTF_H
