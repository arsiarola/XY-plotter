/*
 * XYStepper.h
 *
 *  Created on: 7 Oct 2020
 *      Author: DucVoo
 */

#ifndef XYSTEPPER_H_
#define XYSTEPPER_H_

#include "DigitalIoPin.h"
#include "FreeRTOS.h"
#include "semphr.h"

class XYStepper
{
public:
    XYStepper(DigitalIoPin *dirX, DigitalIoPin *stepX, DigitalIoPin *dirY, DigitalIoPin *stepY,
              DigitalIoPin *lmXMin, DigitalIoPin *lmXMax, DigitalIoPin *lmYMin, DigitalIoPin *lmYMax);
    
    virtual XYStepper();

    void RIT_start(int count, int pps); // pps = pulse per revolution


private:

    DigitalIoPin *dirXPin;
    DigitalIoPin *stepXPin;
    DigitalIoPin *dirYPin;
    DigitalIoPin *stepYPin;
    DigitalIoPin *lmXMin;
    DigitalIoPin *lmXMax;
    DigitalIoPin *lmYMin;
    DigitalIoPin *lmYMax;

    volatile int RIT_count;

    SemaphoreHandle_t sbRIT;

};

#endif /* XYSTEPPER_H_ */
