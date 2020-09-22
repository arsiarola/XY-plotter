/*
 * Syslog.cpp
 *
 *  Created on: 26.8.2020
 *      Author: larke
 */
#include "syslog.h"
#include <stdio.h>
#include <string.h>

Syslog::Syslog() {
	syslogMutex = xSemaphoreCreateMutex();
}
Syslog::~Syslog() {
	vSemaphoreDelete (syslogMutex);
}

int Syslog::read() {
	if (syslogMutex != NULL) {
		if (xSemaphoreTake(syslogMutex, portMAX_DELAY) == pdTRUE) {
			int c = Board_UARTGetChar();
			xSemaphoreGive(syslogMutex);
			return c;
		}
	}
}

void Syslog::write(int description) {
	if (xSemaphoreTake(syslogMutex, portMAX_DELAY) == pdTRUE) {
		Board_UARTPutChar(description);
		xSemaphoreGive (syslogMutex);
	}
}

int Syslog::writeString(const char *description) {
	if (xSemaphoreTake(syslogMutex, portMAX_DELAY) == pdTRUE) {
		Board_UARTPutSTR(description);
		xSemaphoreGive (syslogMutex);
	}
	return strlen(description);
}

