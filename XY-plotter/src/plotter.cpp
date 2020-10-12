#include "plotter.h"
#include "printer.h"
#include <algorithm>

Plotter* Plotter::activePlotter = nullptr;
Plotter::Plotter(Motor* xMotor, Motor* yMotor) :
    xMotor(xMotor),
    yMotor(yMotor)
    {
    	sbRIT = xSemaphoreCreateBinary();
}

void Plotter::calibrate() {
    while(
            xMotor->limMax.read() ||
            xMotor->limOrigin.read() ||
            yMotor->limMax.read() ||
            yMotor->limOrigin.read()
         );
//    xMotor->direction.write((xMotor->originDirection));
//    yMotor->direction.write((yMotor->originDirection));
//    while (xMotor->limOrigin.read() == false) {
//        xMotor->motor.write(true);
//        xMotor->motor.write(false);
//    }
//    while (yMotor->limOrigin.read() == false) {
//        yMotor->motor.write(true);
//        yMotor->motor.write(false);
//    }
//    xMotor->direction.write((!xMotor->originDirection));
//    yMotor->direction.write((!yMotor->originDirection));
//    xMotor->motor.write(true);
//    xMotor->motor.write(false);
//    yMotor->motor.write(true);
//    yMotor->motor.write(false);
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

void Plotter::initValues(int x1_, int y1_, int x2_, int y2_) {
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


void Plotter::plotLineAbsolute(int x1_,int y1_, int x2_,int y2_, int pps_) {
    plotLine(
        x1_,
        y1_,
        x2_ - currentX,
        y2_ - currentY,
        pps_
    );
}

// TODO: since coordinates are given as floats think about error checking
void Plotter::plotLine(int x1_,int y1_, int x2_,int y2_, int pps_) {
    initValues(x1_,y1_, x2_,y2_);
    pps = pps_;
    start_polling(pps);
    xSemaphoreTake(sbRIT, portMAX_DELAY);
}

void Plotter::start_polling(int pps_) {
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
	int ticksPerSecond = 1'000'000;
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
	int ticksPerSecond = 1'000'000;
	int frequency = 50;
    LPC_SCT0->CONFIG |= SCT_CONFIG_32BIT_COUNTER | SCT_CONFIG_AUTOLIMIT_L;
    //LPC_SCT0->CTRL_U |= (SystemCoreClock / ticksPerSecond) << 5;  // set prescaler, SCTimer/PWM clock = 1 MHz
    LPC_SCT0->CTRL_U = SCT_CTRL_PRE_L(SystemCoreClock / ticksPerSecond - 1) | SCT_CTRL_CLRCTR_L | SCT_CTRL_HALT_L;
    LPC_SCT0->MATCHREL[0].U = ticksPerSecond / frequency - 1;
    setPenValue(160);
	LPC_SCT0->EVENT[0].STATE = 0x1;         // event 0 happens in all states
    LPC_SCT0->EVENT[1].STATE = 0x1;         // event 1 happens in all st
    LPC_SCT0->EVENT[0].CTRL = (0 << 0) | (1 << 12); // match 0 condition only
    LPC_SCT0->EVENT[1].CTRL = (1 << 0) | (1 << 12); // match 1 condition only
    LPC_SCT0->OUT[0].SET = (1 << 0);                // event 0 will set SCTx_OUT0
    LPC_SCT0->OUT[0].CLR = (1 << 1);                // event 1 will clear SCTx_OUT0
    LPC_SCT0->CTRL_L &= ~(1 << 2);                  // unhalt it by clearing bit 2 of CTRL reg
}

