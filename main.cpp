#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

# include <stdio.h>

#include "main.h"
#include "parser/parser.h"
#include <cstdlib>
#include "FreeRTOS.h"
#include "task.h"
#include "heap_lock_monitor.h"
#include "syslog.h"
#include "ITM_write.h"
#include "printer.h"

//#define READ_FROM_FILE_TEST

#ifdef READ_FROM_FILE_TEST
#define LINE_SIZE 128
#endif


#include <cr_section_macros.h>
/* VARIABLES */

static void vTask1(void *pvParameters) {
	ITM_print("test %d", 1);
    char buffer[128]="";
    int c;
    int index = 0;
    while (1) {
    	c=mDraw_uart.read();
        if (c == EOF) continue;
        ITM_print("%c",c);
        mDraw_uart.write(c);

        if(index < 128 - 1) {
            buffer[index] = c;
            index++;
        }

        if (c == '\n' || c == '\r') {
            buffer[index] = '\0';
            ITM_write(buffer);
            parseCode(buffer);
            index = 0;
            buffer[0] = '\0';
            mDraw_print("OK\r\n");
        }
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
    ITM_init();
	prvSetupHardware();
	ITM_print("test\n");

#ifdef READ_FROM_FILE_TEST
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

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

