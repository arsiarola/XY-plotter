#include <iostream>
#include <regex>
#include <string>
#include "Gcode.h"


void print_results(const std::string &str, std::regex &regex ) {
}

Gcode M2 = Gcode("M2",
        ".*(M2)"
        "\\s+([U|D]?-?\\d+)"
        "\\s+([U|D]?-?\\d+)"
        );
Gcode M10 = Gcode("M10",
        ".*(M10)"
        ".*"
        );
Gcode M11 = Gcode("M11",
        ".*(M11)"
        "\\s+(-?\\d+)"
        "\\s+(-?\\d+)"
        "\\s+(-?\\d+)"
        "\\s+(-?\\d+)"
        );

#define GCODE_SIZE 3
static Gcode *gcodes[
    &M2,
    &M10,
    &M11
];


int main() {
    std::string s = "M11 -1 2 3 4";
    auto first_token = s.substr(0, s.find(' '));
    uint8_t index = -1;
    for (uint8_t i=0; i<GCODE_SIZE; ++i) {
        if (first_token == gcodes[i].getGcode()) {
            index = i;
            break;
        }
    }
    /* switch(Gcodes[index].getGcode()) { */
    /*     case M2*/
    /* } */
    /* std::smatch result; */
    /* bool found = std::regex_match(str, result, regex); */
    /* if (found) { */
    /*     std::smatch::iterator it = result.begin(); */
    /*     for (std::advance(it, 1); */
    /*          it != result.end(); */
    /*          advance(it, 1)) */
    /*         std::cout << *it << std::endl; */
    /* } else */
    /*     std::cout << "No match!" << std::endl; */
}
