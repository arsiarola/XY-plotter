#include "plotter.h"
#include "printer.h"
#include "usb/user_vcom.h"

#include <algorithm>
#include <string.h>
#include <math.h>

#include "FreeRTOS.h"

Plotter* Plotter::activePlotter = nullptr;
uint32_t volatile RIT_Count = 0;
void (*RIT_Callback)();
SemaphoreHandle_t RIT_Semaphore = xSemaphoreCreateBinary();

Plotter::Plotter(Motor* xMotor_, Motor* yMotor_)
{
    setMotors(xMotor_, yMotor_);
}

void Plotter::setMotors(Motor* xMotor_, Motor* yMotor_) {
    xMotor = xMotor_; yMotor = yMotor_;
    if (!MOTORS_NULL(xMotor_, yMotor_)) {
        saveDirX = xMotor->getOriginDirection();
        saveDirY = yMotor->getOriginDirection();
    }
}

// TODO: calculate the area and put the values in savePlottingWidth and height
void Plotter::calibrate() {
    if (MOTORS_NULL(xMotor, yMotor)) {
        return;
    }
    ITM_print("Starting calibration\n");
    totalStepX = 0;
    totalStepY = 0;
    xStepMM    = 0;
    yStepMM    = 0;
    activePlotter = this;
    goToOrigin();
    for (int i = 0; i < CALIBRATE_RUNS; ++i) {
        RIT_Start_polling(500, []() {
            portBASE_TYPE xHigherPriorityWoken = pdFALSE;
            bool xStep = !activePlotter->xMotor->readMaxLimit();
            bool yStep = !activePlotter->yMotor->readMaxLimit();
            activePlotter->xMotor->writeStepper(xStep);
            activePlotter->yMotor->writeStepper(yStep);
            activePlotter->totalStepX += !activePlotter->xMotor->readMaxLimit() ? 1 : 0;
            activePlotter->totalStepY += !activePlotter->yMotor->readMaxLimit() ? 1 : 0;
            activePlotter->xMotor->writeStepper(false);
            activePlotter->yMotor->writeStepper(false);
            if(activePlotter->xMotor->readMaxLimit() && activePlotter->yMotor->readMaxLimit()){
                RIT_Stop_polling();
                xSemaphoreGiveFromISR(RIT_Semaphore, &xHigherPriorityWoken);
                ITM_print("Give, RIT-semaphore\n");
            }
            portEND_SWITCHING_ISR(xHigherPriorityWoken);
        });
        xSemaphoreTake(RIT_Semaphore, portMAX_DELAY);
        goToOrigin();
    }

    totalStepX = totalStepX / CALIBRATE_RUNS;
    totalStepY = totalStepY / CALIBRATE_RUNS;

    // mdraw coordinate should be same as emulator or paper
    if(totalStepX>totalStepY)
    	savePlottingWidth = savePlottingHeight * totalStepX / totalStepY;
    else
    	savePlottingHeight = savePlottingWidth * totalStepY / totalStepX;

    // Set step per mm to draw more accurate or same scale as svg picture
    setXStepInMM(savePlottingWidth);
    setYStepInMM(savePlottingHeight);

    ITM_print("xTotal=%d, yTotal=%d\n", totalStepX, totalStepY);
    ITM_print("xMM=%f, yMM=%f\n", xStepMM, yStepMM);
    ITM_print("calibration done\n");
    status |= CALIBRATED;
}

// Simple function for calibrating, should not be used for GCODES
void Plotter::goToOrigin() {
    if (MOTORS_NULL(xMotor, yMotor)) {
        return;
    }
    currentX   = 0;
    currentY   = 0;
	xMotor->writeDirection(xMotor->getOriginDirection());
	yMotor->writeDirection(yMotor->getOriginDirection());

    RIT_Start_polling(500, []() {
        portBASE_TYPE xHigherPriorityWoken = pdFALSE;
        bool xStep = !activePlotter->xMotor->readOriginLimit();
        bool yStep = !activePlotter->yMotor->readOriginLimit();
		activePlotter->xMotor->writeStepper(xStep);
		activePlotter->yMotor->writeStepper(yStep);
		activePlotter->xMotor->writeStepper(false);
		activePlotter->yMotor->writeStepper(false);
        if(!xStep && !yStep){
            RIT_Stop_polling();
            xSemaphoreGiveFromISR(RIT_Semaphore, &xHigherPriorityWoken);
            ITM_print("Give, RIT-semaphore\n");
        }
        portEND_SWITCHING_ISR(xHigherPriorityWoken);
    });
    xSemaphoreTake(RIT_Semaphore, portMAX_DELAY);

	xMotor->writeDirection(!xMotor->getOriginDirection());
	yMotor->writeDirection(!yMotor->getOriginDirection());
    xMotor->writeStepper(true);
    yMotor->writeStepper(true);
    vTaskDelay(1);
    xMotor->writeStepper(false);
    yMotor->writeStepper(false);
    vTaskDelay(1);
    ITM_print("comeback to origin\n");
}


void Plotter::moveIfInArea(bool xStep, bool yStep) {
    if (MOTORS_NULL(xMotor, yMotor)) {
        return;
    }
    if (xMotor->isOriginDirection() && currentX < totalStepX && currentX > 0) {
        xMotor->writeStepper(xStep);
    }
    else if (!xMotor->isOriginDirection() && currentX < totalStepX-1 && currentX >= 0) {
        xMotor->writeStepper(xStep);
        ITM_print("currentX=%d, currentY=%d\n", currentX, currentY);

    }
    if (yMotor->isOriginDirection() && currentY < totalStepY && currentY > 0) {
        yMotor->writeStepper(yStep);
        ITM_print("currentX=%d, currentY=%d\n", currentX, currentY);

    }
    else if (!yMotor->isOriginDirection() && currentY < totalStepY-1 && currentY >= 0) {
        yMotor->writeStepper(yStep);
    }

    currentX += xMotor->isOriginDirection() ? -BOOL_TO_NUM(xStep) : BOOL_TO_NUM(xStep);
    currentY += yMotor->isOriginDirection() ? -BOOL_TO_NUM(yStep) : BOOL_TO_NUM(yStep);

}

void Plotter::bresenham() {
    if (MOTORS_NULL(xMotor, yMotor)) {
        return;
    }

    bool xStep = m_x != m_prevX;
    bool yStep = m_y != m_prevY;
    moveIfInArea(xStep, yStep);

    m_prevX = m_x;
    m_prevY = m_y;
    if (m_D > 0) {
        m_xGreater ? ++m_y : ++m_x;
        m_D -= 2 * (m_xGreater ? m_dx : m_dy);
    }
    m_xGreater ? ++m_x : ++m_y;
    m_D += 2 * (m_xGreater ? m_dy : m_dx);

    xMotor->writeStepper(false);
    yMotor->writeStepper(false);
   ++m_count;
}


void Plotter::initBresenhamValues(int x1_,int y1_, int x2_,int y2_) {
    if (MOTORS_NULL(xMotor, yMotor)) {
        return;
    }

    xMotor->writeDirection(x2_ > x1_ ? !xMotor->getOriginDirection() : xMotor->getOriginDirection());
    yMotor->writeDirection(y2_ > y1_ ? !yMotor->getOriginDirection() : yMotor->getOriginDirection());
    int x1      = x1_ < x2_ ? x1_ : x2_;
    int x2      = x1_ > x2_ ? x1_ : x2_;
    int y1      = y1_ < y2_ ? y1_ : y2_;
    int y2      = y1_ > y2_ ? y1_ : y2_;
    m_dx        = abs(x2-x1);
    m_dy        = abs(y2-y1);
    m_xGreater  = (m_dx > m_dy);
    m_D         = m_xGreater ? 2*m_dy - m_dx : 2*m_dx - m_dy;
    m_prevX     = x1;
    m_prevY     = y1;
    m_x         = x1;
    m_y         = y1;

    m_steps     = std::max(m_dx, m_dy);
    m_count     = 0;
    m_threshold = m_steps * ACCEL_THRESHOLD_PERCENT / 100 ;
    ITM_print("%d,%d %d,%d\n", x1,y1, x2,y2);
}

int Plotter::calculatePps() {
    int pps;
    if ((m_steps - m_count) < m_threshold) {
        pps = m_pps * (m_steps - m_count) / m_threshold;
    }
    else if (m_count < m_threshold) {
        pps = m_pps * (m_count / m_threshold);
    }
    else
        pps = m_pps;

    // Set to minimum value pps not in bounds
    if (pps <= 0) pps = m_pps * ACCEL_THRESHOLD_PERCENT / 100;
    return pps;
}

void Plotter::plotLineAbsolute(float x1,float y1, float x2,float y2) {
    plotLine(
        x1 + ((float)currentX/xStepMM),
        y1 + ((float)currentY/yStepMM),
        x2,
        y2
    );
}

void Plotter::plotLineRelative(float x2,float y2) {
    plotLine(
        0,
        0,
        x2,
        y2
    );
}

// TODO: since coordinates are given as floats think about error checking
void Plotter::plotLine(float x1,float y1, float x2,float y2) {
    if ((status & CALIBRATED) == 0) {
        ITM_print("Plotter not calibrated exiting plotting\n");
        return;
    }

    if (MOTORS_NULL(xMotor, yMotor)) {
        return;
    }

    initBresenhamValues(
        round(x1*xStepMM),
        round(y1*yStepMM),
        round(x2*xStepMM),
        round(y2*yStepMM)
    );
#if USE_ACCEL == 1
       start_polling(calculatePps());
#else
       start_polling(m_pps);
#endif /*USE_ACCEL*/
    xSemaphoreTake(RIT_Semaphore, portMAX_DELAY);
}
void Plotter::setPenValue(uint8_t value) {
    if (status & PEN_INITIALISED) {
        LPC_SCT0->MATCHREL[1].U = value * (maxDuty-minDuty) / 255 + minDuty;;
        LPC_SCT0->OUT[0].SET = 1;
    }
    else
        ITM_print("Cannot change pen value, not initialised\n");
}

void Plotter::initPen() {
    Chip_SCT_Init(LPC_SCT0);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
	#if defined(BOARD_NXP_LPCXPRESSO_1549)
	Chip_SWM_MovablePortPinAssign(SWM_SCT0_OUT0_O, 0, 10);
	#endif
	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);
    LPC_SCT0->CONFIG |= SCT_CONFIG_32BIT_COUNTER | SCT_CONFIG_AUTOLIMIT_L;
    LPC_SCT0->CTRL_U = SCT_CTRL_PRE_L(SystemCoreClock / TICKS_PER_SECOND - 1) | SCT_CTRL_CLRCTR_L | SCT_CTRL_HALT_L;
    LPC_SCT0->MATCHREL[0].U = TICKS_PER_SECOND / PEN_FREQ - 1;
	LPC_SCT1->MATCHREL[1].L = 0;
	LPC_SCT0->EVENT[0].STATE = 0x1;         // event 0 happens in state 1
    LPC_SCT0->EVENT[1].STATE = 0x1;         // event 1 happens in state 1
    LPC_SCT0->EVENT[0].CTRL = (0 << 0) | (1 << 12); // match 0 condition only
    LPC_SCT0->EVENT[1].CTRL = (1 << 0) | (1 << 12); // match 1 condition only
    LPC_SCT0->OUT[0].SET = (1 << 0);                // event 0 will set SCTx_OUT0
    LPC_SCT0->OUT[0].CLR = (1 << 1);                // event 1 will clear SCTx_OUT0
    LPC_SCT0->CTRL_L &= ~(1 << 2);                  // unhalt it by clearing bit 2 of CTRL reg
    status |= PEN_INITIALISED;
    setPenValue(160);
}

void Plotter::initLaser() {
    Chip_SCT_Init(LPC_SCT1);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
	#if defined(BOARD_NXP_LPCXPRESSO_1549)
	Chip_SWM_MovablePortPinAssign(SWM_SCT1_OUT0_O, 0, 12);
	#endif
	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);
    LPC_SCT1->CONFIG |= SCT_CONFIG_32BIT_COUNTER | SCT_CONFIG_AUTOLIMIT_L;
    LPC_SCT1->CTRL_U = SCT_CTRL_PRE_L(SystemCoreClock / TICKS_PER_SECOND - 1) | SCT_CTRL_CLRCTR_L | SCT_CTRL_HALT_L;
    LPC_SCT1->MATCHREL[0].U = LASER_FREQ - 1; // Set the laser low
	LPC_SCT1->EVENT[0].STATE = 0x1;         // event 0 happens in state 1
    LPC_SCT1->EVENT[1].STATE = 0x1;         // event 1 happens in state 1
    LPC_SCT1->EVENT[0].CTRL = (0 << 0) | (1 << 12); // match 0 condition only
    LPC_SCT1->EVENT[1].CTRL = (1 << 0) | (1 << 12); // match 1 condition only
    LPC_SCT1->OUT[0].SET = (1 << 0);                // event 0 will set SCTx_OUT0
    LPC_SCT1->OUT[0].CLR = (1 << 1);                // event 1 will clear SCTx_OUT0
    LPC_SCT1->CTRL_L &= ~(1 << 2);                  // unhalt it by clearing bit 2 of CTRL reg
    status |= LASER_INITIALISED;
    setLaserPower(0);
}

void Plotter::setLaserPower(uint8_t pw){
	m_power = pw;
	LPC_SCT1->MATCHREL[1].L = m_power * LASER_FREQ / LASER_CYCLE;
    LPC_SCT1->OUT[0].SET = m_power > 0 ? (1 << 0) : 0; // Disable output when laser off

}

void Plotter::handleGcodeData(const Gcode::Data &data) {
    switch (data.id) {
        case Gcode::Id::G1:
        case Gcode::Id::G28:
            UART_print("%f, %f, %d",
                        data.data.g1.moveX,
                        data.data.g1.moveY,
                        data.data.g1.relative
                       );
            if (data.data.g1.relative) {
                plotLineRelative(
                    data.data.g1.moveX, data.data.g1.moveY
                );
            }
            else {
            		plotLineAbsolute(
                		0,0,
						data.data.g1.moveX, data.data.g1.moveY
                	);
            }
            break;

        case Gcode::Id::M1:
            UART_print("%u", data.data.m1.penPos);
            setPenValue(data.data.m1.penPos);
            vTaskDelay(configTICK_RATE_HZ / 5); // Time to make sure that the pen up or down in the sim
            break;

        case Gcode::Id::M2:
            UART_print("%u, %u", data.data.m2.savePenUp, data.data.m2.savePenDown);
            savePenUp   = data.data.m2.savePenUp;
            savePenDown = data.data.m2.savePenDown;
            break;

        case Gcode::Id::M4:
            // TODO: create function for setting laser power
            UART_print("%u", data.data.m4.laserPower);
            setLaserPower(data.data.m4.laserPower);
            vTaskDelay(configTICK_RATE_HZ / 5); // Time to make sure that the laser off in the sim
            break;

        case Gcode::Id::M5:
            UART_print("%d, %d, %u, %u, %u",
                        data.data.m5.dirX,
                        data.data.m5.dirY,
                        data.data.m5.height,
                        data.data.m5.width,
                        data.data.m5.speed
                       );

            saveDirX           = data.data.m5.dirX;
            saveDirY           = data.data.m5.dirY;
            savePlottingHeight = data.data.m5.height;
            savePlottingWidth  = data.data.m5.width;
            savePlottingSpeed  = data.data.m5.speed;

            xMotor->setOriginDirection(saveDirX); // update motor directions
            yMotor->setOriginDirection(saveDirY);
            calibrate();
            break;
        case Gcode::Id::M10:
        	do {
            char buffer[64];
            snprintf(buffer, 64, Gcode::toFormat(CREATE_GCODE_ID('M', 10)),
                        savePlottingWidth,
                        savePlottingHeight,
                        saveDirX ? 1 : 0,
                        saveDirY ? 1 : 0,
                        savePlottingSpeed,
                        savePenUp,
                        savePenDown
                        );
            USB_send((uint8_t *) buffer, strlen(buffer));
        	} while(0); // do while 0 so we can create the local buffer variable
            break;

        case Gcode::Id::M11:
        	do {
            char buffer[32];
            snprintf(buffer, 32, Gcode::toFormat(CREATE_GCODE_ID('M', 11)),
                    xMotor->readMinLimit(),
                    xMotor->readMaxLimit(),
                    yMotor->readMinLimit(),
                    yMotor->readMaxLimit()
                        );
            USB_send((uint8_t *) buffer, strlen(buffer));
        	} while(0);
            break;
    }
}

void PlotterIsrFunction() {
    if (Plotter::activePlotter != nullptr) Plotter::activePlotter->isrFunction();
}


void Plotter::isrFunction() {
    portBASE_TYPE xHigherPriorityWoken = pdFALSE;
	bool minX = xMotor->readOriginLimit() && (xMotor->isOriginDirection());
	bool maxX = xMotor->readMaxLimit()    && (!xMotor->isOriginDirection());
	bool minY = yMotor->readOriginLimit() && (yMotor->isOriginDirection());
	bool maxY = yMotor->readMaxLimit()    && (!yMotor->isOriginDirection());
	if (minX || maxX || minY || maxY || m_count > m_steps) {
        ITM_print("currentX=%d, currentY=%d\n", currentX, currentY);
        stop_polling();
        xSemaphoreGiveFromISR(RIT_Semaphore, &xHigherPriorityWoken);
	}

    else {
		bresenham();
    	if(m_power > 0)
    	       start_polling(m_pps);
    	else{
#if USE_ACCEL == 1
    		start_polling(calculatePps());
#else
    		start_polling(m_pps);
#endif /*USE_ACCEL*/
    	}
   }
    portEND_SWITCHING_ISR(xHigherPriorityWoken);
}

extern "C" {
    void RIT_IRQHandler(void) {
        Chip_RIT_ClearIntStatus(LPC_RITIMER); // clear IRQ flag
        if (RIT_Callback != nullptr) RIT_Callback();
    }
}

void RIT_Start_polling(uint32_t count, int pps, RIT_void_t callback = RIT_Callback) {
    RIT_Count = count;
    RIT_Start_polling(pps, callback);
}

void RIT_Start_polling(int pps, RIT_void_t callback) {
    RIT_Callback = callback;
    RIT_Start_polling(pps);
}

void RIT_Start_polling(int pps) {
    Chip_RIT_Disable(LPC_RITIMER);
    uint64_t cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() / pps;
    Chip_RIT_EnableCompClear(LPC_RITIMER);
    Chip_RIT_SetCounter(LPC_RITIMER, 0);
    Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
    Chip_RIT_Enable(LPC_RITIMER);
    NVIC_EnableIRQ(RITIMER_IRQn);
}

void RIT_Stop_polling() {
    NVIC_DisableIRQ(RITIMER_IRQn);
    Chip_RIT_Disable(LPC_RITIMER);
}

void Plotter::start_polling(int pps) {
    RIT_Callback = PlotterIsrFunction;
    RIT_Start_polling(pps * savePlottingSpeed / 100);
}

void Plotter::stop_polling() {
    RIT_Stop_polling();
}

