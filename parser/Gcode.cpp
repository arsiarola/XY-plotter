#include "Gcode.h"

Gcode::Gcode(Id id_, const char *gcode_, const char *format_) :
        gcode(gcode_),
        format(format_),
        id(id_)
{ }
