#if defined (__USE_LPCOPEN)
#if defined(NO_BOARD_LIB)
#include "chip.h"
#else
#include "board.h"
#endif
#endif

#include "parser.h"
//#define TESTING

#ifdef TESTING
#include <iostream>
#include <fstream>
#endif 



#include <cr_section_macros.h>
/* Sets up system hardware */
static void prvSetupHardware(void) {
	SystemCoreClockUpdate();
	Board_Init();

	/* Initial LED0 state is off */
	Board_LED_Set(0, false);
}

int main() {
	prvSetupHardware();

#ifdef TESTING
	//const char *s = "M2 U150 D90";
	//const char *s = "M11 -1 2 3 4";
	//const char *s = "M1 90"; 
	//const char *s = "M10";
	//const char *s = "M5 A0 B0 H310 W380 S80";
	//const char *s = "M4 140";
	//const char *s = "G1 X85.14 Y117.29 A0";
	//parseCode("M10");
	//parseCode("M5 A0 B0 H310 W380 S80");
	//parseCode("M2 U150 D90");
	//parseCode("M10");
	//parseCode("G28");

    //std::ifstream input("gcode01.txt");
	//std::string str;
	//while (getline(input, str)) {
    //    parseCode(str);
    //}
#else

#endif /* TESTING */
	return 0;
}
