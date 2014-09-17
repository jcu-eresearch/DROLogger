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

#include "DROLogger.h"


OneWire* busses[ONEWIRE_BUS_COUNT];
DallasTemperature* temps[ONEWIRE_BUS_COUNT];

AB08XX_I2C* clock;
PowerGizmo* power;

SoftwareSerial* debug;
Stream* data;

upload_t upload;
record_t start_delim;
record_t end_delim;

uint8_t retry_count = 0;
boolean sleep = false;


void setup()
{
	uint8_t *buf = (uint8_t *)&start_delim;
	uint8_t *buf2 = (uint8_t *)&end_delim;
	for(uint16_t i = 0; i < sizeof(record_t); i++)
	{
		buf[i] = i;
		buf2[sizeof(record_t) - i - 1] = i;
	}
	Serial.begin(9600);
	data = &Serial;
	debug = new SoftwareSerial(8,9);
	debug->begin(57600);
	debug->println("Starting...");
	busses[0] = new OneWire(A0);
	busses[1] = new OneWire(A1);
	temps[0] = new DallasTemperature(busses[0]);
	temps[1] = new DallasTemperature(busses[1]);
	debug->println(freeRam());
	debug->print("header_t size: ");
	debug->println(sizeof(header_t));
	debug->print("record_t size: ");
	debug->println(sizeof(record_t));
	Serial.setTimeout(5000);

	clock = new AB08XX_I2C();
	power = new PowerGizmo(clock);
	power->set(RTC.get());
}

void loop()
{

	if(!repeat(&dro_log, 5, 100))
	{
		debug->println("Failed to send data.");
	}
	time_t now = power->get();
	if(power->wakeUpAt((now - (now % LOG_INTERVAL)) + LOG_INTERVAL) == POWERGIZMO_OK)
	{
		power->powerDown();
	}else
	{
		if(!repeat(&power_gizmo_error, 5, 100))
		{
			debug->println("Failed send power gizmo error.");
		}
	}
}

bool dro_log(int repeat_count)
{
	data->write((uint8_t*)&start_delim, sizeof(record_t));
	header_t header = { DATA, RTC.get(),repeat_count};
	data->write((uint8_t*)&header, sizeof(header_t));
	upload.humidity_count = 0;
	upload.temperature_count = 0;
	for(int i = 0; i < ONEWIRE_BUS_COUNT; i++)
	{
		temps[i]->begin();
		temps[i]->requestTemperatures();
	}
	for(int i = 0; i < ONEWIRE_BUS_COUNT; i++)
	{
		debug->print("Logging Bus: ");
		debug->println(i);
		log_bus(i);
	}

	debug->print("Temperature Count: ");
	debug->println(upload.temperature_count);
	debug->print("Humidity Count: ");
	debug->println(upload.humidity_count);
	debug->println();

	data->write((uint8_t*)&end_delim, sizeof(record_t));
	debug->println(freeRam());

	if(data->find(ACK))
	{
		debug->println("ACK received.");
		return true;
	}
	debug->println("ACK not received.");
	return false;
}

bool power_gizmo_error(int repeat_count)
{

}

void log_bus(uint8_t bus)
{
	busses[bus]->reset_search();
	uint8_t address[8];

	while(busses[bus]->search(address))
	{
		if (OneWire::crc8(address, 7) == address[7]) {

			switch(address[0])
			{
				case 0x28:
				case 0x10:
					log_temperature(bus, address);
					break;
				case 0x30:
					log_humidity(bus, address);
					break;
				default:
					break;
			}
		}else
		{
			debug->println("Error");
		}
	}
}

void log_temperature(uint8_t bus, uint8_t *address)
{

	record_t record;
	record.value = temps[bus]->getTempC(address);
	memcpy(&record.address, address, 8);
	if(temps[bus]->getResolution(address) != TEMP_12_BIT)
	{
		temps[bus]->setResolution(address, TEMP_12_BIT);
	}

	data->write((uint8_t*)&record, sizeof(record_t));

	debug->print(' ');
	log_address(debug, address);
	debug->print(' ');
	debug->println(record.value);

	upload.temperature_count++;
}

void log_humidity(uint8_t bus, uint8_t *address)
{

	record_t record;
	DS2762 humidity(busses[bus], address);
	if(!humidity.isSleepModeEnabled())
	{
		humidity.setSleepMode(true);
	}

	memcpy(&record.address, address, 8);
	record.value = humidity.readADC();

	data->write((uint8_t*)&record, sizeof(record_t));

	debug->print(' ');
	log_address(debug, address);
	debug->print(' ');
	debug->println(record.value);

	upload.humidity_count++;
}

void log_address(Stream* stream, uint8_t *address)
{
	for(int i = 0; i < 8; i++)
	{
		stream->print(address[i], HEX);
	}
}

bool repeat(bool (*func)(int repeat_count), uint32_t count, uint32_t delayms)
{
	for(uint32_t i = 0; i < count; i++)
	{
		if(func(i))
		{
			return true;
		}
		delay(delayms);
	}
	return false;
}

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
