#include "Gcode.h"
void Gcode::callback (const char *str) {
    if (functionPtr != nullptr) functionPtr(str);
}
