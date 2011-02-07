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

#ifndef AT25DF_H
#define AT25DF_H

#include "WProgram.h"
#include "config.h"

#define AT25DF_SIZE 524288

class AT25DF
{
  public:
    AT25DF(int ssPin);
    void setup();
    void getManufacturerInfo(uint8_t* response);
    void printManufacturerInfo();
    void readArray(uint32_t startAddress, uint8_t* buffer, uint32_t num);
    void writeArray(uint32_t startAddress, uint8_t* buffer, uint32_t num);
    void chipErase();
    uint8_t readStatusRegister();
    void waitUntilDone();
    void writeEnableAndUnprotect();
    void writeDisable();
    void test();
  private:
    int _ssPin;
    void commandAndReadN(uint8_t command, uint8_t* buffer, int n);
    void commandAndWriteN(uint8_t command, uint8_t* buffer, int n);
    void command(uint8_t command);
    void writeAddress(uint32_t address);
};

#endif /*AT25DF_H*/
