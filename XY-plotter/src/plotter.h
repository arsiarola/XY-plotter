#ifndef PLOTTER_H_
#define PLOTTER_H_
#include "motor.h"
#include "FreeRTOS.h"
#include "semphr.h"

class Plotter {
public:
    Plotter(Motor* xMotor, Motor* yMotor);
    static Plotter* activePlotter; // only one plotter can be used in the interrupt
    void calibrate();
    void start_polling(int pps_);
    void stop_polling();
    void bresenham();
    void isrFunction(portBASE_TYPE& xHigherPriorityWoken);
    void initValues(int x1_, int y1_, int x2_, int y2_);
    void plotLine(int x1_,int y1_, int x2_,int y2_, int pps_);
    void plotLineAbsolute(int x1_,int y1_, int x2_,int y2_, int pps_);
    void initPen();
    void setPenValue(uint8_t value);
    void initLaser();

private:
    SemaphoreHandle_t sbRIT;
    Motor* xMotor = nullptr;
    Motor* yMotor = nullptr;
    int currentX;
    int currentY;

    int x1;
    int x2;
    int y1;
    int y2;
    int dx;
    int dy;
    bool xGreater;
    int m_new;
    int slope_error_new;
    int steps;
    int count;
    int x;
    int y;
    int prevX;
    int prevY;
    int pps;
};

#endif /* PLOTTER_H_ */

