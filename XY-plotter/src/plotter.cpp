#include "plotter.h"
#include "printer.h"
#include "usb/user_vcom.h"

#include <algorithm>
#include <string.h>

Plotter* Plotter::activePlotter = nullptr;
Plotter::Plotter(Motor* xMotor, Motor* yMotor) :
    xMotor(xMotor),
    yMotor(yMotor)
    {
    	sbRIT = xSemaphoreCreateBinary();
        saveDirX = !xMotor->getOriginDirection();
        saveDirY = !xMotor->getOriginDirection();
}

// TODO: calculate the area and put the values in savePlottingWidth and height
void Plotter::calibrate() {
    while(
            xMotor->readMaxLimit()    ||
            yMotor->readMaxLimit()    ||
            xMotor->readOriginLimit() ||
            yMotor->readOriginLimit()
     ){}
	xMotor->writeDirection(xMotor->getOriginDirection());
	yMotor->writeDirection(yMotor->getOriginDirection());
	bool xRead;
	bool yRead;
	do {
		xRead = xMotor->readOriginLimit();
		yRead = yMotor->readOriginLimit();
		xMotor->writeStepper(!xRead);
		yMotor->writeStepper(!yRead);
		vTaskDelay(1);
		xMotor->writeStepper(false);
		yMotor->writeStepper(false);
	} while (!xRead && !yRead);


	xMotor->writeDirection(!xMotor->getOriginDirection());
	yMotor->writeDirection(!yMotor->getOriginDirection());
	xMotor->writeStepper(true);
	yMotor->writeStepper(true);
	vTaskDelay(1);
	xMotor->writeStepper(false);
	yMotor->writeStepper(false);
    currentX = 0;
    currentY = 0;
}

void Plotter::bresenham() {
    if (xMotor == nullptr || yMotor == nullptr) {
        ITM_print("Atleast one motor not initalised! exiting bresenham()\n");
        return;
    }
    int xStep = (bool)(x != prevX) ? 1 : 0;
    int yStep = (bool)(y != prevY) ? 1 : 0;
    currentX += xMotor->readDirection() == xMotor->getOriginDirection() ? -xStep : xStep;
    currentY += yMotor->readDirection() == yMotor->getOriginDirection() ? -yStep : yStep;
    xMotor->writeStepper(xStep);
    yMotor->writeStepper(yStep);
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
    xMotor->writeStepper(false);
    yMotor->writeStepper(false);
}

void Plotter::initValues(int x1_, int y1_, int x2_, int y2_) {
    if (xMotor == nullptr || yMotor == nullptr) {
        ITM_print("Atleast one motor not initalised! exiting value initialisation\n");
        return;
    }
    xMotor->writeDirection(x2_ > x1_);
    yMotor->writeDirection(y2_ > y1_);
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

void Plotter::isrFunction(portBASE_TYPE& xHigherPriorityWoken ) {
    bresenham();
    if (count > steps) {
        stop_polling();
        xSemaphoreGiveFromISR(sbRIT, &xHigherPriorityWoken);
    }
    else {
        start_polling(pps);
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


void Plotter::plotLineAbsolute(int x1_,int y1_, int x2_,int y2_) {
    plotLine(
        x1_,
        y1_,
        x2_ - currentX,
        y2_ - currentY
    );
}

// TODO: since coordinates are given as floats think about error checking
void Plotter::plotLine(int x1_,int y1_, int x2_,int y2_) {
    initValues(x1_,y1_, x2_,y2_);
    start_polling(pps);
    xSemaphoreTake(sbRIT, portMAX_DELAY);
}

void Plotter::start_polling(int pps_) {
    pps_ = pps_ * savePlottingSpeed / 100;
    Chip_RIT_Disable(LPC_RITIMER);
    uint64_t cmp_value = (uint64_t) Chip_Clock_GetSystemClockRate() / pps_;
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
	int minDuty = ticksPerSecond / 1000; // 1ms
	int maxDuty = ticksPerSecond / 500;  // 2ms
	int temp = value * (maxDuty-minDuty) / 255 + minDuty;
	int dutycycle = temp;
    LPC_SCT0->MATCHREL[1].U = dutycycle;
    LPC_SCT0->OUT[0].SET = 1;
	ITM_print("duty = %d\n", dutycycle);
}

void Plotter::initPen() {
    Chip_SCT_Init(LPC_SCT0);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
	#if defined(BOARD_NXP_LPCXPRESSO_1549)
	Chip_SWM_MovablePortPinAssign(SWM_SCT0_OUT0_O, 0, 10);
	#endif
	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);
    LPC_SCT0->CONFIG |= SCT_CONFIG_32BIT_COUNTER | SCT_CONFIG_AUTOLIMIT_L;
    //LPC_SCT0->CTRL_U |= (SystemCoreClock / ticksPerSecond) << 5;  // set prescaler, SCTimer/PWM clock = 1 MHz
    LPC_SCT0->CTRL_U = SCT_CTRL_PRE_L(SystemCoreClock / ticksPerSecond - 1) | SCT_CTRL_CLRCTR_L | SCT_CTRL_HALT_L;
    LPC_SCT0->MATCHREL[0].U = ticksPerSecond / penFrequency - 1;
    setPenValue(160);
	LPC_SCT0->EVENT[0].STATE = 0x1;         // event 0 happens in all states
    LPC_SCT0->EVENT[1].STATE = 0x1;         // event 1 happens in all st
    LPC_SCT0->EVENT[0].CTRL = (0 << 0) | (1 << 12); // match 0 condition only
    LPC_SCT0->EVENT[1].CTRL = (1 << 0) | (1 << 12); // match 1 condition only
    LPC_SCT0->OUT[0].SET = (1 << 0);                // event 0 will set SCTx_OUT0
    LPC_SCT0->OUT[0].CLR = (1 << 1);                // event 1 will clear SCTx_OUT0
    LPC_SCT0->CTRL_L &= ~(1 << 2);                  // unhalt it by clearing bit 2 of CTRL reg
}

void Plotter::initLaser() {
    Chip_SCT_Init(LPC_SCT2);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_SWM);
	#if defined(BOARD_NXP_LPCXPRESSO_1549)
	Chip_SWM_MovablePortPinAssign(SWM_SCT2_OUT0_O, 0, 12);
	#endif
	Chip_Clock_DisablePeriphClock(SYSCTL_CLOCK_SWM);
    LPC_SCT1->CONFIG |= SCT_CONFIG_32BIT_COUNTER | SCT_CONFIG_AUTOLIMIT_L;
    LPC_SCT1->CTRL_U = SCT_CTRL_PRE_L(SystemCoreClock / ticksPerSecond - 1) | SCT_CTRL_CLRCTR_L | SCT_CTRL_HALT_L;
    LPC_SCT1->MATCHREL[0].U = 255; // Set the laser low
	LPC_SCT1->EVENT[0].STATE = 0x1;         // event 0 happens in all states
    LPC_SCT1->EVENT[1].STATE = 0x1;         // event 1 happens in all st
    LPC_SCT1->EVENT[0].CTRL = (0 << 0) | (1 << 12); // match 0 condition only
    LPC_SCT1->EVENT[1].CTRL = (1 << 0) | (1 << 12); // match 1 condition only
    LPC_SCT1->OUT[0].SET = (1 << 0);                // event 0 will set SCTx_OUT0
    LPC_SCT1->OUT[0].CLR = (1 << 1);                // event 1 will clear SCTx_OUT0
    LPC_SCT1->CTRL_L &= ~(1 << 2);                  // unhalt it by clearing bit 2 of CTRL reg
}

void Plotter::handleGcodeData(const Gcode::Data &data) {
    switch (data.id) {
        case Gcode::Id::G1:
        case Gcode::Id::G28:
            mDraw_print("%f, %f, %d",
                        data.data.g1.moveX,
                        data.data.g1.moveY,
                        data.data.g1.relative
                       );
            if (data.data.g1.relative) {
                plotLine(
                    0,0,
                    (int)data.data.g1.moveX, (int)data.data.g1.moveY
                );
            }
            else {
                plotLineAbsolute(
                    0,0,
                    (int)data.data.g1.moveX, (int)data.data.g1.moveY
                );
            }
            break;

        case Gcode::Id::M1:
            mDraw_print("%u", data.data.m1.penPos);
            setPenValue(data.data.m1.penPos);
            break;

        case Gcode::Id::M2:
            mDraw_print("%u, %u", data.data.m2.savePenUp, data.data.m2.savePenDown);
            savePenUp   = data.data.m2.savePenUp;
            savePenDown = data.data.m2.savePenDown;
            break;

        case Gcode::Id::M4:
            // TODO: create function for setting laser power
            mDraw_print("%u", data.data.m4.laserPower);
            break;

        case Gcode::Id::M5:
            mDraw_print("%d, %d, %u, %u, %u",
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
            break;
        case Gcode::Id::M10:
        	do {
            char buffer[64];
            snprintf(buffer, 64, Gcode::toFormat((Gcode::Id)CREATE_GCODE_ID('M', 10)),
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
            snprintf(buffer, 32, Gcode::toFormat((Gcode::Id)CREATE_GCODE_ID('M', 11)),
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
