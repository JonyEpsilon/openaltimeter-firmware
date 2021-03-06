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

#include <EEPROM.h>
#include "config.h"
#include "Settings.h"

#include "Messages.h"


void SettingsStore::save(Settings* settings) 
{
  byte* byteArray = (byte*)settings;
  for (int i = 0; i < SETTINGS_SIZE; i++) EEPROM.write(i, byteArray[i]);
}

void SettingsStore::load(Settings* settings)
{
  byte* byteArray = (byte*)settings;
  for (int i = 0; i < SETTINGS_SIZE; i++) byteArray[i] = EEPROM.read(i);
  // if the settings memory is blank then we need to make sure we have some sensible defaults that will let the OA run.
  // We consider a log interval of 0 as equating to blank settings.
  if (settings->logIntervalMS == 0) {
    settings->logIntervalMS = LOG_INTERVAL_MS_DEFAULT;
    settings->heightUnits = HEIGHT_UNITS_DEFAULT;
    settings->batteryType = BATTERY_TYPE_NONE;
    settings->lowVoltageThreshold = LOW_VOLTAGE_THRESHOLD_DEFAULT;
    settings->batteryMonitorCalibration = 1.0;
    settings->logServo = false;
    settings->midPositionAction = OUTPUT_MAX_LAUNCH_HEIGHT;
    settings->onPositionAction = OUTPUT_MAX_HEIGHT;
  }
}

void SettingsStore::erase()
{
  for (int i = 0; i < 512; i++) EEPROM.write(i,0);
}

void SettingsStore::test()
{
  printMessage(SETTINGS_TEST_MESSAGE);
  Settings s;
  s.logIntervalMS = 250;
  save(&s);
  Settings s2;
  load(&s2);
  erase();
  printMessage(DONE_MESSAGE);
  if (s2.logIntervalMS == 250) printMessage(TEST_PASS_MESSAGE);
  else printMessage(TEST_FAIL_MESSAGE);
}

void Settings::print()
{
  printMessage(SETTINGS_LOG_INTERVAL_MESSAGE);
  Serial.println(logIntervalMS);
  printMessage(SETTINGS_HEIGHT_UNITS_MESSAGE);
  Serial.println(heightUnits);
  printMessage(SETTINGS_BATTERY_TYPE_MESSAGE);
  switch (batteryType)
  {
    case BATTERY_TYPE_NONE:
      printMessage(SETTINGS_NO_BATTERY_MESSAGE);
      break;
    case BATTERY_TYPE_LIPO:
      printMessage(SETTINGS_LIPO_BATTERY_MESSAGE);
      break;
    case BATTERY_TYPE_NIMH:
      printMessage(SETTINGS_NIMH_BATTERY_MESSAGE);
      break;
  }
  printMessage(SETTINGS_LOW_VOLTAGE_THRESHOLD_MESSAGE);
  Serial.println(lowVoltageThreshold);
  printMessage(SETTINGS_BATTERY_MONITOR_CALIBRATION_MESSAGE);
  Serial.println(batteryMonitorCalibration);
  printMessage(SETTINGS_LOG_SERVO_MESSAGE);
  Serial.println(logServo);
  printMessage(SETTINGS_MID_POSITION_MESSAGE);
  switch (midPositionAction)
  {
    case OUTPUT_MAX_HEIGHT:
      printMessage(SETTINGS_MAX_HEIGHT_MESSAGE);
      break;
    case OUTPUT_MAX_LAUNCH_HEIGHT:
      printMessage(SETTINGS_MAX_LAUNCH_HEIGHT_MESSAGE);
      break;
    case OUTPUT_LAUNCH_WINDOW_END_HEIGHT:
      printMessage(SETTINGS_LAUNCH_WINDOW_END_HEIGHT_MESSAGE);
      break;
    case OUTPUT_BATTERY_VOLTAGE:
      printMessage(SETTINGS_BATTERY_VOLTAGE_MESSAGE);
      break;
  }
  printMessage(SETTINGS_ON_POSITION_MESSAGE);
  switch (onPositionAction)
  {
    case DO_NOTHING:
      printMessage(SETTINGS_DO_NOTHING_MESSAGE);
      break;    
    case OUTPUT_MAX_HEIGHT:
      printMessage(SETTINGS_MAX_HEIGHT_MESSAGE);
      break;
    case OUTPUT_MAX_LAUNCH_HEIGHT:
      printMessage(SETTINGS_MAX_LAUNCH_HEIGHT_MESSAGE);
      break;
    case OUTPUT_LAUNCH_WINDOW_END_HEIGHT:
      printMessage(SETTINGS_LAUNCH_WINDOW_END_HEIGHT_MESSAGE);
      break;
    case OUTPUT_BATTERY_VOLTAGE:
      printMessage(SETTINGS_BATTERY_VOLTAGE_MESSAGE);
      break;
  }
}
