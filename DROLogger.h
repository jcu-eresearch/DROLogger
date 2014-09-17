/*
	Daintree Rainforest Observatory Data Logger
	Copyright (C) 2014 James Cook University

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.
*/

#ifndef _DROLogger_H_
#define _DROLogger_H_

#include "Arduino.h"
#include "OneWire.h"
#include "Time.h"
#include "DS3232RTC.h"
#include "DS2762.h"
#include "DallasTemperature.h"
#include "SoftwareSerial.h"
#include "AB08XX_I2C.h"
#include "PowerGizmo.h"


#define ONEWIRE_BUS_COUNT 2
#define MAX_RETRIES 5
#define ACK "OK"
#define LOG_INTERVAL 600


bool dro_log(int repeat_count);
bool power_gizmo_error(int repeat_count);

void log_bus(uint8_t bus);
void log_temperature(uint8_t bus, uint8_t *address);
void log_humidity(uint8_t bus, uint8_t *address);
void log_address(Stream* stream, uint8_t *address);

bool repeat(bool (*func)(int repeat_count), uint32_t count, uint32_t delayms);
int freeRam();

enum message_t
{
	DATA = 1,
	ERROR
};

struct header_t
{
	message_t type;
	time_t ts;
	uint32_t code;
};

struct record_t
{
	uint8_t address[8];
	double value;
};

struct upload_t
{
	uint16_t temperature_count;
	uint16_t humidity_count;
};

#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
}
#endif

#endif
