#include <stdio.h>
#include <iostream>
#include <string>
#include "Gcode.h"

Gcode M2 = Gcode("M2",
                 "M2 "
                  "%c%d "
                  "%c%d"
                );
Gcode M10 = Gcode("M10",
                  ".*(M10)"
                  ".*"
                 );
Gcode M11 = Gcode("M11",
                  "M11"
                  "%d "
                  "%d "
                  "%d "
                  "%d"
                 );

#define GCODE_SIZE 3
static Gcode *gcodes[GCODE_SIZE] = {
    &M2,
    &M10,
    &M11
};


int main() {
    std::string s = "M2 U150 D90";
    /* std::string s = "M11 -1 2 3 4"; */
    auto first_token = s.substr(0, s.find(' '));
    uint8_t index = -1;
    bool found = false;
    for (uint8_t i=0; i<GCODE_SIZE; ++i) {
        if (first_token == gcodes[i]->getGcode()) {
            index = i;
            break;
        }
    }

    if (gcodes[index]->getGcode() == M2.getGcode()) {
        char pos1;
        char pos2;
        int num1;
        int num2;
        if (std::sscanf(
                    s.c_str(),
                    M2.getRegex(),
                    &pos1,
                    &num1,
                    &pos2,
                    &num2) >= 3) {
            std::cout << pos1 << std::endl;
            std::cout << num1 << std::endl;
            std::cout << pos2 << std::endl;
            std::cout << num2 << std::endl;
        }
    }

    if (gcodes[index]->getGcode() == M11.getGcode()) {
        int values[4] = {0};
        if (std::sscanf(
                    s.c_str(),
                    M11.getRegex(),
                    &values[0],
                    &values[1],
                    &values[2],
                    &values[3]) >= 3) {
            for (int i = 0; i < 4; ++i) {
                std::cout << values[i] << std::endl;
            }
        }
    }
}
