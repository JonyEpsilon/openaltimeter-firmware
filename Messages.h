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

#ifndef MESSAGES_H
#define MESSAGES_H

#include "WProgram.h"
#include "config.h"
#include <avr/pgmspace.h>

#define WELCOME_MESSAGE 0
#define DATASTORE_SETUP_MESSAGE 1
#define DONE_MESSAGE 2
#define ERASING_MESSAGE 3
#define LOGGING_DISABLED_MESSAGE 4
#define LOGGING_ENABLED_MESSAGE 5
#define NUM_FILES_MESSAGE 6
#define NUM_ENTRIES_MESSAGE 7
#define MAX_ENTRIES_MESSAGE 8
#define DIAG_RUN_MESSAGE 9
#define DIAG_DONE_MESSAGE 10
#define FLASH_MANU_MESSAGE 11
#define FLASH_TEST_MESSAGE 12
#define WRITING_MESSAGE 13
#define READING_MESSAGE 14
#define TEST_PASS_MESSAGE 15
#define BATTERY_TEST_MESSAGE 16
#define BATTERY_MESSAGE 17
#define BEEPER_TEST_MESSAGE 18
#define ENTRY_SIZE_MESSAGE 19
#define ALTIMETER_BASE_PRESSURE_MESSAGE 20
#define LIPO_CELLS_MESSAGE 21
#define START_LVA_MESSAGE 22
#define STOP_LVA_MESSAGE 23
#define START_LMA_MESSAGE 24
#define STOP_LMA_MESSAGE 25
#define FAKING_FLIGHT_MESSAGE 26
#define FLASH_FULL_MESSAGE 27
#define TEST_FAIL_MESSAGE 28
#define DATA_FORMAT_MESSAGE 29
#define WIPE_SETTINGS_MESSAGE 30
#define SETTINGS_TEST_MESSAGE 31
#define SETTINGS_FORMAT_MESSAGE 32
#define SETTINGS_LOG_INTERVAL_MESSAGE 33
#define SETTINGS_HEIGHT_UNITS_MESSAGE 34
#define SETTINGS_BATTERY_TYPE_MESSAGE 35
#define SETTINGS_NO_BATTERY_MESSAGE 36
#define SETTINGS_LIPO_BATTERY_MESSAGE 37
#define SETTINGS_NIMH_BATTERY_MESSAGE 38
#define SETTINGS_LOW_VOLTAGE_THRESHOLD_MESSAGE 39
#define SETTINGS_BATTERY_MONITOR_CALIBRATION_MESSAGE 40
#define SETTINGS_LOG_SERVO_MESSAGE 41
#define SETTINGS_MID_POSITION_MESSAGE 42
#define SETTINGS_ON_POSITION_MESSAGE 43
#define SETTINGS_MAX_HEIGHT_MESSAGE 44
#define SETTINGS_MAX_LAUNCH_HEIGHT_MESSAGE 45
#define SETTINGS_LAUNCH_WINDOW_END_HEIGHT_MESSAGE 46
#define SETTINGS_BATTERY_VOLTAGE_MESSAGE 47
#define SETTINGS_DO_NOTHING_MESSAGE 48
#define OUTPUT_MAX_HEIGHT_MESSAGE 49
#define OUTPUT_MAX_LAUNCH_HEIGHT_MESSAGE 50
#define OUTPUT_LAUNCH_WINDOW_END_HEIGHT_MESSAGE 51
#define OUTPUT_BATTERY_VOLTAGE_MESSAGE 52


void printMessage(int messageIndex);

#endif /*MESSAGES_H*/
