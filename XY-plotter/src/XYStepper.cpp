/*
 * XYStepper.cpp
 *
 *  Created on: 7 Oct 2020
 *      Author: DucVoo
 */
#include <XYStepper.h>
#include <stdlib.h>
#include "math.h"
#include "ITM_write.h"
#include "DigitalIoPin.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

XYStepper::XYStepper(DigitalIoPin *dirX, DigitalIoPin *stepX, DigitalIoPin *dirY, DigitalIoPin *stepY,
                     DigitalIoPin *lmXMin, DigitalIoPin *lmXMax, DigitalIoPin *lmYMin, DigitalIoPin *lmYMax) : dirXPin(dirX), stepXPin(stepX), dirYPin(dirY), stepYPin(stepY),
                                                                                                               lmXMin(lmXMin), lmXMax(lmXMax), lmYMin(lmYMin), lmYMax(lmYMax)
{

    sbRIT = xSemaphoreCreateBinary();
}

XYStepper::~XYStepper()
{
}


void XYStepper::RIT_start(int count, int pps)
{
    uint64_t cmp_value;

    // Determine approximate compare value based on clock rate and passed interval
    cmp_value = (uint64_t)Chip_Clock_GetSystemClockRate() / pps;

    // disable timer during configuration
    Chip_RIT_Disable(LPC_RITIMER);

    RIT_count = count;

    // enable automatic clear on when compare value==timer value
    // this makes interrupts trigger periodically
    Chip_RIT_EnableCompClear(LPC_RITIMER);

    // reset the counter
    Chip_RIT_SetCounter(LPC_RITIMER, 0);
    Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);

    // start counting
    Chip_RIT_Enable(LPC_RITIMER);

    // Enable the interrupt signal in NVIC (the interrupt controller)
    NVIC_EnableIRQ(RITIMER_IRQn);

    // wait for ISR to tell that we're done
    if (xSemaphoreTake(sbRIT, portMAX_DELAY) == pdTRUE)
    {
        // Disable the interrupt signal in NVIC (the interrupt controller)
        NVIC_DisableIRQ(RITIMER_IRQn);
    }
    else
    {
        // unexpected error
    }
}
