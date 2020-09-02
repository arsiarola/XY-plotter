#include <stdio.h>
#include <iostream>
#include <string>
#include "Gcode.h"
#include "parser.h"

#define _CRT_SECURE_NO_WARNINGS
#pragma warning(disable:4996)

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
	"%d "
	"%d "
	"%d "
	"%d"
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

static Gcode *gcodes[GCODE_SIZE] = {
	&M2,
	&M10,
	&M11,
	&M1,
	&M5,
	&M4,
	&G28,
	&G1
};

void parseCode(std::string s) {

	auto token = s.substr(0, s.find(' '));
	if(token == s || token == ""){
		std::cout << "no space found!" << std::endl;
		return;
	}

	if (token == M2.getGcode()) {

		if (std::sscanf(
			s.c_str(),
			M2.getRegex(),
			&savePenUp,
			&savePenDown) >= 2) {
			std::cout << "M2: " << std::endl;
			std::cout << savePenUp << std::endl;
			std::cout << savePenDown << std::endl;
		}
	}

	else if (token == M11.getGcode()) {


		// if (std::sscanf(
		// 	s.c_str(),
		// 	M11.getRegex(),
		// 	&L4,
		// 	&L3,
		// 	&L2,
		// 	&L1) >= 4) {
		// 	std::cout << "M11: " << std::endl;
		// 	std::cout << L4 << std::endl;
		// 	std::cout << L3 << std::endl;
		// 	std::cout << L2 << std::endl;
		// 	std::cout << L1 << std::endl;
		// }


		std::cout << "Limits to be added" << std::endl;
	}

	else if (token == M1.getGcode()) {

		if (std::sscanf(
			s.c_str(),
			M1.getRegex(),
			&penPos) >= 1) {
			std::cout << "M1: " << std::endl;
			std::cout << penPos << std::endl;
		}
	}

	else if (token == M10.getGcode()) {
		std::cout << token << " XY "
			<< height << " " << width << " 0.00 0.00 A" << dirX << " B" << dirY << " H0 "
			<< speed << " U" << savePenUp << " D" << savePenDown << std::endl;
	}

	else if (token == M5.getGcode()) {

		if (std::sscanf(
			s.c_str(),
			M5.getRegex(),
			&dirX,
			&dirY,
			&height,
			&width,
			&speed) >= 5) {
			std::cout << "M5: " << std::endl;
			std::cout << dirX << std::endl;
			std::cout << dirY << std::endl;
			std::cout << height << std::endl;
			std::cout << width << std::endl;
			std::cout << speed << std::endl;
		}
	}

	else if (token == M4.getGcode()) {
		int laserPower;
		if (std::sscanf(
			s.c_str(),
			M4.getRegex(),
			&laserPower) >= 1) {
			std::cout << laserPower << std::endl;
		}
	}

	else if (token == G28.getGcode()) {
		std::cout << "Move to origo" << std::endl;
	}

	else if (token == G1.getGcode()) {

		if (std::sscanf(
			s.c_str(),
			G1.getRegex(),
			&moveX,
			&moveY,
			&moveA) >= 3) {
			std::cout << moveX << std::endl;
			std::cout << moveY << std::endl;
			std::cout << moveA << std::endl;
		}
	}
	else
	{
		std::cout << "Error! " << s << std::endl;
	}


	//system("pause");
}





