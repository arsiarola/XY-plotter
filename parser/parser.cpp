#include <stdio.h>
#include <string.h>
#include "parser.h"
#include "../printer.h"
#include "../ITM_write.h"
#include "Gcode.h"

#include <stdio.h>
#include <string.h>



static GcodeData data;

// set penpos
static Gcode M1 = Gcode(Gcode::M1, "M1",
        "M1 "
        "%u",
        m1ExtractData
        );

//Save pen up/down
static Gcode M2 = Gcode(Gcode::M2, "M2",
        "M2 "
        "U" "%u "
        "D" "%u",
        m2ExtractData
        );

// set laser power
static Gcode M4 = Gcode(Gcode::M4, "M4",
        "M4 "
        "%u",
        m4ExtractData
        );

// save stepper directions, area and speed
static Gcode M5 = Gcode(Gcode::M5, "M5",
        "M5 "
        "A" "%d " //A0
        "B" "%d " //B0
        "H" "%u " //H310
        "W" "%u " //W380
        "S" "%u", //S80
m5ExtractData
        );

// reply to mdraw with all values
static Gcode M10 = Gcode(Gcode::M10, "M10",
        "M10",
        m10ExtractData
        );

// get the limit switches from plotter
static Gcode M11 = Gcode(Gcode::M11, "M11",
        "M11",
        m11ExtractData
        );

// go to position
static Gcode G1 = Gcode(Gcode::G1, "G1",
        "G1 "
        "X" "%f " //X85.14
        "Y" "%f " //Y117.29
        "A" "%d", //A0
        g1ExtractData
        );

// Go to origin
static Gcode G28 = Gcode(Gcode::G28, "G28",
        "G28 ",
        g28ExtractData
        );

#define GCODE_SIZE 8
static Gcode *gcodes[GCODE_SIZE] = {
    &G1,
    &M1,
    &M2,
    &M4,
    &M5,
    &M10,
    &M11,
    &G28
};

void parseCode(const char *str, QueueHandle_t &queue) {
    int index = -1;
    char gcode[8];
    strncpy(gcode, str, 8);

    trimTrailing(gcode);

    char *token = strchr(gcode, ' ');
    if (token != NULL) {
        gcode[token-gcode] = '\0';
    }

    bool found = false;
    bool dataExtracted = false;
    for (uint8_t i = 0; i < GCODE_SIZE; ++i) {
        if (strcmp(gcodes[i]->getGcode(), gcode) == 0) {
            found = true;
            dataExtracted = gcodes[i]->callback(str);
            break;
        }
    }

    /*Unknown Gcode*/
    if (!found)
    {
        ITM_write("Error!\n");
        ITM_print("%s is unknown Gcode",gcode);
    }

    if (found && dataExtracted) {
        data.id = gcodes[index]->getId();
        //TODO: think about how to make queue public variable
        // make own header for public variables?
        xQueueSendToBack(queue, &data, 0);
    }

    ITM_write("\n");
}


/*Functions*/

/* Set penpos */
bool m1ExtractData(const char *str) {
    if (sscanf( str,
                M1.getFormat(),
                &data.M1.penPos) == 1) {
        ITM_write("M1: ");
        ITM_print("pen position: %u\n", data.M1.penPos);
        return true;
    }
    return false;
}

/* Save pen up and down */
 bool m2ExtractData (const char *str) {
    if (sscanf( str,
                M2.getFormat(),
                &data.M2.savePenUp,
                &data.M2.savePenDown) == 2) {
        ITM_write("M2: ");
        ITM_print("up: %u  down: %u\n",data.M2.savePenUp, data.M2.savePenDown);
        return true;
    }
    return false;
}

/*M4: Set laser power*/
bool m4ExtractData (const char *str) {
    if (sscanf(
                str,
                M4.getFormat(),
                &data.M4.laserPower) == 1) {
        ITM_write("M4: ");
        ITM_print("Power level of laser: %u\n", data.M4.laserPower);
        return true;
    }
    return false;
}

    /*M5: Save stepper directions, plot area, and plotting speed*/
bool m5ExtractData(const char *str) {
    if (sscanf(
                str,
                M5.getFormat(),
                &data.M5.dirX,
                &data.M5.dirY,
                &data.M5.height,
                &data.M5.width,
                &data.M5.speed
            ) == 5) {
        ITM_write("M5: ");
        ITM_print("X direction: %d, Y direction: %d, canvas dimensions: %d x %d, plotting speed: %d\n",
                  data.M5.dirX,
                  data.M5.dirY,
                  data.M5.height,
                  data.M5.width,
                  data.M5.speed);
        return true;
    }
    return false;
}

/* Reply to mdraw with all values? */
bool m10ExtractData (const char *str) {
    ITM_write("M10\n");
    return false;
}

/*M11: Limit switch status query*/
bool m11ExtractData (const char *str) {
    ITM_write("M11\n");
    //TODO: get limit switch statuses from plotter and print them to mdraw
    return false;
}

/*G1: Move to coordinate*/
bool g1ExtractData (const char *str) {
    if (sscanf(
                str,
                G1.getFormat(),
                &data.G1.moveX,
                &data.G1.moveY,
                &data.G1.absoluteOrRelative
                ) == 3)
    {
        ITM_write("G1: ");
        ITM_print("Moving to coordinates X %.2f and Y %.2f\n",
                  data.G1.moveX,
                  data.G1.moveY
                 );
        return true;
    }
    return false;
}

/*G28: Move to origin*/
bool g28ExtractData (const char *str) {
    ITM_write("G28: Moving to origin\n");
    return false;
}

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

