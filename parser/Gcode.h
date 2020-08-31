#ifndef GCODE_H_
#define GCODE_H_

#include <regex>
#include <string>

class Gcode {
public:
    Gcode(const char *gcode_, const std::string regex_);
    const char* getRegex() { return regex.c_str(); };
    const char* getGcode() { return gcode.c_str(); };
    virtual ~Gcode() { };

private:
    std::string gcode;
    std::string regex;
};
#endif /* GCODE_H_ */
