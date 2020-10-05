

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

#define READ_FROM_FILE_TEST 0

#if READ_FROM_FILE_TEST == 1
#define LINE_SIZE 128
#endif


/* VARIABLES */
QueueHandle_t queue;


#define BUFFER_SIZE 128
#define STR_SIZE 80

static void vTask1(void *pvParameters) {
	vTaskDelay(100); /* wait until semaphores are created */
    char buffer[BUFFER_SIZE] = "";
    char str[STR_SIZE] = "";
    int length = 0;
    ITM_print("starting while\n");
    while (1) {
        //received = mDraw_uart->read(buffer, 128, portTICK_PERIOD_MS * 100);
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
            USB_send((uint8_t *) "OK\r\n", 4);
        }
        /* vTaskDelay(10); */
    }
}

static void vTask2(void *pvParameters) {
	GcodeData data;
	while (true) {
		if (xQueueReceive(
                queue,
                &data,
                portMAX_DELAY
             	 ) == pdTRUE ) {
			mDraw_print("got something\n\r");
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
	queue = xQueueCreate(5, sizeof(GcodeData));
    ITM_init();
    prvSetupHardware();
    ITM_print("test\n");

#if READ_FROM_FILE_TEST == 1
    // TODO: what is the current working directory in mcu?
    FILE *fp;
    const char *fname = "parser/gcode01.txt";
    fp = fopen(fname, "r");
    if (fp == NULL) ITM_print("Error: cannot open %s for reading\n", fname);
    else {char line[LINE_SIZE];
        while(!feof(fp)) {
            if (fgets(line, LINE_SIZE, fp) != NULL) {
                ITM_print(line);
                /* parseCode(line); */
            }
        }
        fclose(fp);
    }

#else

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

