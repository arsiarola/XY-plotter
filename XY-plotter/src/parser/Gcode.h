#ifndef GCODE_H_
#define GCODE_H_

#include <string>

#define getGcodeId(letter, number) ((letter << 8) | (number))
// have mask just in case we would store to a bigger variable than 8 bits
#define getLetterFromID(id) ((id >> 8) & (0xFF))
#define getNumberFromID(id) ((id)      & (0xFF))

class Gcode {
public:
    enum Letter : char { M = 'M', G = 'G' };
    enum Number : uint8_t { _1 = 1, _2 = 2, _4 = 4, _5 = 5, _10 = 10, _11 = 11, _28 = 28 };
    enum Id : uint16_t {
        G1 = getGcodeId(Letter::G, Number::_1),
        G28 = getGcodeId(Letter::G, Number::_28),
        M1 = getGcodeId(Letter::M, Number::_1),
        M2 = getGcodeId(Letter::M, Number::_2),
        M4 = getGcodeId(Letter::M, Number::_4),
        M5 = getGcodeId(Letter::M, Number::_5),
        M10 = getGcodeId(Letter::M, Number::_10),
        M11 = getGcodeId(Letter::M, Number::_11)
    };

    typedef struct Data {
        Id id;
        union data {
            struct m1  { uint8_t penPos; } m1;
            struct m2  { uint8_t savePenUp; uint8_t savePenDown; } m2;
            struct m4  { uint8_t laserPower; } m4;
            struct m5  {
                bool dirX ;
                bool dirY ;
                uint32_t height ;
                uint32_t width ;
                uint8_t speed ;
            } m5;
            struct g1  { // g1 and g28 have same data
                float moveX ;
                float moveY ;
                bool relative;
            } g1;
        } data;
    } Data;


    Gcode(Letter letter_, Number number_, bool (*functionPtr_)(const char *str) = nullptr)  :
        letter(letter_),
        number(number_),
        id((Id)getGcodeId(letter, number)),
        functionPtr(functionPtr_)
    { }
    static const char* toFormat(Id id_);
    const char* toFormat() { return toFormat(id); }
    static const char* toString(Letter let, Number num);
    const char* toString() { return toString(letter, number); }
    static const char* toString(Id id_);
    Id getId() { return id; };
    bool callback(const char *str);
    virtual ~Gcode() { };

private:
    Letter letter;
    Number number;
    Id id;
    bool (*functionPtr)(const char *str);
};

#endif /* GCODE_H_ */
