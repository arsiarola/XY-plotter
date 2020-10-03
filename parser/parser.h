#ifndef PARSER_H
#define PARSER_H
#include "Gcode.h"

// possibly move this struct to plotter.cpp
struct GcodeData{
    Gcode::Id id;
    uint32_t chunk1; // main chunk
    uint32_t chunk2;
    uint16_t chunk3;
};

void parseCode(const char *s);
void trimTrailing(char * str);

#endif /* PARSER_H */
