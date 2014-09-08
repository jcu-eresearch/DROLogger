/*
	Daintree Rainforest Observatory Data Logger
	Copyright (C) 2014  NigelB

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
time_t temp_sample[ONEWIRE_BUS_COUNT];

SoftwareSerial* debug;
Stream* data;

upload_t upload;
record_t delim;

void setup()
{
	uint8_t *buf = (uint8_t *)&delim;
	for(uint16_t i = 0; i < sizeof(record_t); i++)
	{
		buf[i] = i;
	}
	Serial.begin(57600);
	data = &Serial;
	debug = new SoftwareSerial(8,9);
	debug->begin(9600);
	debug->println("Starting...");
	busses[0] = new OneWire(A0);
	busses[1] = new OneWire(A1);
	temps[0] = new DallasTemperature(busses[0]);
	temps[1] = new DallasTemperature(busses[1]);
	debug->println(freeRam());
	debug->println(sizeof(record_t));

}

void loop()
{
	data->write((uint8_t*)&delim, sizeof(record_t));
	upload.humidity_count = 0;
	upload.temperature_count = 0;
	for(int i = 0; i < ONEWIRE_BUS_COUNT; i++)
	{
		temps[i]->begin();
		temps[i]->requestTemperatures();
		temp_sample[i]=RTC.get();
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
	debug->println(freeRam());
	delay(500);
	data->write((uint8_t*)&delim, sizeof(record_t));
}

uint64_t log_bus(uint8_t bus)
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
	record.timestamp = temp_sample[bus];
	record.value = temps[bus]->getTempC(address);
	memcpy(&record.address, address, 8);
	if(temps[bus]->getResolution(address) != TEMP_12_BIT)
	{
		temps[bus]->setResolution(address, TEMP_12_BIT);
	}

	data->write((uint8_t*)&record, sizeof(record_t));

	debug->print(record.timestamp);
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
	record.timestamp = RTC.get();
	memcpy(&record.address, address, 8);
	record.value = humidity.readADC();

	data->write((uint8_t*)&record, sizeof(record_t));

	debug->print(record.timestamp);
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

int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
