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
#define BIT_0	( 1 << 0 )

#if READ_FROM_FILE_TEST == 1
#define LINE_SIZE 128
#endif

/* VARIABLES */
static EventGroupHandle_t eventBit;
static QueueHandle_t queue;
static SemaphoreHandle_t* initVariablesSemaphore;
static TaskHandle_t* initVariablesHandle;
static Motor* xMotor;
static Motor* yMotor;
static Plotter* plotter;

static DigitalIoPin* limx1;
static DigitalIoPin* limx2;
static DigitalIoPin* limy1;
static DigitalIoPin* limy2;
static DigitalIoPin* xStep;
static DigitalIoPin* xDirection;
static DigitalIoPin* yStep;
static DigitalIoPin* yDirection;

#define BUFFER_SIZE 128
#define STR_SIZE 64
static void vTask1(void *pvParameters) {
	vTaskDelay(100); /* wait until semaphores are created */
	while ((xEventGroupWaitBits(eventBit, BIT_0,
	pdTRUE,
	pdTRUE,
	portMAX_DELAY) & (BIT_0)) != BIT_0) {
	}
	ITM_print("task1\n");
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
	vTaskDelay(100); /* wait just in case */
	while ((xEventGroupWaitBits(eventBit, BIT_0,
	pdTRUE,
	pdTRUE,
	portMAX_DELAY) & (BIT_0)) != BIT_0) {
	}
	ITM_print("task2\n");
	Gcode::Data data;
	plotter->initPen();
	plotter->initLaser();
	plotter->calibrate();

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

static void vTask3(void *pvParameters) {
	xDirection->write(CLOCKWISE);
	yDirection->write(CLOCKWISE);
	// TODO check the limit switches here which corresponds to which axis

	while (limx1->read() && limx2->read() && limy1->read() && limy2->read())
		;

	while (!limx1->read() || !limx2->read()) {
		xStep->write(true);
		vTaskDelay(1);
		xStep->write(false);
		if (limx1->read()) {
			xMotor = new Motor( { xStep, xDirection, limx1, limx2,
			CLOCKWISE });
			break;
		} else if (limx2->read()) {
			xMotor = new Motor( { xStep, xDirection, limx2, limx1,
			CLOCKWISE });
			break;
		}
	}

	while (!limy1->read() || !limy2->read()) {
		yStep->write(true);
		vTaskDelay(1);
		yStep->write(false);
		if (limy1->read()) {
			yMotor = new Motor( { yStep, yDirection, limy1, limy2,
			CLOCKWISE });
			break;
		} else if (limx2->read()) {
			yMotor = new Motor( { yStep, yDirection, limy2, limy1,
			CLOCKWISE });
			break;
		}
	}
	plotter = new Plotter(xMotor, yMotor);
	Plotter::activePlotter = plotter;
	xEventGroupSetBits(eventBit, BIT_0);
	vTaskSuspend(*initVariablesHandle);

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
	initVariablesSemaphore = new SemaphoreHandle_t;
	initVariablesHandle = new TaskHandle_t;
	eventBit = xEventGroupCreate();
	queue = xQueueCreate(5, sizeof(Gcode::Data));
	limx1 = new DigitalIoPin(0, 9, DigitalIoPin::pullup, true);
	limx2 = new DigitalIoPin(0, 29, DigitalIoPin::pullup, true);
	limy1 = new DigitalIoPin(0, 0, DigitalIoPin::pullup, true);
	limy2 = new DigitalIoPin(1, 3, DigitalIoPin::pullup, true);

	xStep = new DigitalIoPin(0, 24, DigitalIoPin::output, true);
	xDirection = new DigitalIoPin(1, 0, DigitalIoPin::output, true);

	yStep = new DigitalIoPin(0, 27, DigitalIoPin::output, true);
	yDirection = new DigitalIoPin(0, 28, DigitalIoPin::output, true);

	Chip_RIT_Init(LPC_RITIMER);
	Chip_RIT_Disable(LPC_RITIMER);
	NVIC_SetPriority(RITIMER_IRQn,
	configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
	ITM_print("Starting\n");

	xTaskCreate(cdc_task, "CDC",
	configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(vTask1, "parser",
	configMINIMAL_STACK_SIZE + 512, NULL, (tskIDLE_PRIORITY + 1UL),
			(TaskHandle_t *) NULL);
	xTaskCreate(vTask2, "motor",
	configMINIMAL_STACK_SIZE + 512, NULL, (tskIDLE_PRIORITY + 2UL),
			(TaskHandle_t *) NULL);

	xTaskCreate(vTask3, "initVariables",
	configMINIMAL_STACK_SIZE + 512, NULL, (tskIDLE_PRIORITY + 2UL),
			initVariablesHandle);

	vTaskStartScheduler();

#if READ_FROM_FILE_TEST == 1
	// TODO: what is the current working directory in mcu?
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

