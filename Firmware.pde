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
    case 'u':
      uploadLogEntry();
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
    addLogEntry(&le);
  }
}
// this part of the function is broken out to support data upload/flight simulation.
void addLogEntry(LogEntry* le)
{
  // update the height related quantities
  updateHeightMonitor(le);
  // store the entry
  if (datastore.addEntry(le))
  Serial.print(".");
  else
  {
    printMessage(FLASH_FULL_MESSAGE);
    stopLogging();
  }
}

// these functions track various height-related quantities. They implement the launch detectors, max height detector etc.
int32_t currentHeight = 0;
int32_t maxHeight = 0;                    // The overall maximum height of the flight.
void updateHeightMonitor(LogEntry* le)
{
  currentHeight = pressureSensor.convertToAltitude(le->getPressure(), settings.heightUnits);
  if (currentHeight > maxHeight) maxHeight = currentHeight;
  updateDLGHeightMonitor();
}

// DLG specific height functions and variables. This is broken out from the main height monitor to make the firmware
// easier to customise.
uint8_t launchCount = 0;                  // A launch is defined as N successive periods with more than a certain climb rate.
                                          // This keeps track of how long we've been climbing.
uint8_t launchWindowCount = 0;            // Used to track launch height separate from max height.
int32_t lastHeight = 0;                  // Used for measuring climb rates.
boolean launched = false;                 // This indicates whether we're in flight or not. Launch detector is disabled in flight.
int32_t maxLaunchHeight = 0;
int32_t launchWindowEndHeight = 0;       // It's useful to know what height was attained a few seconds after launch to optimise push over.
void updateDLGHeightMonitor()
{
  // We monitor the height data looking for a "launch". This is a number of samples that climb consistently at greater than a given rate.
  if (!launched)
  {
    if (currentHeight - lastHeight > LAUNCH_CLIMB_THRESHOLD * settings.heightUnits) launchCount++;
    else launchCount = 0;
    lastHeight = currentHeight;
    if (launchCount >= (LAUNCH_CLIMB_TIME / settings.logIntervalMS))
    {
      // we've just detected a launch - disable the launch detector
      launched = true;
      launchCount = 0;
      // When we detect a launch we do a few things: we reset the base pressure to the highest pressure in the few seconds before the launch;
      // we start a countdown which defines the "launch window"; we reset the maximum heights.
      // -- reset base pressure
      uint32_t newBasePressure = 0;
      datastore.startReverseRead();
      for (int i = LAUNCH_SEEKBACK_SAMPLES; i > 0; i--)
      {
        if (datastore.entryReverseAvailable())
        {
          LogEntry le;
          datastore.getPreviousEntry(&le);
          // we stop if we encounter a file boundary.
          // unlikely to happen, but worth checking for.
          if (le.isFileEndMarker()) break;
          if (le.getPressure() > newBasePressure) newBasePressure = le.getPressure();
        }      
      }
      pressureSensor.setBasePressure(newBasePressure);
      // -- time the launch window
      launchWindowCount = (LAUNCH_WINDOW_TIME / settings.logIntervalMS) + 1;
      // -- reset max heights
      maxHeight = 0;
      maxLaunchHeight = 0;
      launchWindowEndHeight = 0;
    }
  }
  else
  {
    // The launch detector is disabled after a launch, so that it can't retrigger in flight. It is reset by either the logger's altitude coming
    // below a certain threshold, or a height output function being commanded by the user (on the basis that this should always happen on the
    // ground - the latter is implemented to stop the logger getting stuck should the ground-level pressure change dramatically during a flight.)
    // Here we check for the former.
    if (currentHeight < LAUNCH_DETECTOR_REARM_HEIGHT * settings.heightUnits) launched = false;
  }
  // if we're in the launch window we need to track the maximum altitude.
  if (launchWindowCount > 0)
  {
    if (currentHeight > maxLaunchHeight) maxLaunchHeight = currentHeight;
    // if this is the end of the launch window then we record the height
    if (launchWindowCount == 1) launchWindowEndHeight = currentHeight;
    launchWindowCount--;
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

void outputHeight(int32_t h)
{
  stopLogging();
  //-- reset the launch detector
  launched = false;
  Serial.println(h);
  Beeper::outputInteger(h);
  delay(500);
  startLogging();
}

void outputMaxHeight()
{
  outputHeight(maxHeight);
}

void outputMaxLaunchHeight()
{
  outputHeight(maxLaunchHeight);
}

void outputLaunchWindowEndHeight()
{
  outputHeight(launchWindowEndHeight);
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

// the following function allows uploading data to the OA which is useful for debugging things
// like height detectors etc without having to take a trip to the field. The data is send as a string
// like "P: 101529 T: 2725 B: 755 S: 0*". To make the parsing easier the battery voltage and the temperature
// should be multiplied by 100 and passed as integers. Note that the string must end with an asterisk.
// If the pressure value is -1 the entry will be interpreted as a file end marker.
//
// The OA will process the data as if it was a new log entry coming in live (i.e. height detector will be
// updated etc).
void uploadLogEntry()
{ 
  char sBuffer[80];
  int bufPtr = 0;
  int nextChar;
  
  stopLogging();
  while (nextChar != '*')
  {
    nextChar = Serial.read();
    if (nextChar != -1) {
      sBuffer[bufPtr++] = nextChar;
    }
  }
  sBuffer[bufPtr++] = 0;
  int32_t pressure;
  int temperature;
  int battery;
  int servo;
  int servo2;
  int numRead = sscanf(sBuffer, "P: %ld T: %d B: %d S: %d", &pressure, &temperature, &battery, &servo);
  if (numRead != 4) return;
  if (pressure == -1)
  {
    datastore.addFileEndMarker();
    return;
  }
  else
  {
    LogEntry le;
    le.setPressure(pressure);
    le.setTemperature(temperature / 10);
    le.setBattery((float)battery / 100);
    le.setServo(servo);
    
    addLogEntry(&le);
  }
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
