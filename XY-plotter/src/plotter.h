#ifndef PLOTTER_H_
#define PLOTTER_H_
#include "motor.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "Gcode.h"

#define  ACCEL_THRESHOLD_PERCENT 10
#define DEFAULT_PPS 1500
#define USE_ACCEL 0

#define PEN_INITIALISED   (1 << 0)
#define LASER_INITIALISED (1 << 1)
#define CALIBRATED        (1 << 2)
#define LS_FREQ 1000
#define LS_CYCLE 255

class Plotter {
public:
    Plotter(Motor* xMotor, Motor* yMotor);
    static Plotter* activePlotter; // only one plotter can be used in the interrupt
    void resetStepValues();
    void calibrate();
    void start_polling(int pps_);
    void stop_polling();
    void moveIfInArea(Motor* motor, int step, int& currentPos);
    void bresenham();
    void isrFunction(portBASE_TYPE& xHigherPriorityWoken);
    void initBresenhamValues      (int x1_,int y1_, int x2_,int y2_);
    void plotLine        (float x1,float y1, float x2,float y2);
    void plotLineAbsolute(float x1,float y1, float x2,float y2);
    void plotLineRelative(                   float x2,float y2);
    void initPen();
    void setPenValue(uint8_t value);
    void initLaser();
    void handleGcodeData(const Gcode::Data& data);
    void goToOrigin();
    int calculatePps();
    void setLaserPower(uint8_t pw);

    float calculateIfRounding(float coordinate, float& previousCoordinate);

    inline int getTotalStepX() { return totalStepX; };
    inline int getTotalStepY() { return totalStepY; };
    void setXStepInMM(int width)  { xStepMM = (float) totalStepX / width; }
    void setYStepInMM(int height) { yStepMM = (float) totalStepY / height; }

private:
    uint32_t status = 0;
    SemaphoreHandle_t sbRIT;
    Motor* xMotor = nullptr;
    Motor* yMotor = nullptr;

    int currentX;
    int currentY;
    int totalStepX = 0;
    int totalStepY = 0;
    float xStepMM;
    float yStepMM;

    // M5 reply
    bool saveDirX;
    bool saveDirY;
    int savePlottingWidth     = 380;
    int savePlottingHeight    = 300;
    uint8_t savePlottingSpeed = 100; // in percent

    //Pen / Laser
    int ticksPerSecond  = 1'000'000;
    int penFrequency    = 50;
    int minDuty         = ticksPerSecond / 1000; // 1ms
    int maxDuty         = ticksPerSecond / 500; // 2ms
    uint8_t savePenUp   = 160;
    uint8_t savePenDown = 90;
    uint8_t m_power;


    // m_ prefix for  Bresenham values only to make less confusing
    int  m_dx;
    int  m_dy;
    bool m_xGreater;
    int  m_D;
    int  m_steps;
    int  m_count;
    int  m_x;
    int  m_y;
    int  m_prevX;
    int  m_prevY;
    int  m_pps = DEFAULT_PPS;
    int  m_threshold;
};

#endif /* PLOTTER_H_ */

