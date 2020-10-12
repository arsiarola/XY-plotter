#ifndef PRINTER_H_
#define PRINTER_H_
#include "ITM_write.h"
#include "LpcUart.h"
#define ARG_BUFFER_SIZE 128

#define RELEASE 0

extern LpcUart *mDraw_uart;

// we cant just use mDraw_uart.write as a callback function since mDraw_uart hasnt been compiled yet
// so lets make function that just uses mDraw_uart.write. We can use uart_print now as callback.
int uart_print(const char *str);
int arg_print(int (*callback) (const char *), const char *format, ...);

#if RELEASE == 1
	#define mDraw_print(...)
	#define ITM_print(...)
#else
	#define mDraw_print(...) arg_print(uart_print, __VA_ARGS__)
    #define ITM_print(...) arg_print(ITM_write, __VA_ARGS__)
#endif /* RELEASE */


#endif /* PRINTER_H_ */

