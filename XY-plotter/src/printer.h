#ifndef PRINTER_H_
#define PRINTER_H_
#include "ITM_write.h"
#include "LpcUart.h"
#define ARG_BUFFER_SIZE 128

#define RELEASE 0

namespace Printer {
    extern LpcUart* debug_uart;
    int uart_print(const char *str);
    int arg_print(int (*callback) (const char *), const char *format, ...);
}


#if RELEASE == 1
	#define UART_print(...)
	#define ITM_print(...)
#else
	#define UART_print(...) Printer::arg_print(Printer::uart_print, __VA_ARGS__)
    #define ITM_print(...)  Printer::arg_print(ITM_write, __VA_ARGS__)
#endif /* RELEASE */


#endif /* PRINTER_H_ */

