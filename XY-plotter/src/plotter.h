#ifndef PLOTTER_H_
#define PLOTTER_H_
#include "motor.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "Gcode.h"

class Plotter {
public:
    Plotter(Motor* xMotor, Motor* yMotor);
    static Plotter* activePlotter; // only one plotter can be used in the interrupt
    void calibrate();
    void start_polling(int pps_);
    void stop_polling();
    void moveIfInArea(Motor* motor, bool step);
    void bresenham();
    void isrFunction(portBASE_TYPE& xHigherPriorityWoken);
    void initValues(int x1_, int y1_, int x2_, int y2_);
    void plotLine(int x1_,int y1_, int x2_,int y2_);
    void plotLineAbsolute(int x1_,int y1_, int x2_,int y2_);
    void initPen();
    void setPenValue(uint8_t value);
    void initLaser();
    void handleGcodeData(const Gcode::Data &data);


    inline int getTotalStepX() { return totalStepX; };
    inline int getTotalStepY() { return totalStepY; };
    inline void setXStepInMM(float standard) { xStepMM = (float) totalStepX / standard; };
    inline void setYStepInMM(float standard) { yStepMM = (float) totalStepY / standard; };

private:
    SemaphoreHandle_t sbRIT;
    Motor* xMotor = nullptr;
    Motor* yMotor = nullptr;
    int currentX;
    int currentY;

    long totalStepX;
    long totalStepY;
    float xStepMM;
    float yStepMM;

    // M5 reply
    bool saveDirX;// TODO: what should the default values be when M10 asks in the beginning
    bool saveDirY;
    uint32_t savePlottingWidth = 380;
    uint32_t savePlottingHeight = 320;
    uint8_t savePlottingSpeed = 100;  // in percent

    //Pen
    int ticksPerSecond = 1'000'000;
	int penFrequency = 50;
	int minDuty = ticksPerSecond / 1000; // 1ms
	int maxDuty = ticksPerSecond / 500;  // 2ms
    uint8_t savePenUp = 160;
    uint8_t savePenDown = 90;
    uint8_t currentPenValue;

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
    int pps = 1500;
};

#endif /* PLOTTER_H_ */

