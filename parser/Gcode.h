#ifndef GCODE_H_
#define GCODE_H_

#include <string>

class Gcode {
public:
    enum Id : uint8_t { M1, M2, M4, M5, M10, M11, G1, G28 };
    Gcode(Id id_, const char *gcode_, const char *format_, void (*functionPtr_)(const char *str) = nullptr)  :
        gcode(gcode_),
        format(format_),
        id(id_),
        functionPtr(functionPtr_)
    { }
    const char* getFormat() { return format; };
    const char* getGcode() { return gcode; };
    Id getId() { return id; };
    void callback(const char *str);
    virtual ~Gcode() { };

private:
    const Id id;
    const char *gcode;
    const char *format;
    void (*functionPtr)(const char *str);
};

struct GcodeData {
	Gcode::Id id;
	union Data {
		struct M1  { uint8_t penPos; }M1;
		struct M2  { uint8_t savePenUp; uint8_t savePenDown; }M2;
		struct M4  { uint8_t laserPower; }M4;
		struct M5  {
			bool dirX ;
			bool dirY ;
			uint32_t height ;
			uint32_t width ;
			uint8_t speed ;
		}M5;
		struct G1  { // G1 and G28 have same data
			float moveX ;
			float moveY ;
			bool absolute;
		}G1;
	} Data;
};
#endif /* GCODE_H_ */
