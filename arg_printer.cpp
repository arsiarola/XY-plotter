#include "arg_printer.h"

#include <stdio.h>
#include <stdarg.h>     /* va_list, va_start, va_arg, va_end */


int arg_print(int (*callback) (const char *), const char *format, ...) {
    char buffer [BUFFER_SIZE];
    va_list argptr;
    va_start(argptr, format);
    vsnprintf (buffer, BUFFER_SIZE, format, argptr);
    va_end(argptr);
    return callback(buffer);
}

