#include "Gcode.h"
bool Gcode::callback (const char *str) {
    if (functionPtr != nullptr) return functionPtr(str);
    return false;
}
