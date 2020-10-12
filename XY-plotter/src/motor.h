#ifndef MOTOR_H_
#define MOTOR_H_

#include "DigitalIoPin.h"

class Motor {
public:

    Motor(
        DigitalIoPin&& stepper,
        DigitalIoPin&& direction,
        DigitalIoPin&& limOrigin,
        DigitalIoPin&& limMax,
        bool originDirection
    );
    ~Motor();

    bool readStepper()     { return stepper.read(); }
    bool readDirection()   { return direction.read(); }
    bool readMinLimit()    { return minLimit.read(); }
    bool readMaxLimit()    { return maxLimit.read(); }
    bool readOriginLimit() { return readMinLimit(); } // "Alias" for readMinLimit

    void writeStepper(bool step)  { return stepper.write(step); }
    void writeDirection(bool dir) { return direction.write(dir); }

    bool getOriginDirection() { return originDirection; }
private:
    DigitalIoPin stepper;
    DigitalIoPin direction;
    DigitalIoPin minLimit;
    DigitalIoPin maxLimit;
    bool originDirection;


};
#endif /* MOTOR_H_ */
