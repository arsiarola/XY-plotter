#include "Gcode.h"
#include <stdio.h>

bool Gcode::callback (const char *str) {
    return (functionPtr != nullptr ? functionPtr(str) : false);
}

const char* Gcode::toFormat(Id id_) {
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
            return "M10" ;

        case M11:
            return "M11";

        default:
            return "";
    }
}

const char* Gcode::toString(Letter let, Number num) {
    std::string str;
    str.reserve(4);
    std::snprintf((char*)str.data(), 4, "%c%u", let, num);
    return str.c_str();
}

const char* Gcode::toString(Id id_) {
    return toString((Letter)GET_LETTER_FROM_ID(id_), (Number)GET_NUMBER_FROM_ID(id_));
}
