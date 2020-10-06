#ifndef PARSER_H
#define PARSER_H
#include "FreeRTOS.h"
#include "queue.h"

void parseCode(const char *s, QueueHandle_t &queue);
void trimTrailing(char * str);

void m1ExtractData(const char *str);
void m2ExtractData (const char *str);
void m4ExtractData (const char *str);
void m5ExtractData(const char *str);
void m10ExtractData (const char *str);
void m11ExtractData (const char *str);
void g1ExtractData (const char *str);
void g28ExtractData (const char *str);


#endif /* PARSER_H */
