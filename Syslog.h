/*
 * Syslog.h
 *
 *  Created on: 26.8.2020
 *      Author: larke
 */

#ifndef SYSLOG_H_
#define SYSLOG_H_

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include <string>

class Syslog {
public:
	Syslog();
	virtual ~Syslog();
	int read();
	void write(int *description);
	void writeString(char *description);
private:
	SemaphoreHandle_t syslogMutex;
};





#endif /* SYSLOG_H_ */
