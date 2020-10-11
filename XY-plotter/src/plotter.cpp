#include "plotter.h"
#include "printer.h"
#include <algorithm>

namespace Plotter {
    SemaphoreHandle_t sbRIT = xSemaphoreCreateBinary();
    Motor* xMotor = nullptr;
    Motor* yMotor = nullptr;
    int currentX = 0;
    int currentY = 0;
    int x1 = 0;
    int x2 = 0;
    int y1 = 0;
    int y2 = 0;
    int dx = 0;
    int dy = 0;
    bool xGreater = 0;
    int m_new = 0;
    int slope_error_new = 0;
    int steps = 0;
    int count = 0;
    int x = 0;
    int y = 0;
    int prevX = 0;
    int prevY = 0;
    int pps = 0;

void setMotors(Motor* xMotor_, Motor* yMotor_) {
    xMotor = xMotor_;
    yMotor = yMotor_;
}

void calibrate() {
    while(
            xMotor->limMax.read() ||
            xMotor->limOrigin.read() ||
            yMotor->limMax.read() ||
            yMotor->limOrigin.read()
         );
    xMotor->direction.write((xMotor->originDirection));
    yMotor->direction.write((yMotor->originDirection));
    /* while (xMotor->limOrigin.read() == false) { */
    /*     xMotor->motor.write(true); */
    /*     xMotor->motor.write(false); */
    /* } */
    /* while (yMotor->limOrigin.read() == false) { */
    /*     yMotor->motor.write(true); */
    /*     yMotor->motor.write(false); */
    /* } */
    /* xMotor->direction.write((!xMotor->originDirection)); */
    /* yMotor->direction.write((!yMotor->originDirection)); */
    /* xMotor->motor.write(true); */
    /* xMotor->motor.write(false); */
    /* yMotor->motor.write(true); */
    /* yMotor->motor.write(false); */
    currentX = 0;
    currentY = 0;
}

void bresenham() {
    if (xMotor == nullptr || yMotor == nullptr) {
        ITM_print("Atleast one motor not initalised! exiting bresenham()\n");
        return;
    }
    int xStep = (bool)(x != prevX);
    int yStep = (bool)(y != prevY);
    currentX += xMotor->direction.read() == xMotor->originDirection ? -xStep : xStep;
    currentY += yMotor->direction.read() == yMotor->originDirection ? -yStep : yStep;
    xMotor->motor.write(xStep);
    yMotor->motor.write(yStep);
    /* ITM_print("count = %d, steps = %d\n", steps, count); */
    /* ITM_print("(%d,%d)  %d,%d\n", x,y, (bool)(x != prevX), (bool)(y != prevY)); */
    prevX = x;
    prevY = y;

    slope_error_new += m_new;
    if (xGreater) {
        ++x;
        if (slope_error_new >= 0) {
            slope_error_new -= 2 * dx;
            ++y;
        }
    }
    else {
        ++y;
        if (slope_error_new >= 0) {
            slope_error_new -= 2 * dy;
            ++x;
        }
    }
    ++count;
    xMotor->motor.write(false);
    yMotor->motor.write(false);
}

void initValues(int x1_, int y1_, int x2_, int y2_) {
    if (xMotor == nullptr || yMotor == nullptr) {
        ITM_print("Atleast one motor not initalised! exiting value initialisation\n");
        return;
    }
    xMotor->direction.write(x2_ > x1_);
    yMotor->direction.write(y2_ > y1_);
    x1              = x1_ < x2_ ? x1_ : x2_;
    x2              = x1_ > x2_ ? x1_ : x2_;
    y1              = y1_ < y2_ ? y1_ : y2_;
    y2              = y1_ > y2_ ? y1_ : y2_;
    dx              = abs(x2-x1);
    dy              = abs(y2-y1);
    xGreater        = (dx > dy);
    m_new           = xGreater ? 2 * dy : 2 * dx;
    slope_error_new = m_new - (xGreater ? dx : dy);
    prevX           = x1;
    prevY           = y1;
    steps           = std::max(dx, dy);
    count           = 0;
    x               = x1;
    y               = y1;
    prevX           = x;
    prevY           = y;
    ITM_print("%d,%d    %d,%d\n", x1_,y1_, x2_,y2_);
    ITM_print("%d,%d    %d,%d\n", x1,y1, x2,y2);
}

extern "C" {
    void RIT_IRQHandler(void) {
        Chip_RIT_ClearIntStatus(LPC_RITIMER); // clear IRQ flag
        portBASE_TYPE xHigherPriorityWoken = pdFALSE;
        bresenham();
        if (count > steps) {
            stop_polling();
            xSemaphoreGiveFromISR(sbRIT, &xHigherPriorityWoken);
        }
        else {
            start_polling(pps);
        }

        // End the ISR and (possibly) do a context switch
        portEND_SWITCHING_ISR(xHigherPriorityWoken);
    }
}


void plotLineAbsolute(int x1_,int y1_, int x2_,int y2_, int pps_) {
    plotLine(
        x1_,
        y1_,
        x2_ - currentX,
        y2_ - currentY,
        pps_
    );
}

// TODO: since coordinates are given as floats think about error checking
void plotLine(int x1_,int y1_, int x2_,int y2_, int pps_) {
    initValues(x1_,y1_, x2_,y2_);
    pps = pps_;
    start_polling(pps);
    xSemaphoreTake(sbRIT, portMAX_DELAY);
}

void start_polling(int pps_) {
    Chip_RIT_Disable(LPC_RITIMER);
    uint64_t cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() / pps_;
    Chip_RIT_EnableCompClear(LPC_RITIMER);
    Chip_RIT_SetCounter(LPC_RITIMER, 0);
    Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
    Chip_RIT_Enable(LPC_RITIMER);
    NVIC_EnableIRQ(RITIMER_IRQn);
}

void stop_polling() {
    NVIC_DisableIRQ(RITIMER_IRQn);
    Chip_RIT_Disable(LPC_RITIMER);
}
}
