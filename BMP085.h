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

#ifndef BMP085_H
#define BMP085_H

#include "WProgram.h"
#include "config.h"

#define BMP085_ADDRESS 0x77

#define BMP085_ULTRA_LOW_POWER 0
#define BMP085_STANDARD 1
#define BMP085_HIGH_RESOLUTION 2
#define BMP085_ULTRA_HIGH_RESOLUTION 3

class BMP085
{
  public:
    int32_t temperature;
    int32_t pressure;
    BMP085(int xclrPin, int eocPin, int oversampling);
    void setup();
    void enable();
    void disable();
    void update();
    void calculate();
    void updateRawTemperature();
    void updateRawPressure();
    void softOversample(int ost, int osp);
    void setBasePressure();
    void setBasePressure(int32_t pressure);
    uint32_t getBasePressure();
    float convertToAltitude(uint32_t pressure, float heightUnits);
    void test();
  private:
    // configuration
    int _xclrPin;
    int _eocPin;
    int _oversampling;
    // calibration constants
    int16_t _ac1;
    int16_t _ac2; 
    int16_t _ac3; 
    uint16_t _ac4;
    uint16_t _ac5;
    uint16_t _ac6;
    int16_t _b1; 
    int16_t _b2;
    int16_t _mb;
    int16_t _mc;
    int16_t _md;
    uint32_t _basePressure;
    // raw readings
    uint32_t _ut;
    uint32_t _up;
    // low-level comms with the device
    uint8_t read8bit(uint8_t register);
    uint16_t read16bit(uint8_t register);
    uint32_t read24bit(uint8_t register);
    void write8bit(uint8_t register, uint8_t value);
};

#endif /*BMP085_H*/

