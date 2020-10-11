#ifndef PLOTTER_H_
#define PLOTTER_H_
#include "motor.h"
#include "FreeRTOS.h"
#include "semphr.h"

namespace Plotter {
    extern void setMotors(Motor* xMotor_, Motor* yMotor_);
    extern void start_polling(int pps_);
    extern void stop_polling();
    extern void bresenham();
    extern void initValues(int x1_, int y1_, int x2_, int y2_);
    extern void plotLine(int x1_,int y1_, int x2_,int y2_, int pps_);

    extern SemaphoreHandle_t sbRIT;
    extern Motor* xMotor;
    extern Motor* yMotor;
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

