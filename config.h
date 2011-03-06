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

#include "notes.h"

//#define SHHHH

// ** Hardware definitions **
#define BMP085_XCLR_PIN 17
#define BMP085_EOC_PIN 16
#define BEEPER_PIN 3
#define BATTERY_ANALOG_PIN 6
#define AT25DF_SS_PIN 10
#define RADIO_INPUT_PIN 8
#define SERVO_INPUT_PIN 9

// ** Configuration **
// -- pressure logging
#define ALTIMETER_SOFT_OVERSAMPLE 20
#define ALTIMETER_BASE_PRESSURE_SAMPLES 200
#define LOG_INTERVAL_MS_DEFAULT 500
// Default height units, in case no valid settings are found: 3.281 for feet, 1.0 for metres. Defaults to feet.
#define HEIGHT_UNITS_DEFAULT 3.281

// -- launch detector
// these parameters tune the launch detector
// this is the rate of climb that is considered a launch. It's measured in m/s.
#define LAUNCH_CLIMB_THRESHOLD 3.0
// this is how long the climb intervals have to exceed the launch climb rate to
// trigger the launch detector, measured in ms
#define LAUNCH_CLIMB_TIME 1500
// this is how many samples to seek back after the launch was detected to find the
// minimum height
#define LAUNCH_SEEKBACK_SAMPLES 20

// -- low voltage alarm
// default LVA threshold if no valid settings are found.
#define LOW_VOLTAGE_THRESHOLD_DEFAULT 4.7
// when the battery alarm goes off, the voltage will need to come up to the threshold plus this voltage
// before the alarm will switch off. This stops the alarm from switching on and off repeatedly when the
// voltage is very close to threshold.
#define BATTERY_MONITOR_HYSTERESIS 0.2
// --- lipo options
// this is the voltage that will be used to distinguish between 2s and 3s packs.
#define LIPO_CELL_DETECT_THRESHOLD 8.6

// -- radio control
// these define the servo pulse lengths that define the switch position. They are in us.
#define RADIO_MID_THRESHOLD_LOW 1400
#define RADIO_MID_THRESHOLD_HIGH 1600

// -- tunes and alarms
// startup tune
#define STARTUP_TUNE { NOTE_C7, 2, NOTE_D7, 2, NOTE_E7, 2, NOTE_F7, 2, NOTE_G7, 2, TUNE_END }
// alarms { NOTE_G7, 8, NOTE_REST, 8, TUNE_LOOP }
#define LOW_VOLTAGE_TUNE { BEEPER_BEEP_FREQUENCY, 4, NOTE_REST, 4, TUNE_LOOP }
#define LOST_MODEL_TUNE { BEEPER_BEEP_FREQUENCY, 16, NOTE_REST, 4, BEEPER_BEEP_FREQUENCY, 16, NOTE_REST, 4, BEEPER_BEEP_FREQUENCY, 16, NOTE_REST, 100, TUNE_LOOP }
#define SETTINGS_SAVE_TUNE { NOTE_C7, 2, NOTE_E7, 2, NOTE_C7, 2, NOTE_G7, 2, TUNE_END }
#define BEEPER_BEEP_FREQUENCY 3000
#define BEEPER_INTEGER_TONE_DURATION 5
#define BEEPER_INTEGER_REST_DURATION 5
#define BEEPER_INTEGER_PAUSE_DURATION 15

// -- test settings
#define NUMBER_OF_TEST_LOGS 200
