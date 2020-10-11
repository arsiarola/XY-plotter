#ifndef MOTOR_H_
#define MOTOR_H_

#include "DigitalIoPin.h"

class Motor {
public:

    Motor(
        DigitalIoPin&& motor,
        DigitalIoPin&& limOrigin,
        DigitalIoPin&& limMax,
        DigitalIoPin&& direction
    );
    virtual ~Motor() { };
    DigitalIoPin motor;
    DigitalIoPin limOrigin;
    DigitalIoPin limMax;
    DigitalIoPin direction;

};
#endif /* MOTOR_H_ */
