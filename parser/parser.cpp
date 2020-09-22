#include <stdio.h>
#include <string.h>
#include "Gcode.h"
#include "parser.h"
#include "../printer.h"
#include "../ITM_write.h"



//#define _CRT_SECURE_NO_WARNINGS
//#pragma warning(disable:4996)

#define GCODE_SIZE 8


//M1
uint8_t penPos = 0;
//M2
uint8_t savePenUp = 0;
uint8_t savePenDown = 0;
//M11
int L4 = 0;
int L3 = 0;
int L2 = 0;
int L1 = 0;
//M4
uint8_t laserPower;
//M5
int dirX = 0;
int dirY = 0;
int height = 0;
int width = 0;
int speed = 0;
//G1
float moveX = 0;
float moveY = 0;
int moveA = 0;







Gcode M2 = Gcode("M2",
	"M2 "
	"U" "%d "
	"D" "%d"
);
Gcode M10 = Gcode("M10",
	"M10"
);
Gcode M11 = Gcode("M11",
	"M11 "
	// "%d "
	// "%d "
	// "%d "
	// "%d"
);
Gcode M1 = Gcode("M1",
	"M1 "
	"%d"
);

Gcode M5 = Gcode("M5",
	"M5 "
	"A" "%d " //A0
	"B" "%d " //B0
	"H" "%d " //H310
	"W" "%d " //W380
	"S" "%d" //S80
);

Gcode M4 = Gcode("M4",
	"M4 "
	"%d"
);

Gcode G28 = Gcode("G28",
	"G28 "
);

Gcode G1 = Gcode("G1",
	"G1 "
	"X" "%f " //X85.14
	"Y" "%f " //Y117.29
	"A" "%d" //A0
);

//static Gcode *gcodes[GCODE_SIZE] = {
//	&M2,
//	&M10,
//	&M11,
//	&M1,
//	&M5,
//	&M4,
//	&G28,
//	&G1
//};

void parseCode(const char *str) {

    char gcode[128];
    strncpy(gcode, str, 128);

    trimTrailing(gcode);

	char *token = strchr(gcode, ' ');
	if(token != NULL){
		gcode[token-gcode] = '\0';
	}

	/*M1: Set pen position*/
	if (strcmp(gcode, M1.getGcode()) == 0) {
		if (sscanf(
			str,
			M1.getFormat(),
			&penPos) == 1) {
			ITM_write("M1: ");
			ITM_print("pen position: %u\n",penPos);
		}
	}

	/*M2: Save pen position*/
	else if (strcmp(gcode, M2.getGcode()) == 0) {
		if (sscanf(
			str,
			M2.getFormat(),
			&savePenUp,
			&savePenDown) == 2) {
			ITM_write("M2: ");
			ITM_print("up: %u  down: %u\n",savePenUp,savePenDown);
		}
	}

	/*M4: Set laser power*/
	else if (strcmp(gcode, M4.getGcode()) == 0) {
		if (sscanf(
			str,
			M4.getFormat(),
			&laserPower) == 1) {
			ITM_write("M4: ");
			ITM_print("Power level of laser: %u\n",laserPower);
		}
	}

	/*M5: Save stepper directions, plot area, and plotting speed*/
	else if (strcmp(gcode, M5.getGcode()) == 0) {
		if (sscanf(
			str,
			M5.getFormat(),
			&dirX,
			&dirY,
			&height,
			&width,
			&speed) == 5) {
			ITM_write("M5: ");
			ITM_print("X direction: %d, Y direction: %d, canvas dimensions: %d x %d, plotting speed: %d\n",dirX,dirY,height,width,speed);
		}
	}


	/*M10: Log of opening a COM port in mDraw*/
	else if (strcmp(gcode, M10.getGcode()) == 0) {
		ITM_write("M10\n");
	}

	/*M11: Limit switch status query*/
	else if (strcmp(gcode, M11.getGcode()) == 0) {
		//Limits to be added
		ITM_write("M11\n");
	}

	/*G1: Move to coordinate*/
	else if (strcmp(gcode ,G1.getGcode()) == 0) {
		if (sscanf(
			str,
			G1.getFormat(),
			&moveX,
			&moveY,
			&moveA) == 3) {
			ITM_write("G1: ");
			ITM_print("Moving to coordinates X %.2f and Y %.2f\n",moveX,moveY);
		}

	}

	/*G1: Move to origin*/
	else if (strcmp(gcode, G28.getGcode()) == 0) {
		ITM_write("G28: Moving to origin\n");
	}

	/*Unknown Gcode*/
	else
	{
		ITM_write("Error!\n");
		ITM_print("%s is unknown Gcode",gcode);
	}

	ITM_write("\n");
}


/*Functions*/

void trimTrailing(char * str)
{
    int index, i;

    /* Set default index */
    index = -1;

    /* Find last index of non-white space character */
    i = 0;
    while(str[i] != '\0')
    {
        if(str[i] != ' ' && str[i] != '\t' && str[i] != '\n')
        {
            index= i;
        }

        i++;
    }

    /* Mark next character to last non-white space character as NULL */
    str[index + 1] = '\0';
}

