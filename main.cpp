#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

//#define TESTING

#include "parser/parser.h"
#include <cstdlib>
#include "FreeRTOS.h"
#include "task.h"
#include "Syslog.h"
#include "heap_lock_monitor.h"

#ifdef TESTING
//TODO: change to c style file handling 
//so we have enough space for flash
//#include <iostream>
//#include <fstream>

#else
#include "ITM_write.h"
#endif 

#include <cr_section_macros.h>

/* Sets up system hardware */
static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

Syslog log = Syslog();

static void vTask1(void *pvParameters) {
    //ITM_init();
    char buffer[128]="";
    int c;
    int index = 0;
    while (1) {
        //c = Board_UARTGetChar();
    	c=log.read();
        if (c == EOF) continue;
        //ITM_print("%c",c);

        if(index < 128 - 1) {
            buffer[index] = c;
            index++;
        }

        if (c == '\n' || c == '\r') {
        	//ITM_write("test\n\n");
            buffer[index] = '\0';
            ITM_write(buffer);
            parseCode(buffer);
            index = 0;
            buffer[0] = '\0';
            //Board_UARTPutSTR("OK\r\n");
            log.writeString("OK\r\n");
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
	prvSetupHardware();

	xTaskCreate(vTask1, "vTask1",
				configMINIMAL_STACK_SIZE+512, NULL, (tskIDLE_PRIORITY + 1UL),
				(TaskHandle_t *) NULL);

	vTaskStartScheduler();





#ifdef TESTING
	//const char *s = "M2 U150 D90";
	//const char *s = "M11 -1 2 3 4";
	//const char *s = "M1 90"; 
	//const char *s = "M10";
	//const char *s = "M5 A0 B0 H310 W380 S80";
	//const char *s = "M4 140";
	//const char *s = "G1 X85.14 Y117.29 A0";
	parseCode("M10");
	parseCode("M5 A0 B0 H310 W380 S80");
	parseCode("M2 U150 D90");
	parseCode("M10");
	parseCode("G28");

    //std::ifstream input("parser/gcode01.txt");
	//char *str;
	//while (getline(input, str)) {
    //    parseCode(str);
    //}
#else



#endif /* TESTING */
	return 0;
}

