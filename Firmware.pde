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
#include "AT25DF.h"
#include "Battery.h"
#include "Beeper.h"
#include "BMP085.h"
#include "Datastore.h"
#include "Messages.h"
#include "Radio.h"
#include "Settings.h"
#include "SPI.h"
#include <Wire.h>
#include <EEPROM.h>

// hardware objects
BMP085 pressureSensor(BMP085_XCLR_PIN, BMP085_EOC_PIN, BMP085_STANDARD);
Battery battery(BATTERY_ANALOG_PIN);
AT25DF flash(AT25DF_SS_PIN);
Datastore datastore(&flash);
Radio radio(RADIO_INPUT_PIN);
Radio servo(SERVO_INPUT_PIN);

// state variables
boolean logging = true;
boolean lowVoltageAlarm = false;
boolean lostModelAlarm = false;
uint32_t millisCounter;

// the settings structure - this is loaded from non-volatile memory when the logger starts up
Settings settings;

// tunes - it's safest to declare the tunes as globals as they will be asynchronously
// accessed by the tune-player interrupt service routine. If they are declared as
// function-local variables then they'll likely go out of scope before the tune is
// finished, causing everything to foul up.
int16_t startupTune[] = STARTUP_TUNE;
int16_t lowVoltageTune[] = LOW_VOLTAGE_TUNE;
int16_t lostModelTune[] = LOST_MODEL_TUNE;
int16_t settingsSaveTune[] = SETTINGS_SAVE_TUNE;

// these are initialised when the settings are loaded and indicate which switch state will trigger the LMA
// and the height readout respectively. This allows the software to be configured for two- and three-position
// switches from the settings. These default values will result in neither the LMA or height readout being
// active
uint8_t lostModelSwitchState = RADIO_SWITCH_IMPOSSIBLE;
uint8_t heightReadoutSwitchState = RADIO_SWITCH_IMPOSSIBLE;

void setup()
{
  // set up all the hardware
  analogReference(EXTERNAL);
  Wire.begin();
  Serial.begin(SERIAL_BAUD_RATE);
  // enable the pull-up resistor on the serial input, to stop noise being read as characters
  digitalWrite(0, HIGH);
  // This prints out the time that this file was last modified every time the altimeter starts up.
  // This is useful when debugging, as it can get difficult to remember which
  // version of the firmware is on a particular board.
  printMessage(WELCOME_MESSAGE);
  Serial.print("Build: ");
  Serial.println(__TIMESTAMP__);
  printMessage(DATA_FORMAT_MESSAGE);
  printMessage(SETTINGS_FORMAT_MESSAGE);
  SettingsStore::load(&settings);
  settings.print();
  Spi.setup();
  pressureSensor.setup();
  radio.setup();
  servo.setup();
  flash.setup();
  Beeper::setup(BEEPER_PIN);
  printMessage(DATASTORE_SETUP_MESSAGE);
  datastore.setup();
  printMessage(DONE_MESSAGE);
  printMessage(ALTIMETER_BASE_PRESSURE_MESSAGE);
  pressureSensor.setBasePressure();
  Serial.print(pressureSensor.getBasePressure());
  Serial.print(" ");
  printMessage(DONE_MESSAGE);
  Beeper::playTune(startupTune);
  Beeper::waitForTuneToEnd();
  battery.setup(settings.batteryType, settings.batteryMonitorCalibration, settings.lowVoltageThreshold);
  if (settings.batteryType == BATTERY_TYPE_LIPO)
  {
    printMessage(LIPO_CELLS_MESSAGE);
    Serial.print(battery.numberOfCells());
    Serial.println(".");
    Beeper::outputInteger(battery.numberOfCells());
  }
  // set up the radio switch states
  if (settings.threePositionSwitch) {
    // if we have a three position switch then lost model alarm is assigned to the fully on position
    lostModelSwitchState = RADIO_SWITCH_ON;
    // and height readout is assigned the middle position
    heightReadoutSwitchState = RADIO_SWITCH_MID;
  } else {
    // whereas if we only have a two position switch then height readout is fully on, and lost model is not accessible
    heightReadoutSwitchState = RADIO_SWITCH_ON;
  }
  // take note of when we're starting the loop, which we'll use for our periodic logging
  millisCounter = millis();
}

// we run around this loop as fast as we can, each time doing several things:
// - check if there's a serial command which would change our state
// - check whether there's a hardware condition which would change our state
// - check whether it's time to write another entry to the log.
void loop()
{
  // Lost model alarm takes precedence over everything, including the low voltage alarm.
  // This is to maximise the battery life when in lost model condition.
  if (radio.getState() == lostModelSwitchState)
  {
    stopLowVoltageAlarm();
    soundLostModelAlarm();
    // slow down the loop, so we're not hammering radio.getState().
    delay(500);
  }
  else
  {
    stopLostModelAlarm();
    checkBatteryVoltage();
    // we only beep out the height if the low voltage alarm is not sounding
    if (!lowVoltageAlarm)
    {
      if (radio.getState() == heightReadoutSwitchState) outputMaxHeight();
    }
    if (millis() > millisCounter) 
    {
      millisCounter += settings.logIntervalMS;
      log();
    }
  }
  // check for serial commands
  if (Serial.available() > 0) parseCommand(Serial.read());
}

void parseCommand(uint8_t comm)
{
  switch (comm)
  {
    case 'e':
      erase();
      break;
    case 'c':
      stopLogging();
      break;
    case 'g':
      startLogging();
      break;
    case 'i':
      getFileInfo();
      break;
    case 'd':
      downloadData();
      break;
    case 'p':
      printData();
      break;
    case 't':
      selfTest();
      break;
    case 'w':
      wipeSettings();
      break;
    case 's':
      programSettings();
      break;
    case 'r':
      readSettings();
      break;
    // these are mainly useful for debugging
    case 'a':
      Beeper::playTune(lowVoltageTune);
      break;
    case 'b':
      Beeper::stopTune();
      break;
    case 'f':
      fakeAFlight();
      break;
    case 'o':
      outputMaxHeight();
      break;
  }
}

// this function that is called periodically to log the pressure etc
void log()
{
  if (logging)
  {
    LogEntry le;
    pressureSensor.softOversample(ALTIMETER_OST, ALTIMETER_OSP);
    le.setPressure(pressureSensor.pressure);
    le.setTemperature(pressureSensor.temperature);
    le.setBattery(battery.readVoltage());
    if (settings.logServo) le.setServo(servo.getServoValueQuick());
    else le.setServo(0);
    if (datastore.addEntry(&le))
      Serial.print(".");
    else
    {
      printMessage(FLASH_FULL_MESSAGE);
      stopLogging();
    }
  }
}

void checkBatteryVoltage()
{
  if (battery.isLow()) soundLowVoltageAlarm();
  else stopLowVoltageAlarm();
}

void erase()
{
  stopLogging();
  printMessage(ERASING_MESSAGE);
  datastore.erase();
  printMessage(DONE_MESSAGE);
}

void stopLogging()
{
  if (logging)
  {
    logging = false;
    printMessage(LOGGING_DISABLED_MESSAGE);
  }
}

void startLogging()
{
  if (!logging)
  {
    millisCounter = millis();
    logging = true;
    printMessage(LOGGING_ENABLED_MESSAGE);
  }
}

void getFileInfo()
{
  stopLogging();
  printMessage(NUM_FILES_MESSAGE);
  Serial.println(datastore.getNumberOfFiles());
  printMessage(NUM_ENTRIES_MESSAGE);
  Serial.println(datastore.getNumberOfEntries());
  printMessage(MAX_ENTRIES_MESSAGE);
  Serial.println(DATASTORE_MAX_ENTRIES);
}

void downloadData()
{
  // we transmit all of the data, followed by two blank records, TODO: followed by a checksum
  LogEntry le;
  stopLogging();
  datastore.startRead();
  while( datastore.entryAvailable() )
  {
    datastore.getNextEntry(&le);
    Serial.write((byte*)&le, DATASTORE_LOG_ENTRY_SIZE);
  }
  // write the two blank entries
  for (int i = 0; i < 2 * DATASTORE_LOG_ENTRY_SIZE; i++) Serial.write(0xff);
}

void printData()
{
  LogEntry le;
  stopLogging();
  datastore.startRead();
  while( datastore.entryAvailable() )
  {
    datastore.getNextEntry(&le);
    le.print();
  }
}

void outputMaxHeight()
{
  stopLogging();
  // working backwards from now we step through the data, keeping track of the highest point,
  // until we detect a launch
  int32_t highestAltitude = 0;
  int32_t lastHeight = 0;
  int16_t launchingFor = 0;
  int32_t lowestAltitude;
  datastore.startReverseRead();
  while (datastore.entryReverseAvailable())
  {
    LogEntry le;
    datastore.getPreviousEntry(&le);
    int32_t alt = pressureSensor.convertToAltitude(le.getPressure(), settings.heightUnits);
    if (alt > highestAltitude) highestAltitude = alt;
    if (lastHeight - alt > (LAUNCH_CLIMB_THRESHOLD * settings.heightUnits * ((float)settings.logIntervalMS / 1000.0))) launchingFor++;
    else launchingFor = 0;
    if (launchingFor >= (LAUNCH_CLIMB_TIME / settings.logIntervalMS)) break;
    lastHeight = alt;
  }
  // we've detected a launch, so now we need to seek back and find the lowest point before the launch
  lowestAltitude = highestAltitude;
  for (int i = LAUNCH_SEEKBACK_SAMPLES; i > 0; i--)
  {
    if (datastore.entryReverseAvailable())
    {
      LogEntry le;
      datastore.getPreviousEntry(&le);
      // we stop if we encounter a file boundary.
      // unlikely to happen, but worth checking for.
      if (le.getPressure() == -1) break;
      int32_t alt = pressureSensor.convertToAltitude(le.getPressure(), settings.heightUnits);
      if (alt < lowestAltitude) lowestAltitude = alt;
    }
    else break;
  }
  Serial.print("Flight height: ");
  Serial.println(highestAltitude - lowestAltitude);
  Beeper::outputInteger(highestAltitude - lowestAltitude);
  delay(500);
  startLogging();
}

void soundLowVoltageAlarm()
{
  // enter the alarm state, if we're not already in it
  if (!lowVoltageAlarm)
  {
    printMessage(START_LVA_MESSAGE);
    lowVoltageAlarm = true;
    Beeper::playTune(lowVoltageTune);
  }
}

void stopLowVoltageAlarm()
{
  // stop the alarm, if it's running
  if (lowVoltageAlarm)
  {
    // exit the alarm state
    printMessage(STOP_LVA_MESSAGE);
    Beeper::stopTune();
    lowVoltageAlarm = false;
  }
}

// this function starts the lost model alarm, if it's not already sounding.
void soundLostModelAlarm()
{
  if (!lostModelAlarm)
  {
    printMessage(START_LMA_MESSAGE);
    lostModelAlarm = true;
    stopLogging();
    Beeper::playTune(lostModelTune);
  }
}

// this function stops the lost model alarm, if it's sounding.
void stopLostModelAlarm()
{
  if (lostModelAlarm)
  {
    printMessage(STOP_LMA_MESSAGE);
    lostModelAlarm = false;
    startLogging();
    Beeper::stopTune();
  }
}

// this function erases the settings memory
void wipeSettings()
{
  stopLogging;
  printMessage(WIPE_SETTINGS_MESSAGE);
  SettingsStore::erase();
  Beeper::playTune(settingsSaveTune);
  Beeper::waitForTuneToEnd();
  printMessage(DONE_MESSAGE);
}

// this function reads a settings structure off the serial port and writes it to the settings store
void programSettings()
{ 
  byte* settingsBytes = (byte*)&settings;
  stopLogging();
  while (Serial.available() < SETTINGS_SIZE) {}
  for (int i = 0; i < SETTINGS_SIZE; i++) settingsBytes[i] = (byte)Serial.read();
  SettingsStore::save(&settings);
  Beeper::playTune(settingsSaveTune);
  Beeper::waitForTuneToEnd();
}

// this function outputs the current settings to the serial port
void readSettings()
{
  stopLogging();
  byte* settingsBytes = (byte*)&settings;
  for (int i = 0; i < SETTINGS_SIZE; i++) Serial.write(settingsBytes[i]);
}

// this adds a fake flight to the logger's memory. Useful for testing.
void fakeAFlight()
{
  printMessage(FAKING_FLIGHT_MESSAGE);
  uint32_t bp = pressureSensor.getBasePressure();
  for (uint32_t i = 0; i < 10; i++)
  {
    LogEntry le;
    le.setPressure(bp - i*100 * 3);
    le.setTemperature(20.0);
    le.setBattery(5.99);
    le.setServo(0);
    datastore.addEntry(&le);
  }
  for (uint32_t i = 0; i < 500; i++)
  {
    LogEntry le;
    le.setPressure( bp - (500-i)*2 * 3 );
    le.setTemperature(20.0);
    le.setBattery(5.99); 
    le.setServo(0);
    datastore.addEntry(&le);
  }
  printMessage(DONE_MESSAGE);
}

void selfTest()
{
  stopLogging();
  printMessage(DIAG_RUN_MESSAGE);
  flash.test();
//  datastore.test();
  datastore.erase();
  pressureSensor.test();
  battery.test();
  Beeper::test();
  radio.test();
  SettingsStore::test();
  
  // we log a number of entries and then look at the range of the logged values
  // (this is easier than computing the s.d., and does the job pretty much as well.)
  Serial.println("Testing sensor noise.");
  logging = true;
  for (int i = 0; i < NUMBER_OF_TEST_LOGS; i++) log();
  logging = false;

  LogEntry le;
  int32_t pMin, pMax, tMin, tMax;
  float vMin, vMax;
  datastore.startRead();
  datastore.getNextEntry(&le);
  pMin = le.getPressure();
  pMax = le.getPressure();
  tMin = le.getTemperature();
  tMax = le.getTemperature();
  vMin = le.getBattery();
  vMax = le.getBattery();
  while( datastore.entryAvailable() )
  {
    datastore.getNextEntry(&le);
    if (le.getPressure() < pMin) pMin = le.getPressure();
    if (le.getTemperature() < tMin) tMin = le.getTemperature();
    if (le.getBattery() < vMin) vMin = le.getBattery();
    if (le.getPressure() > pMax) pMax = le.getPressure();
    if (le.getTemperature() > tMax) tMax = le.getTemperature();
    if (le.getBattery() > vMax) vMax = le.getBattery();
  }
  int32_t deltaP = pMax - pMin;
  int32_t deltaT = tMax - tMin;
  float deltaV = vMax - vMin;
  Serial.println();
  Serial.print("deltaP: ");
  Serial.println(deltaP);
  Serial.print("deltaT: ");
  Serial.println(deltaT);
  Serial.print("deltaV: ");
  Serial.println(deltaV);
  
  // max deviation of 150 hPa, 1.5 degree C, and 100mV is acceptable
  if (deltaP < 150 && deltaT < 15 && deltaV < 0.1) printMessage(TEST_PASS_MESSAGE);
  else printMessage(TEST_FAIL_MESSAGE);
  
  printMessage(DIAG_DONE_MESSAGE);
}
