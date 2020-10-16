#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include <cstdlib>
#include <cr_section_macros.h>
#include <stdio.h>
#include <string.h>

#include "main.h"
#include "parser/parser.h"
#include "ITM_write.h"
#include "printer.h"
#include "parser/Gcode.h"
#include "usb/user_vcom.h"
#include "motor.h"
#include "plotter.h"

// Freertos API includes
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "heap_lock_monitor.h"
#include "event_groups.h"

#define READ_FROM_FILE_TEST 0
#define INIT_VARIABLE_BIT	( 1 << 0 )

#if READ_FROM_FILE_TEST == 1
#define LINE_SIZE 128
#endif

/* VARIABLES */
static EventGroupHandle_t eventBit;
static QueueHandle_t queue;
static TaskHandle_t initVariablesHandle;
static SemaphoreHandle_t initVariablesSemaphore;
static Motor* xMotor;
static Motor* yMotor;
static Plotter* plotter;

DigitalIoPin* lim1;
DigitalIoPin* lim2;
DigitalIoPin* lim3;
DigitalIoPin* lim4;

static DigitalIoPin* xStep;
static DigitalIoPin* xDirection;
static DigitalIoPin* yStep;
static DigitalIoPin* yDirection;

#define BUFFER_SIZE 128
#define STR_SIZE 64
static void vTask1(void *pvParameters) {
	vTaskDelay(configTICK_RATE_HZ / 10); /* wait until semaphores are created */
	xEventGroupWaitBits(
	    			eventBit,
					INIT_VARIABLE_BIT,
	                pdFALSE,
	                pdTRUE,
	                portMAX_DELAY);

	char buffer[BUFFER_SIZE] = "";
	char str[STR_SIZE] = "";
	int bufferLength = 0;
	int strLength = 0;
	bool endLine = false;
	while (1) {
		uint32_t received = USB_receive((uint8_t *) str + strLength,
		STR_SIZE - strLength - 1);
		if (received > 0) {
			str[strLength + received] = 0; /* make sure we have a null at the end */
			strLength += received;
			/* ITM_print("str=%s,strLen=%d		buf=%s,bufLen=%d\n", str, strLength, buffer, bufferLength); */
			endLine = (strchr(str, '\n') != NULL || strchr(str, '\r') != NULL
					|| bufferLength >= BUFFER_SIZE - 1);
			if (endLine || strLength >= STR_SIZE - 1) {
				strncat(buffer + bufferLength, str,
				BUFFER_SIZE - bufferLength - 1);
				bufferLength =
						bufferLength + strLength >= BUFFER_SIZE - 1 ?
								BUFFER_SIZE - 1 : bufferLength + strLength;
				strLength = 0;
				str[0] = '\0';
			}
			//ITM_print("str=%s, bufLen=%d, strLen=%d\n", str, bufferLength, strLength);
			if (endLine) {
				ITM_print("%s", buffer);
				parseCode(buffer, queue);
				bufferLength = 0;
				buffer[0] = '\0';
				strLength = 0;
				str[0] = '\0';
			}
		}
	}
}

static void vTask2(void *pvParameters) {
	vTaskDelay(configTICK_RATE_HZ / 10); /* wait just in case */
    xEventGroupWaitBits(
    			eventBit,
				INIT_VARIABLE_BIT,
                pdFALSE,
                pdTRUE,
                portMAX_DELAY);

	Gcode::Data data;
	while (true) {
		if (xQueueReceive(queue, &data,
		portMAX_DELAY) == pdTRUE) {
			UART_print("ID: %s\n\rValues: ", Gcode::toString(data.id).data());
			UART_print("\r\n");
			plotter->handleGcodeData(data);
			USB_send((uint8_t *) "OK\r\n", 4);
		}
	}
}

DigitalIoPin* getCorrespondingLimit(DigitalIoPin* step, DigitalIoPin* direction, bool dir) {
    direction->write(dir);
    DigitalIoPin* ret;
	while (1) {
        if     (lim1->read()) { ret = lim1; break; }
        else if(lim2->read()) { ret = lim2; break; }
        else if(lim3->read()) { ret = lim3; break; }
        else if(lim4->read()) { ret = lim4; break; }
		step->write(true);
		vTaskDelay(1);
		step->write(false);
		vTaskDelay(1);
	}

    // Why an earth do we need to move steps back for the y axis
    direction->write(!dir);
    step->write(true);
    vTaskDelay(1);
    step->write(false);
    vTaskDelay(1);
    return ret;
}

static void vTask3(void *pvParameters) {
	lim1 = new DigitalIoPin(0, 9, DigitalIoPin::pullup, true);
	lim2 = new DigitalIoPin(0, 29, DigitalIoPin::pullup, true);
	lim3 = new DigitalIoPin(0, 0, DigitalIoPin::pullup, true);
	lim4 = new DigitalIoPin(1, 3, DigitalIoPin::pullup, true);

	xStep = new DigitalIoPin(0, 24, DigitalIoPin::output, true);
	xDirection = new DigitalIoPin(1, 0, DigitalIoPin::output, true);

	yStep = new DigitalIoPin(0, 27, DigitalIoPin::output, true);
	yDirection = new DigitalIoPin(0, 28, DigitalIoPin::output, true);

	while (lim1->read() || lim2->read() || lim3->read() || lim4->read()) {}
    DigitalIoPin* yLimMin = getCorrespondingLimit(yStep, yDirection, CLOCKWISE);
    DigitalIoPin* yLimMax = getCorrespondingLimit(yStep, yDirection, COUNTER_CLOCKWISE);

    DigitalIoPin* xLimMin = getCorrespondingLimit(xStep, xDirection, CLOCKWISE);
    DigitalIoPin* xLimMax = getCorrespondingLimit(xStep, xDirection, COUNTER_CLOCKWISE);

    xMotor = new Motor (xStep, xDirection, xLimMin, xLimMax, CLOCKWISE);
    yMotor = new Motor (yStep, yDirection, yLimMin, yLimMax, CLOCKWISE);
	plotter = new Plotter(xMotor, yMotor);
	Plotter::activePlotter = plotter;
	plotter->initPen();
	plotter->initLaser();
	plotter->calibrate();

	xEventGroupSetBits(eventBit, INIT_VARIABLE_BIT);
	vTaskSuspend(initVariablesHandle);
}

extern "C" {
void vConfigureTimerForRunTimeStats(void) {
	Chip_SCT_Init(LPC_SCTSMALL1);
	LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
	LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
}
}

int main() {
	ITM_init();
	prvSetupHardware();
    // Initalise only freertos variables in main

	eventBit = xEventGroupCreate();
	queue = xQueueCreate(5, sizeof(Gcode::Data));

	Chip_RIT_Init(LPC_RITIMER);
	Chip_RIT_Disable(LPC_RITIMER);
	NVIC_SetPriority(RITIMER_IRQn,
	configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	ITM_print("Starting\n");

	xTaskCreate(cdc_task, "CDC",
	configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vTask1, "parser",
	configMINIMAL_STACK_SIZE + 512, NULL, (tskIDLE_PRIORITY + 1UL), (TaskHandle_t *) NULL);

	xTaskCreate(vTask2, "motor",
	configMINIMAL_STACK_SIZE + 512, NULL, (tskIDLE_PRIORITY + 2UL), (TaskHandle_t *) NULL);

	xTaskCreate(vTask3, "initVariables",
	configMINIMAL_STACK_SIZE + 512, NULL, (tskIDLE_PRIORITY + 2UL), &initVariablesHandle);

	vTaskStartScheduler();

#if READ_FROM_FILE_TEST == 1
	// TODO: what is the current working directory in mcu? Cant find the file no matter what
	FILE *fp;
	const char *fname = "parser/gcode01.txt";
	fp = fopen(fname, "r");
	if (fp == NULL) ITM_print("Error: cannot open %s for reading\n", fname);
	else {
		char line[LINE_SIZE];
		while(!feof(fp)) {
			if (fgets(line, LINE_SIZE, fp) != NULL) {
				ITM_print(line);
				/* parseCode(line); */
			}
		}
		fclose(fp);
	}
#endif /* READ_FROM_FILE_TEST */

	delete xMotor;
	delete yMotor;
	delete plotter;
	delete lim1;
	delete lim2;
	delete lim3;
	delete lim4;
	delete xStep;
	delete xDirection;
	delete yStep;
	delete yDirection;

	return 1;
}

/* Sets up system hardware */
void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();
	Chip_PININT_Init(LPC_GPIO_PIN_INT);
	Chip_Clock_EnablePeriphClock(SYSCTL_CLOCK_PININT);
	Chip_SYSCTL_PeriphReset(RESET_PININT);

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

