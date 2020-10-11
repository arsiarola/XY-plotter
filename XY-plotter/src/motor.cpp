#include "motor.h"

Motor::Motor(
        DigitalIoPin&& motor,
        DigitalIoPin&& limOrigin,
        DigitalIoPin&& limMax,
        DigitalIoPin&& direction
    ) :
        motor(motor),
        limOrigin (limOrigin),
        limMax(limMax),
        direction(direction)
{ }
