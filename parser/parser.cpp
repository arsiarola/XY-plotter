#include <stdio.h>
#include <string.h>
#include "Gcode.h"
#include "parser.h"
#include "../ITM_write.h"

//#define _CRT_SECURE_NO_WARNINGS
//#pragma warning(disable:4996)

#define GCODE_SIZE 8

//M2
int savePenUp = 0;
int savePenDown = 0;
//M11
int L4 = 0;
int L3 = 0;
int L2 = 0;
int L1 = 0;
//M1
int penPos = 0;
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
	//ITM_write("Test1\n\n");
    char s[128];
    //ITM_write("Test2\n\n");
    strncpy(s, str, 128);
    //ITM_write("Test3\n\n");
	char *token = strchr(s, ' ');
	//ITM_write("Test4\n\n");
	if(token == NULL){
		//std::cout << "no space found!" << std::endl;
		ITM_write("No space found\n");

	}
	else{
		s[token-s] = '\0';
	}

	ITM_print("Token: %s\n",s);
	ITM_print("Gcode: %s",G1.getGcode());

	if (token == M2.getGcode()) {

		if (sscanf(
			s,
			M2.getFormat(),
			&savePenUp,
			&savePenDown) >= 2) {
			//std::cout << "M2: " << std::endl;
			//std::cout << savePenUp << std::endl;
			//std::cout << savePenDown << std::endl;
			//ITM_write("M2\n");
		}
	}

	else if (token == M11.getGcode()) {
		// if (sscanf(
		// 	s,
		// 	M11.getFormat(),
		// 	&L4,
		// 	&L3,
		// 	&L2,
		// 	&L1) >= 4) {
		// 	//std::cout << "M11: " << std::endl;
		// 	//std::cout << L4 << std::endl;
		// 	//std::cout << L3 << std::endl;
		// 	//std::cout << L2 << std::endl;
		// 	//std::cout << L1 << std::endl;
		// }


		//std::cout << "Limits to be added" << std::endl;
		//ITM_write("M11\n");
	}

	else if (token == M1.getGcode()) {

		if (sscanf(
			s,
			M1.getFormat(),
			&penPos) >= 1) {
			//std::cout << "M1: " << std::endl;
			//std::cout << penPos << std::endl;
			//ITM_write("M1\n");
		}
	}

	else if (token == M10.getGcode()) {
		//std::cout << token << " XY ";
			//std::cout height << " " << width << " 0.00 0.00 A" << dirX << " B" << dirY << " H0 ";
			//std::cout speed << " U" << savePenUp << " D" << savePenDown << std::endl;
		//ITM_write("M10\n");
	}

	else if (token == M5.getGcode()) {

		if (sscanf(
			s,
			M5.getFormat(),
			&dirX,
			&dirY,
			&height,
			&width,
			&speed) >= 5) {
			//std::cout << "M5: " << std::endl;
			//std::cout << dirX << std::endl;
			//std::cout << dirY << std::endl;
			//std::cout << height << std::endl;
			//std::cout << width << std::endl;
			//std::cout << speed << std::endl;
			//ITM_write("M5\n");
		}
	}

	else if (token == M4.getGcode()) {
		int laserPower;
		if (sscanf(
			s,
			M4.getFormat(),
			&laserPower) >= 1) {
			//std::cout << laserPower << std::endl;
			//ITM_write("M4\n");
		}
	}

	else if (token == G28.getGcode()) {
		//std::cout << "Move to origo" << std::endl;
		//ITM_write("G28\n");
	}

	//else if (s == G1.getGcode()) {
	else if (strcmp(s,G1.getGcode())==0) {

		if (sscanf(
			s,
			G1.getFormat(),
			&moveX,
			&moveY,
			&moveA) >= 3) {
			//std::cout << moveX << std::endl;
			//std::cout << moveY << std::endl;
			//std::cout << moveA << std::endl;

		}
		ITM_write("G1\n");
	}
	else
	{
		//std::cout << "Error! " << s << std::endl;
		//ITM_write("Error!\n");
	}

	ITM_write("Parser done\n");


	//system("pause");
}





