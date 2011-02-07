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
#include "Battery.h"
#include "WProgram.h"

#include "Messages.h"
#include <math.h>

#define DIVIDER_RATIO 5.7 // the divider is a 1k resistor and a 4.7k resistor
#define MCU_SUPPLY_VOLTAGE 3.3

#define CONVERSION_FACTOR (DIVIDER_RATIO * MCU_SUPPLY_VOLTAGE * BATTERY_MONITOR_CALIBRATION) / 1024.0

Battery::Battery(int analogInputPin)
{
  _analogInputPin = analogInputPin;
}

void Battery::setup()
{
  _isLow = false;
#ifdef BATTERY_LIPO
  // measure the LIPO voltage and work out the number of cells
  float v = readVoltage();
  _numberOfCells = ((v < LIPO_CELL_DETECT_THRESHOLD) ? 2 : 3);
#endif
}

float Battery::readVoltage()
{
  return CONVERSION_FACTOR * analogRead(_analogInputPin);
}

// There is a small amount of hysteresis in this function to stop the alarm from
// intermittently switching on and off near the voltage threshold.
boolean Battery::isLow()
{
#ifdef BATTERY_NONE
   return false;
#endif
  // TODO: would be better to abstract the hysteresis calculation here
  boolean low;
  double v = readVoltage();
#ifdef BATTERY_LIPO
  if (_isLow) low = ((v / _numberOfCells) < (LIPO_LOW_VOLTAGE_PER_CELL_THRESHOLD + BATTERY_MONITOR_HYSTERESIS));
  else low = ((v / _numberOfCells) < LIPO_LOW_VOLTAGE_PER_CELL_THRESHOLD);
#endif
#ifdef BATTERY_NIMH
  if (_isLow) low = (v < (NIHM_LOW_VOLTAGE_THRESHOLD + BATTERY_MONITOR_HYSTERESIS));
  else low = (v < NIHM_LOW_VOLTAGE_THRESHOLD);
#endif
  _isLow = low;
  return low;
}

int Battery::numberOfCells()
{
  return _numberOfCells;
}

void Battery::test()
{
  printMessage(BATTERY_TEST_MESSAGE);
  printMessage(BATTERY_MESSAGE);
  float v = readVoltage();
  Serial.println(v);
  // the voltage must be between 4.8 and 5.1 to pass the test
  if (v < 5.08 && v > 4.92) printMessage(TEST_PASS_MESSAGE);
  else printMessage(TEST_FAIL_MESSAGE);
}
