#ifndef MOTOR_H_
#define MOTOR_H_

#include "DigitalIoPin.h"

class Motor {
public:

    Motor(
        DigitalIoPin&& motor,
        DigitalIoPin&& direction,
        DigitalIoPin&& limOrigin,
        DigitalIoPin&& limMax,
        bool originDirection
    );
    virtual ~Motor() { };
    DigitalIoPin motor;
    DigitalIoPin direction;
    DigitalIoPin limOrigin;
    DigitalIoPin limMax;
    bool originDirection;


};
#endif /* MOTOR_H_ */
