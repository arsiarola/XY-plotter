#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "../printer.h"
#include "../ITM_write.h"
#include "Gcode.h"

#include <stdio.h>
#include <string.h>


//M1
uint8_t penPos = 0;
//M2
uint8_t savePenUp = 0;
uint8_t savePenDown = 0;
//M4
uint8_t laserPower;
//M5
bool dirX = 0;
bool dirY = 0;
uint32_t height = 0;
uint32_t width = 0;
uint8_t speed = 0;
//G1
float moveX = 0;
float moveY = 0;
bool absoluteOrRelative = 0; //0 absolute, 1 relative

// set penpos
Gcode M1 = Gcode(Gcode::M1, "M1",
                 "M1 "
                 "%u"
                );

//Save pen up/down
Gcode M2 = Gcode(Gcode::M2, "M2",
                 "M2 "
                 "U" "%u "
                 "D" "%u"
                );

// set laser power
Gcode M4 = Gcode(Gcode::M4, "M4",
                 "M4 "
                 "%u"
                );

// save stepper directions, area and speed
Gcode M5 = Gcode(Gcode::M5, "M5",
                 "M5 "
                 "A" "%d " //A0
                 "B" "%d " //B0
                 "H" "%u " //H310
                 "W" "%u " //W380
                 "S" "%u" //S80
                );

// reply to mdraw with all values
Gcode M10 = Gcode(Gcode::M10, "M10",
                  "M10"
                 );

// get the limit switches from plotter
Gcode M11 = Gcode(Gcode::M11, "M11",
                  "M11"
                 );

// go to position
Gcode G1 = Gcode(Gcode::G1, "G1",
                 "G1 "
                 "X" "%f " //X85.14
                 "Y" "%f " //Y117.29
                 "A" "%d" //A0
                );

// Go to origin
Gcode G28 = Gcode(Gcode::G28, "G28",
                  "G28 "
                 );

#define GCODE_SIZE 8
static Gcode *gcodes[GCODE_SIZE] = {
    &M1,
    &M2,
    &M4,
    &M5,
    &M10,
    &M11,
    &G1,
    &G28
};

void parseCode(const char *str) {
    bool handled = true;
    int index = -1;
    GcodeData data;
    char gcode[128];
    strncpy(gcode, str, 128);

    trimTrailing(gcode);

    char *token = strchr(gcode, ' ');
    if (token != NULL) {
        gcode[token-gcode] = '\0';
    }

    /*M1: Set pen position*/
    if (strcmp(gcode, gcodes[++index]->getGcode()) == 0) {
        if (sscanf( str,
                    M1.getFormat(),
                    &penPos) == 1) {
            ITM_write("M1: ");
            ITM_print("pen position: %u\n",penPos);
            data.chunk1 = penPos;
        }
    }

    /*M2: Save pen position*/
    else if (strcmp(gcode, gcodes[++index]->getGcode()) == 0) {
        if (sscanf( str,
                    M2.getFormat(),
                    &savePenUp,
                    &savePenDown) == 2) {
            ITM_write("M2: ");
            ITM_print("up: %u  down: %u\n",savePenUp,savePenDown);
            data.chunk1 = savePenUp;
            data.chunk2 = savePenDown;
        }
    }

    /*M4: Set laser power*/
    else if (strcmp(gcode, gcodes[++index]->getGcode()) == 0) {
        if (sscanf(
                    str,
                    M4.getFormat(),
                    &laserPower) == 1) {
            ITM_write("M4: ");
            ITM_print("Power level of laser: %u\n",laserPower);
            data.chunk1 = laserPower;
        }
    }

    /*M5: Save stepper directions, plot area, and plotting speed*/
    else if (strcmp(gcode, gcodes[++index]->getGcode()) == 0) {
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
            data.chunk1 = height;
            data.chunk2 = width;
            data.chunk3 = speed;
            // TODO: define masks and shifts in plotter.h
            int dirXshift = 8;
            int dirYshift = 9;
            data.chunk3 |= dirX << dirXshift;
            data.chunk3 |= dirY << dirYshift;
        }
    }

    /*M10: Log of opening a COM port in mDraw*/
    else if (strcmp(gcode, gcodes[++index]->getGcode()) == 0) {
        ITM_write("M10\n");
    }

    /*M11: Limit switch status query*/
    else if (strcmp(gcode, gcodes[++index]->getGcode()) == 0) {
        ITM_write("M11\n");
        //TODO: get limit switch statuses from plotter and print them to mdraw
    }

    /*G1: Move to coordinate*/
    else if (strcmp(gcode, gcodes[++index]->getGcode()) == 0) {
        if (sscanf(
                    str,
                    G1.getFormat(),
                    &moveX,
                    &moveY,
                    &absoluteOrRelative) == 3) {
            ITM_write("G1: ");
            ITM_print("Moving to coordinates X %.2f and Y %.2f\n",moveX,moveY);
            memcpy(&data.chunk1, &moveX, sizeof(moveX));
            memcpy(&data.chunk2, &moveY, sizeof(moveY));
            data.chunk3 = absoluteOrRelative;
        }
    }

    /*G28: Move to origin*/
    else if (strcmp(gcode, gcodes[++index]->getGcode()) == 0) {
        ITM_write("G28: Moving to origin\n");
    }

    /*Unknown Gcode*/
    else
    {
        ITM_write("Error!\n");
        ITM_print("%s is unknown Gcode",gcode);
        handled = false;
    }
    if (handled) {
        data.id = gcodes[index]->getId();
//TODO: think about how to make queue public variable
        // make own header for public variables?
        //xQueueSendToBack(queue, data, 0);
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
    while (str[i] != '\0')
    {
        if (str[i] != ' ' && str[i] != '\t' && str[i] != '\n')
        {
            index= i;
        }

        i++;
    }

    /* Mark next character to last non-white space character as NULL */
    str[index + 1] = '\0';
}

