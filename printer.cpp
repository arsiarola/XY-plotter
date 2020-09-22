#include "printer.h"

Syslog mDraw_uart = Syslog();
namespace Printer {
    int uart_print(const char *str) {
        return mDraw_uart.writeString(str);
    }
}
