#include "parser.h"
#include <iostream>
#include <fstream>
#include <vector>



int main() {
	//std::string s = "M2 U150 D90";
	//std::string s = "M11 -1 2 3 4";
	//std::string s = "M1 90"; 
	//std::string s = "M10";
	//std::string s = "M5 A0 B0 H310 W380 S80";
	//std::string s = "M4 140";
	//std::string s = "G1 X85.14 Y117.29 A0";
	parseCode("M10");
	parseCode("M5 A0 B0 H310 W380 S80");
	parseCode("M2 U150 D90");
	parseCode("M10");
	parseCode("G28");


    std::ifstream input("gcode01.txt");
	std::string str;
	int i = 0;
	std::string code = "";
	std::vector<std::string> result;

	while (getline(input, str)) {
		i++;
        //str.append("\n");
		//std::cout << str ;
        parseCode(str);
    }
}