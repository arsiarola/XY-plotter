#include "motor.h"

Motor::Motor(
        DigitalIoPin&& motor,
        DigitalIoPin&& direction,
        DigitalIoPin&& limOrigin,
        DigitalIoPin&& limMax,
        bool originDirection
    ) :
        motor(motor),
        direction(direction),
        limOrigin (limOrigin),
        limMax(limMax),
        originDirection(originDirection)
{ }
