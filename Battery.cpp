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
#include "Settings.h"
#include <math.h>

#define DIVIDER_RATIO 5.7 // the divider is a 1k resistor and a 4.7k resistor
#define MCU_SUPPLY_VOLTAGE 3.3

#define CONVERSION_FACTOR (DIVIDER_RATIO * MCU_SUPPLY_VOLTAGE) / 1024.0

Battery::Battery(int analogInputPin)
{
  _analogInputPin = analogInputPin;
}

void Battery::setup(BatteryType batteryType, float batteryMonitorCalibration, float threshold)
{
  _isLow = false;
  _batteryType = batteryType;
  _calibration = batteryMonitorCalibration;
  _threshold = threshold;
  if (batteryType == BATTERY_TYPE_LIPO)
  {
    // measure the LIPO voltage and work out the number of cells
    float v = readVoltage();
    _numberOfCells = ((v < LIPO_CELL_DETECT_THRESHOLD) ? 2 : 3);
  }
}

float Battery::readVoltage()
{
  return CONVERSION_FACTOR * _calibration * analogRead(_analogInputPin);
}

// There is a small amount of hysteresis in this function to stop the alarm from
// intermittently switching on and off near the voltage threshold.
boolean Battery::isLow()
{
  if (_batteryType == BATTERY_TYPE_NONE) return false;
  // TODO: would be better to abstract the hysteresis calculation here
  boolean low;
  double v = readVoltage();
  if (_batteryType == BATTERY_TYPE_LIPO)
  {
    if (_isLow) low = ((v / _numberOfCells) < (_threshold + BATTERY_MONITOR_HYSTERESIS));
    else low = ((v / _numberOfCells) < _threshold);
  }
  if (_batteryType == BATTERY_TYPE_NIMH)
  {
    if (_isLow) low = (v < (_threshold + BATTERY_MONITOR_HYSTERESIS));
    else low = (v < _threshold);
  }
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
