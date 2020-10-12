#ifndef PLOTTER_H_
#define PLOTTER_H_
#include "motor.h"
#include "FreeRTOS.h"
#include "semphr.h"

namespace Plotter {
    void setMotors(Motor* xMotor_, Motor* yMotor_);
    void calibrate();
    void start_polling(int pps_);
    void stop_polling();
    void bresenham();
    void initValues(int x1_, int y1_, int x2_, int y2_);
    void plotLine(int x1_,int y1_, int x2_,int y2_, int pps_);
    void plotLineAbsolute(int x1_,int y1_, int x2_,int y2_, int pps_);
void initPen();
void setPenValue(uint8_t value);

    extern SemaphoreHandle_t sbRIT;
    extern Motor* xMotor;
    extern Motor* yMotor;
    extern int currentX;
    extern int currentY;

    extern int x1;
    extern int x2;
    extern int y1;
    extern int y2;
    extern int dx;
    extern int dy;
    extern bool xGreater;
    extern int m_new;
    extern int slope_error_new;
    extern int steps;
    extern int count;
    extern int x;
    extern int y;
    extern int prevX;
    extern int prevY;
    extern int pps;
}

#endif /* PLOTTER_H_ */

