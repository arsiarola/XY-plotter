#ifndef PRINTER_H_
#define PRINTER_H_
#include "arg_printer.h"
#include "ITM_write.h"
#include "syslog.h"

namespace Printer {
	int uart_print(const char *str);
}
extern Syslog mDraw_uart;
#define mDraw_print(...) arg_print(Printer::uart_print, __VA_ARGS__)

#define ITM_print(...) arg_print(ITM_write, __VA_ARGS__)


#endif /* PRINTER_H_ */
