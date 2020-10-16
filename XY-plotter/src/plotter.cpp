#include "plotter.h"
#include "printer.h"
#include "usb/user_vcom.h"

#include <algorithm>
#include <string.h>
#include <math.h>

Plotter* Plotter::activePlotter = nullptr;
Plotter::Plotter(Motor* xMotor, Motor* yMotor) :
    xMotor(xMotor),
    yMotor(yMotor)
    {
    	sbRIT = xSemaphoreCreateBinary();
        saveDirX = !xMotor->getOriginDirection();
        saveDirY = !xMotor->getOriginDirection();
}

void Plotter::resetStepValues() {
    totalStepX = 0;
    totalStepY = 0;
    currentX   = 0;
    currentY   = 0;
    xStepMM    = 0;
    yStepMM    = 0;
}

// TODO: calculate the area and put the values in savePlottingWidth and height
void Plotter::calibrate() {
    resetStepValues();
    goToOrigin();

	bool xStep;
	bool yStep;

    int i = 0;
    int times = 0;
    do {
        //ITM_print("%d: xStep=%\n", i, xStep);
        xStep = !xMotor->readMaxLimit();
        yStep = !yMotor->readMaxLimit();
		xMotor->writeStepper(xStep);
		yMotor->writeStepper(yStep);
		vTaskDelay(1);
		xMotor->writeStepper(false);
		yMotor->writeStepper(false);
        totalStepX += !xMotor->readMaxLimit() ? 1 : 0;
        totalStepY += !yMotor->readMaxLimit() ? 1 : 0;
		vTaskDelay(1);
        ITM_print("%d: xStep=%\n", i++, xStep);
        if(xMotor->readMaxLimit() && yMotor->readMaxLimit()){
        	times++;
            goToOrigin();
        }

    } while (times < 2);

    totalStepX = totalStepX/2;
    totalStepY = totalStepY/2;

    if(totalStepX>totalStepY)
    	savePlottingWidth = savePlottingHeight * totalStepX / totalStepY;
    else
    	savePlottingHeight = savePlottingWidth * totalStepY / totalStepX;
    ITM_print("width = %d \n", savePlottingWidth);
    setXStepInMM(savePlottingWidth);
    setYStepInMM(savePlottingHeight);
    ITM_print("calibration done\n");
    ITM_print("xTotal=%d, yTotal=%d\n", totalStepX, totalStepY);
    ITM_print("xMM=%f, yMM=%f\n", xStepMM, yStepMM);
    status |= CALIBRATED;
}

// Simple function for calibrating, should not be used for GCODES
void Plotter::goToOrigin() {
	xMotor->writeDirection(xMotor->getOriginDirection());
	yMotor->writeDirection(yMotor->getOriginDirection());
	bool xStep;
	bool yStep;

    do {
        xStep = !xMotor->readOriginLimit();
        yStep = !yMotor->readOriginLimit();
		xMotor->writeStepper(xStep);
		yMotor->writeStepper(yStep);
		vTaskDelay(1);
		xMotor->writeStepper(false);
		yMotor->writeStepper(false);
		vTaskDelay(1);
    } while (xStep || yStep);

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
    if (xMotor == nullptr || yMotor == nullptr) {
        ITM_print("Atleast one motor not initalised! exiting bresenham()\n");
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
    if (xMotor == nullptr || yMotor == nullptr) {
        ITM_print("Atleast one motor not initalised! exiting value initialisation\n");
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

void Plotter::isrFunction(portBASE_TYPE& xHigherPriorityWoken ) {
	bool minX = xMotor->readOriginLimit() && (xMotor->isOriginDirection());
	bool maxX = xMotor->readMaxLimit()    && (!xMotor->isOriginDirection());
	bool minY = yMotor->readOriginLimit() && (yMotor->isOriginDirection());
	bool maxY = yMotor->readMaxLimit()    && (!yMotor->isOriginDirection());
	if (minX || maxX || minY || maxY || m_count > m_steps) {
        ITM_print("currentX=%d, currentY=%d\n", currentX, currentY);
        stop_polling();
        xSemaphoreGiveFromISR(sbRIT, &xHigherPriorityWoken);
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
}

extern "C" {
    void RIT_IRQHandler(void) {
        Chip_RIT_ClearIntStatus(LPC_RITIMER); // clear IRQ flag
        portBASE_TYPE xHigherPriorityWoken = pdFALSE;
        if (Plotter::activePlotter != nullptr) Plotter::activePlotter->isrFunction(xHigherPriorityWoken);

        // End the ISR and (possibly) do a context switch
        portEND_SWITCHING_ISR(xHigherPriorityWoken);
    }
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
    xSemaphoreTake(sbRIT, portMAX_DELAY);
}

void Plotter::start_polling(int pps) {
    pps = pps * savePlottingSpeed / 100;
    Chip_RIT_Disable(LPC_RITIMER);
    uint64_t cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() / pps;
    Chip_RIT_EnableCompClear(LPC_RITIMER);
    Chip_RIT_SetCounter(LPC_RITIMER, 0);
    Chip_RIT_SetCompareValue(LPC_RITIMER, cmp_value);
    Chip_RIT_Enable(LPC_RITIMER);
    NVIC_EnableIRQ(RITIMER_IRQn);
}

void Plotter::stop_polling() {
    NVIC_DisableIRQ(RITIMER_IRQn);
    Chip_RIT_Disable(LPC_RITIMER);
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
    LPC_SCT0->CTRL_U = SCT_CTRL_PRE_L(SystemCoreClock / ticksPerSecond - 1) | SCT_CTRL_CLRCTR_L | SCT_CTRL_HALT_L;
    LPC_SCT0->MATCHREL[0].U = ticksPerSecond / penFrequency - 1;
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
    LPC_SCT1->CTRL_U = SCT_CTRL_PRE_L(SystemCoreClock / ticksPerSecond - 1) | SCT_CTRL_CLRCTR_L | SCT_CTRL_HALT_L;
    LPC_SCT1->MATCHREL[0].U = LS_FREQ - 1; // Set the laser low
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
	LPC_SCT1->MATCHREL[1].L = m_power * LS_FREQ / LS_CYCLE;
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
            calibrate();
            break;
        case Gcode::Id::M10:
        	do {
            char buffer[64];
            snprintf(buffer, 64, Gcode::toFormat(CREATE_GCODE_ID('M', 10)),
                        savePlottingWidth,
                        savePlottingHeight,
                        saveDirX,
                        saveDirY,
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
