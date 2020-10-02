#ifndef GCODE_H_
#define GCODE_H_

#include <string>

class Gcode {
public:
    enum Id { M1, M2, M4, M5, M10, M11, G1, G28 };
    Gcode(Id id_, const char *gcode_, const char *format_);
    const char* getFormat() { return format; };
    const char* getGcode() { return gcode; };
    Id getId() { return id; };
    virtual ~Gcode() { };

private:
    const Id id;
    const char *gcode;
    const char *format;
};
#endif /* GCODE_H_ */
