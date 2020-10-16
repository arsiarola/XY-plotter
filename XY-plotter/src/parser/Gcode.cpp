#include "Gcode.h"
#include <stdio.h>

bool Gcode::callback (const char *str) {
    if (functionPtr != nullptr) return functionPtr(str);
    else                        return false;
}

const char* Gcode::toFormat(const Id& id_) {
    switch (id_) {
        case G1:
            return
                "G1 "
                "X" "%f " //X85.14
                "Y" "%f " //Y117.29
                "A" "%d"; //A0

        case G28:
            return "G28 ";

        case M1:
            return "M1 " "%u";

        case M2:
            return
                "M2 "
                "U" "%u "
                "D" "%u";
        case M4:
            return "M4 " "%u";

        case M5:
            return
                "M5 "
                "A" "%d " //A0
                "B" "%d " //B0
                "H" "%u " //H310
                "W" "%u " //W380
                "S" "%u"; //S80

        case M10:
            return
                "M10 "
                "XY "
                "%u %u "       // area, width, height
                "0.00 0.00 " // undocumented
                "A%d B%d "     // X and Y direction
                "H0 "          // undocumented
                "S%u "         // speed
                "U%u D%u\r\n"      // penup / down
            	"OK\r\n"
                ;

        case M11:
            return "M11 %d %d %d %d\r\nOK\r\n";

        default:
            return "";
    }
}

Gcode::array Gcode::toString(const Letter& letter, const Number& number) {
    std::array<char, 4> str;
    std::snprintf(str.data(), 4, "%c%u", letter, number);
    return str;
}

Gcode::array Gcode::toString(const Id& id) {
    return toString((Letter)GET_LETTER_FROM_ID(id), (Number)GET_NUMBER_FROM_ID(id));
}
