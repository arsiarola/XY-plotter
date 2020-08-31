#include "Gcode.h"

Gcode::Gcode(const char *gcode_, const std::string regex_) :
        gcode(gcode_),
        regex(regex_)
{
}
