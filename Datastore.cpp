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

#include "config.h"
#include "Datastore.h"
#include "AT25DF.h"

#include "WProgram.h"

#include "Messages.h"


Datastore::Datastore(AT25DF* flash)
{
  _flash = flash;
  _firstFreeAddress = 0;
  _numberOfFiles = 0;
  _readPointer = 0;
}

void Datastore::setup()
{
  scanFlash();
}

void Datastore::erase()
{
  _flash->chipErase();
  _firstFreeAddress = 0;
  _numberOfFiles = 0;
}

boolean Datastore::addEntry(LogEntry* logEntry)
{
  if (_firstFreeAddress < DATASTORE_MAX_ADDRESS) {
    _flash->writeArray(_firstFreeAddress, (byte*)logEntry, DATASTORE_LOG_ENTRY_SIZE);
    _firstFreeAddress += DATASTORE_LOG_ENTRY_SIZE;   
    return true;
  }
  else return false;
}

void Datastore::addFileEndMarker()
{
  // We'll use a sequence of bytes, the length of a LogEntry, all set to 0xff
  // as a file end marker. As the flash erases all bytes to 0xff we need do
  // no writing, just move the _firstFreeAddress pointer.
  _firstFreeAddress += DATASTORE_LOG_ENTRY_SIZE; 
  _numberOfFiles++;
}

void Datastore::startRead()
{
  _readPointer = 0;
}

void Datastore::getNextEntry(LogEntry* buffer)
{
  _flash->readArray(_readPointer, (byte*)buffer, DATASTORE_LOG_ENTRY_SIZE );
  _readPointer += DATASTORE_LOG_ENTRY_SIZE; 
}

boolean Datastore::entryAvailable()
{
  return !(_readPointer == _firstFreeAddress);
}

void Datastore::startReverseRead()
{
  _readPointer = _firstFreeAddress - DATASTORE_LOG_ENTRY_SIZE;
}

void Datastore::getPreviousEntry(LogEntry* buffer)
{
  _flash->readArray(_readPointer, (byte*)buffer, DATASTORE_LOG_ENTRY_SIZE );
  _readPointer -= DATASTORE_LOG_ENTRY_SIZE; 
}

boolean Datastore::entryReverseAvailable()
{
  return !(_readPointer == 0);
}

// this function finds the first free address and counts the number of files. If
// the flash is not blank then this function must be called before anything else
// is done.
// A file is considered to end when a "blank" entry is found, i.e. filled all with
// 0xff. When we find two consecutive blank entries we know that we've found the
// start of the free space.
void Datastore::scanFlash()
{
  byte entryBytes[DATASTORE_LOG_ENTRY_SIZE];
  uint32_t entryChecksum;
  boolean previousEntryBlank = false;
  _numberOfFiles = 0;
  startRead();
  while (_readPointer < DATASTORE_MAX_ADDRESS)
  {
    getNextEntry((LogEntry*)entryBytes);
    // we add the bytes of the LogEntry together. The only way we can get 0xff * DATASTORE_LOG_ENTRY_SIZE
    // is if all the bytes are 0xff i.e. this is a blank entry.
    entryChecksum = 0;
    for (int i = 0; i < DATASTORE_LOG_ENTRY_SIZE; i++) entryChecksum += entryBytes[i];
//    ((LogEntry*)entryBytes)->print();
//    Serial.println(entryChecksum);
    if (entryChecksum == ((uint32_t)0xff * (uint32_t)DATASTORE_LOG_ENTRY_SIZE))
    {
      // we've found a blank entry, so update the file counter
      _numberOfFiles++;
      // if the previous entry was also blank, then we've found the end
      // of the used portion of the flash
      if (previousEntryBlank)
      {
        // the readPointer is now pointing to the start of the entry after the _two_ blank entries, so
        // take it back one entry and set this as the first free address
        _firstFreeAddress = _readPointer - DATASTORE_LOG_ENTRY_SIZE;
        // there's no need for a blank entry at the start of the flash - deal with this as a special case
        if (_firstFreeAddress == DATASTORE_LOG_ENTRY_SIZE)
        {
          _firstFreeAddress = 0;
          _numberOfFiles = 0;
        }
        else
        {
          // the file counter will be one bigger than it should be, because of the last blank entry, so fix it
          _numberOfFiles -= 1;
        }
        return;
      }
      // set a flag that this is entry is blank, in case the next one also is, signifying the end of 
      // the used portion
      previousEntryBlank = true;
    }
    else
    {
      // not a blank entry, so clear the blank entry flag
      previousEntryBlank = false;
    }
  }
  // if we've got to here it means the flash is full
  _firstFreeAddress = _readPointer;
}

uint32_t Datastore::getNumberOfFiles()
{
  return _numberOfFiles;
}

uint32_t Datastore::getNumberOfEntries()
{
  return _firstFreeAddress / DATASTORE_LOG_ENTRY_SIZE;
}

void Datastore::testWrite(int n)
{
  LogEntry le;
  for (int i = 0; i < n; i++)
  {
    le.setPressure( i + 1 );
    le.setTemperature( 2 * i + 1 );
    le.setBattery( 0.1 * (double)(i + 1) );
//    le.print();
    addEntry(&le);
  }
  addFileEndMarker();
}

void Datastore::test()
{
  printMessage(ENTRY_SIZE_MESSAGE);
  Serial.println(DATASTORE_LOG_ENTRY_SIZE);
  printMessage(MAX_ENTRIES_MESSAGE);
  Serial.println(DATASTORE_MAX_ENTRIES, DEC);
  printMessage(ERASING_MESSAGE);
  erase();
  printMessage(DONE_MESSAGE);
  printMessage(WRITING_MESSAGE);
  testWrite(1000);
  Serial.print("f1 ");
  testWrite(1000);
  Serial.print("f2 ");
  testWrite(2000);
  Serial.print("f3 ");
  printMessage(DONE_MESSAGE);
  printMessage(DATASTORE_SETUP_MESSAGE);
  setup();
  printMessage(DONE_MESSAGE);
  printMessage(NUM_FILES_MESSAGE);
  Serial.println(getNumberOfFiles(), DEC);
  printMessage(NUM_ENTRIES_MESSAGE);
  Serial.println(getNumberOfEntries(), DEC);
  printMessage(WRITING_MESSAGE);
  testWrite(1000);
  Serial.print("f1 ");
  testWrite(1000);
  Serial.print("f2 ");
  testWrite(2000);
  Serial.print("f3 ");
  printMessage(DONE_MESSAGE);
  printMessage(DATASTORE_SETUP_MESSAGE);
  setup();
  printMessage(DONE_MESSAGE);
  printMessage(NUM_FILES_MESSAGE);
  Serial.println(getNumberOfFiles(), DEC);
  printMessage(NUM_ENTRIES_MESSAGE);
  Serial.println(getNumberOfEntries(), DEC);
  
  printMessage(ERASING_MESSAGE);
  erase();
  printMessage(DONE_MESSAGE);
  
  printMessage(TEST_PASS_MESSAGE);

}

void LogEntry::print()
{
  Serial.print("P: ");
  Serial.print(getPressure());
  Serial.print(" T: ");
  Serial.print(getTemperature());
  Serial.print(" B: ");  
  Serial.print(getBattery());
  Serial.print(" S: ");
  Serial.println(getServo(), DEC);
}

// pressure is stored as a 16-bit signed integer. The mapping
// is pressure = pressureRaw + 101325 . This gives a pressure
// range of 68557 to 134093 hPa. This corresponds to an altitude
// range of -2.4 to +3.2 km around sea-level. Should be enough for most
// purposes!
void LogEntry::setPressure(int32_t pressure)
{
  pressureRaw = (int16_t)(pressure - (int32_t)101325);
}

int32_t LogEntry::getPressure()
{
  return (int32_t)pressureRaw + (int32_t)101325;
}

// temperature is stored in an 8-bit unsigned integer. The mapping
// is (remembering that getTemperature() returns a value in tenths
// of a degree Celcius - the format the BMP085 uses)
// temperature = (rawValue * 2.5) - 150. This gives a range of
// -150 to 487.5 (i.e. -15 to 48.75 C).
void LogEntry::setTemperature(int32_t temperature)
{
  temperatureRaw = (temperature + 150) * 0.4;
}

int32_t LogEntry::getTemperature()
{
  return (temperatureRaw * 2.5) - 150;
}

// we store the battery voltage in an 8-bit unsigned integer. The mapping is 
// batteryVoltage = 2 + (rawValue * 0.05). This gives a range of 
// 2V to 14.75V in 0.05V steps
void LogEntry::setBattery(float battery)
{
  batteryRaw = (battery - 2.0) * 20;
}

float LogEntry::getBattery()
{
  return 2.0 + (0.05 * (float)batteryRaw);
}

// servo values are stored in an 8-bit unsigned integer. The mapping is
// servoValue = servoRaw * 8 + 500. This gives a range of
// 500us to 2540us. Zero is treated as a special case, and zero raw is 
// mapped to zero real;
void LogEntry::setServo(uint16_t servo)
{
  if (servo == 0) servoRaw = 0;
  else servoRaw = (servo - 500) / 8;
}

uint16_t LogEntry::getServo()
{
  if (servoRaw == 0) return 0;
  else return (servoRaw * 8) + 500;
}

boolean LogEntry::isFileEndMarker()
{
  return (pressureRaw == -1 && temperatureRaw == 255);
}

