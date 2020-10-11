
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
#include "FreeRTOS.h"
#include "parser/parser.h"
#include "task.h"
#include "heap_lock_monitor.h"
#include "ITM_write.h"
#include "printer.h"
#include "parser/Gcode.h"
#include "usb/user_vcom.h"
#include "motor.h"
#include "plotter.h"

#define READ_FROM_FILE_TEST 0

#if READ_FROM_FILE_TEST == 1
#define LINE_SIZE 128
#endif


/* VARIABLES */
static QueueHandle_t queue;
static Motor* xMotor;
static Motor* yMotor;

#define BUFFER_SIZE 128
#define STR_SIZE 80
static void vTask1(void *pvParameters) {
	vTaskDelay(100); /* wait until semaphores are created */
    char buffer[BUFFER_SIZE] = "";
    char str[STR_SIZE] = "";
    int length = 0;
    while (1) {
		uint32_t received = USB_receive((uint8_t *)str, STR_SIZE-1);

        if (received > 0) {
			str[received] = 0; /* make sure we have a zero at the end */
            length += received;
            strncat(buffer, str, BUFFER_SIZE);
            if (strchr(str, '\n') == NULL && strchr(str, '\r') == NULL && length < 128-1) continue;
            if (length > BUFFER_SIZE-1) length = BUFFER_SIZE-1;
            buffer[length] = '\0';
            ITM_write(buffer);
            parseCode(buffer, queue);
            length = 0;
			buffer[0] = '\0';
        }
    }
}

static void vTask2(void *pvParameters) {
    Gcode::Data data;
    Plotter::calibrate();
	while (true) {
		if (xQueueReceive(
                queue,
                &data,
                portMAX_DELAY
             	 ) == pdTRUE ) {
			mDraw_print("ID: %s\n\rValues: ", Gcode::toString(data.id).data());
            switch (data.id) {
                case Gcode::Id::G1:
                case Gcode::Id::G28:
                    mDraw_print("%f, %f, %d",
                            data.data.g1.moveX,
                            data.data.g1.moveY,
                            data.data.g1.relative
                            );
                    Plotter::plotLineAbsolute(
                            0,0,
                            (int)data.data.g1.moveX, (int)data.data.g1.moveY,
							1000
                        );
                    break;
                case Gcode::Id::M1:
                    mDraw_print("%u", data.data.m1.penPos);
                    break;
                case Gcode::Id::M2:
                    mDraw_print("%u, %u", data.data.m2.savePenUp, data.data.m2.savePenDown);
                    break;
                case Gcode::Id::M4:
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
                    break;
                case Gcode::Id::M10:
                    break;
            }
            mDraw_print("\r\n");
            USB_send((uint8_t *) "OK\r\n", 4);
		}
	}
}

extern "C" {
    void vConfigureTimerForRunTimeStats( void ) {
        Chip_SCT_Init(LPC_SCTSMALL1);
        LPC_SCTSMALL1->CONFIG = SCT_CONFIG_32BIT_COUNTER;
        LPC_SCTSMALL1->CTRL_U = SCT_CTRL_PRE_L(255) | SCT_CTRL_CLRCTR_L; // set prescaler to 256 (255 + 1), and start timer
    }
}

int main() {
	queue = xQueueCreate(5, sizeof(Gcode::Data));
    ITM_init();
    prvSetupHardware();
xMotor = new Motor({
        { 0, 24, DigitalIoPin::output, true},
        { 1, 0,  DigitalIoPin::output, true},
        { 0, 9,  DigitalIoPin::pullup, true},
        { 0, 29, DigitalIoPin::pullup, true},
        false
});

yMotor = new Motor({
        { 0, 27, DigitalIoPin::output, true},
        { 0, 28, DigitalIoPin::output, true},
        { 1, 3,  DigitalIoPin::pullup, true},
        { 0, 0,  DigitalIoPin::pullup, true},
        false
});


    Chip_RIT_Init(LPC_RITIMER);
    Chip_RIT_Disable(LPC_RITIMER);
    NVIC_SetPriority(RITIMER_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    ITM_print("Starting\n");
    Plotter::setMotors(xMotor, yMotor);

    xTaskCreate(cdc_task, "CDC",
				configMINIMAL_STACK_SIZE + 128, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

    xTaskCreate(vTask1, "parser",
            configMINIMAL_STACK_SIZE+512, NULL, (tskIDLE_PRIORITY + 1UL),
            (TaskHandle_t *) NULL);
    xTaskCreate(vTask2, "motor",
                configMINIMAL_STACK_SIZE+512, NULL, (tskIDLE_PRIORITY + 1UL),
                (TaskHandle_t *) NULL);

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

    return 0;
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

