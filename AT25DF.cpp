/*
    openaltimeter -- an open-source altimeter for RC aircraft
    Copyright (C) 2010  Jony Hudson
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
#include "AT25DF.h"
#include "WProgram.h"

#include "Messages.h"
#include "SPI.h"

#define AT25DF_DUMMY_BYTE 0x00
#define AT25DF_MANUFACTURER_INFO_COMMAND 0x9f
#define AT25DF_STATUS_READ_COMMAND 0x05
#define AT25DF_STATUS_WRITE_COMMAND 0x01
#define AT25DF_CHIP_ERASE_COMMAND 0xc7
#define AT25DF_WRITE_ENABLE_COMMAND 0x06
#define AT25DF_WRITE_DISABLE_COMMAND 0x04
#define AT25DF_READ_ARRAY_FAST_COMMAND 0x0b
#define AT25DF_WRITE_SEQUENTIAL_COMMAND 0xad

#define AT25DF_STATUS_DONE_MASK 0x01

#define AT25DF_TEST_BUFFER_SIZE 128
#define AT25DF_TEST_REPEAT 128

AT25DF::AT25DF(int ssPin)
{
  _ssPin = ssPin;
}

void AT25DF::setup()
{
  pinMode(_ssPin, OUTPUT);
  writeEnableAndUnprotect();
}

void AT25DF::getManufacturerInfo(uint8_t* response)
{
  commandAndReadN(AT25DF_MANUFACTURER_INFO_COMMAND, response, 4);
}

void AT25DF::printManufacturerInfo()
{
  byte buffer[4];
  getManufacturerInfo(buffer);
  printMessage(FLASH_MANU_MESSAGE);
  Serial.print(buffer[0], HEX);
  Serial.print(" ");
  Serial.print(buffer[1], HEX);
  Serial.print(" ");
  Serial.print(buffer[2], HEX);
  Serial.print(" ");
  Serial.print(buffer[3], HEX);
  Serial.print(" ");
  Serial.println(" .");  
}

void AT25DF::writeAddress(uint32_t address)
{
  Spi.exchangeByte((uint8_t)((address & 0x00ff0000) >> 16));
  Spi.exchangeByte((uint8_t)((address & 0x0000ff00) >> 8));
  Spi.exchangeByte((uint8_t)(address & 0x000000ff));
}

void AT25DF::readArray(uint32_t startAddress, uint8_t* buffer, uint32_t num)
{
  Spi.assertSS(_ssPin);
  Spi.exchangeByte(AT25DF_READ_ARRAY_FAST_COMMAND);
  writeAddress(startAddress);
  Spi.exchangeByte(AT25DF_DUMMY_BYTE);
  for (int i = 0; i < num; i++ )
    buffer[i] = Spi.exchangeByte(AT25DF_DUMMY_BYTE);
  Spi.deassertSS(_ssPin);
}

void AT25DF::writeArray(uint32_t startAddress, uint8_t* buffer, uint32_t num)
{
  writeEnableAndUnprotect();
  Spi.assertSS(_ssPin);
  Spi.exchangeByte(AT25DF_WRITE_SEQUENTIAL_COMMAND);
  writeAddress(startAddress);
  // clock in the first byte of data
  Spi.exchangeByte(buffer[0]);
  Spi.deassertSS(_ssPin);
  waitUntilDone();  
  // and the rest
  for (int i = 1; i < num; i++)
  {
    Spi.assertSS(_ssPin);
    Spi.exchangeByte(AT25DF_WRITE_SEQUENTIAL_COMMAND);
    Spi.exchangeByte(buffer[i]);
    Spi.deassertSS(_ssPin);
    waitUntilDone();
  }
  writeDisable();
}

void AT25DF::chipErase()
{
  writeEnableAndUnprotect();
  command(AT25DF_CHIP_ERASE_COMMAND);
  waitUntilDone();
}

void AT25DF::writeEnableAndUnprotect()
{
  uint8_t comm = 0x00;
  commandAndWriteN(AT25DF_STATUS_WRITE_COMMAND, &comm, 1);
  command(AT25DF_WRITE_ENABLE_COMMAND);
}

void AT25DF::writeDisable()
{
  command(AT25DF_WRITE_DISABLE_COMMAND);
}

uint8_t AT25DF::readStatusRegister()
{
  uint8_t statusReg;
  commandAndReadN(AT25DF_STATUS_READ_COMMAND, &statusReg, 1);
  return statusReg;
}

void AT25DF::waitUntilDone()
{
  while (readStatusRegister() & AT25DF_STATUS_DONE_MASK) {}
}

void AT25DF::commandAndReadN(uint8_t command, uint8_t* buffer, int n)
{
  Spi.assertSS(_ssPin);
  Spi.exchangeByte(command);
  for (int i = 0; i < n; i++)
    buffer[i] = Spi.exchangeByte(AT25DF_DUMMY_BYTE);
  Spi.deassertSS(_ssPin);
}

void AT25DF::commandAndWriteN(uint8_t command, uint8_t* data, int n)
{
  Spi.assertSS(_ssPin);
  Spi.exchangeByte(command);
  for (int i = 0; i < n; i++)
    Spi.exchangeByte(data[i]);
  Spi.deassertSS(_ssPin);
}

void AT25DF::command(uint8_t command)
{
  commandAndReadN(command, 0, 0);
}


void AT25DF::test()
{
  printMessage(FLASH_TEST_MESSAGE);
  printMessage(ERASING_MESSAGE);
  chipErase();
  printMessage(DONE_MESSAGE);
  
  printMessage(WRITING_MESSAGE);
  uint8_t bufferW[AT25DF_TEST_BUFFER_SIZE];
  for (int i = 0; i < AT25DF_TEST_BUFFER_SIZE; i++) bufferW[i] = 0;
  for (uint32_t i = 0; i < AT25DF_TEST_REPEAT; i++) writeArray(i * AT25DF_TEST_BUFFER_SIZE, bufferW, AT25DF_TEST_BUFFER_SIZE);
  printMessage(DONE_MESSAGE);
  
  printMessage(READING_MESSAGE);
  uint8_t bufferR[AT25DF_TEST_BUFFER_SIZE];
  uint32_t checksum;
  for (uint32_t j = 0; j < AT25DF_TEST_REPEAT; j++)
  {
    checksum = 0;
    readArray(j * AT25DF_TEST_BUFFER_SIZE, bufferR, AT25DF_TEST_BUFFER_SIZE);
    for (int i = 0; i < AT25DF_TEST_BUFFER_SIZE; i++) checksum += bufferR[i];
    if (checksum != 0)
    {
      Serial.print("Error in flash, test block ");
      Serial.println(j);
      printMessage(TEST_FAIL_MESSAGE);
    }
  }
  printMessage(DONE_MESSAGE);
  printMessage(ERASING_MESSAGE);
  chipErase();
  printMessage(DONE_MESSAGE);
  printMessage(TEST_PASS_MESSAGE);
}



