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
    
    ********************************************************************
    This file is a driver for the Bosch SensorTech BMP085 barometric
    pressure sensor.
    
    These two websites were very helpful in getting this to work:
    http://interactive-matter.org/2009/12/arduino-barometric-pressure-sensor-bmp085/
    http://news.jeelabs.org/
    The calibration code is based on the MIT licenced code from Jean-Claude
    Wippler's Ports library, available from jeelabs (URL above).
*/

#include "config.h"
#include "BMP085.h"
#include "WProgram.h"

#include <Wire.h>

#include "Messages.h"

BMP085::BMP085(int xclrPin, int eocPin, int oversampling)
{
  _xclrPin = xclrPin;
  _eocPin = eocPin;
  _oversampling = oversampling;
}

void BMP085::setup()
{
  if (_xclrPin != 0) pinMode(_xclrPin, OUTPUT);
  pinMode(_eocPin, INPUT);
  digitalWrite(_eocPin, LOW);
  enable();
  // let the sensor stabilise
  delay(20);
  // get the calibration constants from the sensor
  _ac1 = read16bit(0xAA);
  _ac2 = read16bit(0xAC);
  _ac3 = read16bit(0xAE);
  _ac4 = read16bit(0xB0);
  _ac5 = read16bit(0xB2);
  _ac6 = read16bit(0xB4);
  _b1 = read16bit(0xB6);
  _b2 = read16bit(0xB8);
  _mb = read16bit(0xBA);
  _mc = read16bit(0xBC);
  _md = read16bit(0xBE);
}

void BMP085::enable()
{
  if (_xclrPin != 0)
  {
    digitalWrite(_xclrPin, HIGH);
    delay(10);
  }
}

void BMP085::disable()
{
  if (_xclrPin != 0) digitalWrite(_xclrPin, LOW);
}

void BMP085::update()
{
  updateRawTemperature();
  updateRawPressure();
  calculate();
}

void BMP085::calculate()
{    
  int32_t x1, x2, x3, b3, b5, b6, p;
  uint32_t b4, b7;

  x1 = (_ut - _ac6) * _ac5 >> 15;
  x2 = ((int32_t) _mc << 11) / (x1 + _md);
  b5 = x1 + x2;
  temperature = (b5 + 8) >> 4;

  b6 = b5 - 4000;
  x1 = (_b2 * (b6 * b6 >> 12)) >> 11; 
  x2 = _ac2 * b6 >> 11;
  x3 = x1 + x2;
  b3 = (((int32_t) _ac1 * 4 + x3) << _oversampling) >> 2;
  x1 = _ac3 * b6 >> 13;
  x2 = (_b1 * (b6 * b6 >> 12)) >> 16;
  x3 = ((x1 + x2) + 2) >> 2;
  b4 = (_ac4 * (uint32_t) (x3 + 32768)) >> 15;
  b7 = ((uint32_t) _up - b3) * (50000 >> _oversampling);
  p = b7 < 0x80000000 ? (b7 * 2) / b4 : (b7 / b4) * 2;
  x1 = (p >> 8) * (p >> 8);
  x1 = (x1 * 3038) >> 16;
  x2 = (-7357 * p) >> 16;
  pressure = p + ((x1 + x2 + 3791) >> 4);
}

void BMP085::updateRawTemperature()
{
  write8bit(0xf4, 0x2e);
  if (_eocPin == 0) delay(5);
  else { 
    while (digitalRead(_eocPin) != HIGH) {
    } 
  }
  _ut = read16bit(0xf6);
}

void BMP085::updateRawPressure()
{
  write8bit(0xf4, 0x34 + (_oversampling << 6));
  if (_eocPin == 0)
  {
    switch (_oversampling)
    {
    case 0:
      delay(5);
      break;
    case 1:
      delay(8);
      break;
    case 2:
      delay(14);
      break;
    default:
      delay(26);
      break;
    }      
  }
  else { 
    while (digitalRead(_eocPin) != HIGH) {
    } 
  }
  _up = read24bit(0xf6) >> (8 - _oversampling);
}

void BMP085::softOversample(int ost, int osp)
{
  uint32_t avUT = 0;
  for (int i = 0 ; i < ost ; i++)
  {
    updateRawTemperature();
    avUT += _ut;
  }
  _ut = avUT / ost;
  
  uint32_t avUP = 0;
  for (int i = 0 ; i < osp ; i++)
  {
    updateRawPressure();
    avUP += _up;
  }
  _up = avUP / osp;
  calculate();
}

// sets the base pressure which is used to calculate alititude changes
void BMP085::setBasePressure()
{
  softOversample(ALTIMETER_OST, ALTIMETER_OSP);
  _basePressure = pressure;
}

void BMP085::setBasePressure(int32_t pressure)
{
  _basePressure = pressure;
}

uint32_t BMP085::getBasePressure()
{
  return _basePressure;
}

// this function assumes that basePressure is at sea level. This could be fixed if its important to anyone (like
// those who live up mountains!)
int32_t BMP085::convertToAltitude(uint32_t pressure, float heightUnits)
{
  return heightUnits * 44330.0 * (1.0 - pow((double)pressure / (double)_basePressure, 1.0 / 5.25));
}

uint8_t BMP085::read8bit(uint8_t reg)
{
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.send(reg);
  Wire.endTransmission();

  Wire.requestFrom(BMP085_ADDRESS, 1);
  while (Wire.available() < 1) {
  }
  return (uint8_t)Wire.receive();
}

// reads a byte from the given register and one from the following register, and returns them
// as a 16-bit unsigned value.
uint16_t BMP085::read16bit(uint8_t reg)
{
  uint16_t msb = read8bit(reg);
  uint8_t lsb = read8bit(reg + 1);
  return (msb << 8) + lsb;
}

// reads bytes from the given register and the following two registers, and returns them
// as a 32-bit unsigned value.
uint32_t BMP085::read24bit(uint8_t reg)
{
  uint32_t msb = read8bit(reg);
  uint16_t lsb = read8bit(reg + 1);
  uint8_t patheticByte = read8bit(reg + 2);
  return (msb << 16) + (lsb << 8) + patheticByte;
}

void BMP085::write8bit(uint8_t reg, uint8_t value)
{
  Wire.beginTransmission(BMP085_ADDRESS);
  Wire.send(reg);
  Wire.send(value);
  Wire.endTransmission();
}

void BMP085::test()
{
  Serial.println("Testing pressure sensor ...");
  Serial.print("P: ");
  softOversample(ALTIMETER_OST, ALTIMETER_OSP);
  Serial.print(pressure);
  Serial.print(" T: ");
  Serial.println(temperature);
  Serial.println("Done.");
  
  // to pass the test the temperature should be between 15 and 30 degrees C, and the pressure between 99000 and 103000 hPa
  if (temperature < 300 && temperature > 150 && pressure < 105000 && pressure > 99000) printMessage(TEST_PASS_MESSAGE);
  else printMessage(TEST_FAIL_MESSAGE);
}


