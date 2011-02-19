/*
    openaltimeter -- an open-source altimeter for RC aircraft
    Copyright (C) 2010  Jony Hudson, Jan Steidl
    http://openaltimeter.org

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef DATASTORE_H
#define DATASTORE_H

#include "AT25DF.h"

#include "WProgram.h"
#include "config.h"

#define DATASTORE_LOG_ENTRY_SIZE sizeof(LogEntry)
#define DATASTORE_MAX_ENTRIES (uint32_t)(((double)AT25DF_SIZE / (double)DATASTORE_LOG_ENTRY_SIZE) - 2)  // the - 2 makes sure that there are always a
                                                                                                        // couple of null records at the end.                                                                                                     
#define DATASTORE_MAX_ADDRESS (AT25DF_SIZE - (2 * DATASTORE_LOG_ENTRY_SIZE))                                      // biggest possible entry address                                                        

class LogEntry
{
  public:
    void print();
    int32_t getPressure();
    int32_t getTemperature();
    float getBattery();
    uint8_t getServo();
    void setPressure(int32_t pressure);
    void setTemperature(int32_t temperature);
    void setBattery(float battery);
    void setServo(uint8_t servo);
  private:
    int16_t pressureRaw;
    uint8_t temperatureRaw;
    uint8_t batteryRaw;
    uint8_t servoRaw;
} __attribute__ ((__packed__)); // this is to force the compiler not to pad the data structure. It probably makes no difference on AVR-GCC.

class Datastore
{
  public:
    Datastore(AT25DF* flash);
    void setup();
    boolean addEntry(LogEntry* logEntry);
    void addFileEndMarker();
    void startRead();
    void getNextEntry(LogEntry* buffer);
    boolean entryAvailable();
    void startReverseRead();
    void getPreviousEntry(LogEntry* buffer);
    boolean entryReverseAvailable();
    void erase();
    uint32_t getNumberOfFiles();
    uint32_t getNumberOfEntries();
    void testWrite(int n);
    void test();
  private:
    AT25DF* _flash;
    uint32_t _firstFreeAddress;
    uint32_t _numberOfFiles;
    uint32_t _readPointer;
    void scanFlash();
};

#endif /*DATASTORE_H*/
