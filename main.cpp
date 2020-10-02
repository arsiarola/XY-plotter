

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

#define READ_FROM_FILE_TEST 0

#if READ_FROM_FILE_TEST == 1
#define LINE_SIZE 128
#endif


/* VARIABLES */
QueueHandle_t queue;


#define I2C_BUFFER_SIZE 128
static void vTask1(void *pvParameters) {
	int n = 5;
    char buffer[128] = "";
    int received;
    int length = 0;
    ITM_print("starting while\n");
    mDraw_print("testing Uart\n\r");
    int x = 0;
    while (1) {
    	if (++x > 100) { ITM_print("In while\n"); x = 0; }
        received = mDraw_uart.read(buffer, 128, portTICK_PERIOD_MS * 100);
        if (received > 0) {
            ITM_print("got something\n");
            ITM_print("%s", buffer);
            length += received;

            if (strchr(buffer, '\n') == NULL && strchr(buffer, '\r') == NULL && length < 128-1) continue;
            mDraw_print("OK\r\n");
            length = 0;
        }


        /* if (c == '\n' || c == '\r') { */
        /*     buffer[index] = '\0'; */
        /*     ITM_write(buffer); */
        /*     parseCode(buffer); */
        /*     buffer[0] = '\0'; */
        /*     mDraw_print("OK\r\n"); */
        /* } */
        //vTaskDelay(10);
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

    xTaskCreate(vTask1, "vTask1",
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

