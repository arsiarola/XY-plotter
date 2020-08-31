#ifndef GCODE_H_
#define GCODE_H_

#include <regex>
#include <string>

class Gcode {
public:
    Gcode(const char *gcode_, const std::regex regex_);
    std::regex getRegex() { return regex; }
    const char* getGcode() { return gcode.c_str(); }

private:
    std::string gcode;
    std::regex regex;
};
#endif /* GCODE_H_ */
