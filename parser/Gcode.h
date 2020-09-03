#ifndef GCODE_H_
#define GCODE_H_

#include <string>

class Gcode {
public:
    Gcode(const char *gcode_, const char *format_);
    const char* getFormat() { return format; };
    const char* getGcode() { return gcode; };
    virtual ~Gcode() { };

private:
    const char *gcode;
    const char *format;
};
#endif /* GCODE_H_ */
