#ifndef ARG_PRINTER_H_
#define PRINTER_H_

#define BUFFER_SIZE 128
int arg_print(int (*callback) (const char *), const char *format, ...);

#endif /* ARG_PRINTER_H_ */
