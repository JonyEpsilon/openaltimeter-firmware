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
#include "Messages.h"

#include "WProgram.h"

// The messages are collected together here to make it easier to store them in flash.
// This is a bit awkward, as we have to store them all in a table and then index them
// using defines. I don't know of a better way to do it.

// this must be longer than the longest message
#define MESSAGE_BUFFER_LENGTH 80

// the welcome message prints the build version. This should correspond to the tag in the SVN repository.
// The first word of this string must be openaltimeter, as it's what the desktop app uses to 
// verify that it's connected.
char _m0[] PROGMEM = "openaltimeter firmware version: beta6.\n";
char _m1[] PROGMEM = "Setting up datastore ...";
char _m2[] PROGMEM = " done.\n";
char _m3[] PROGMEM = "Erasing ... ";
char _m4[] PROGMEM = "Logging disabled.\n";
char _m5[] PROGMEM = "Logging enabled.\n";
char _m6[] PROGMEM = "Number of files: ";
char _m7[] PROGMEM = "Number of entries: ";
char _m8[] PROGMEM = "Max entries: ";
char _m9[] PROGMEM = "Running diagnostic tests ... \n";
char _m10[] PROGMEM = "Diagnostic tests done.\n";
char _m11[] PROGMEM = "Flash manufacturer info: ";
char _m12[] PROGMEM = "Testing flash.\n";
char _m13[] PROGMEM = "Writing ... ";
char _m14[] PROGMEM = "Reading ... ";
char _m15[] PROGMEM = "Test: PASSED.\n";
char _m16[] PROGMEM = "Testing battery monitor.\n";
char _m17[] PROGMEM = "Battery: ";
char _m18[] PROGMEM = "Testing beeper ... ";
char _m19[] PROGMEM = "Size of log entry : ";
char _m20[] PROGMEM = "Setting altimeter base pressure: ";
char _m21[] PROGMEM = "LIPO - number of cells: ";
char _m22[] PROGMEM = "Sounding low voltage alarm.\n";
char _m23[] PROGMEM = "Stopping low voltage alarm.\n";
char _m24[] PROGMEM = "Sounding lost model alarm.\n";
char _m25[] PROGMEM = "Stopping lost model alarm.\n";
char _m26[] PROGMEM = "Faking flight ... ";
char _m27[] PROGMEM = "Flash memory full.\n";
char _m28[] PROGMEM = "Test: FAILED.\n";
// the data format message can be used by the downloader app to parse the downloaded
// data correctly.
char _m29[] PROGMEM = "Data format: V1";

// This table must include all the messages you want to use.
// The defines in the header refer to the index in this table. You
// need to make sure that you keep them in sync. Messy, I know.
PGM_P _messages[] PROGMEM = 
{
  _m0, _m1, _m2, _m3, _m4, _m5, _m6, _m7, _m8, _m9, _m10, _m11, _m12, _m13, _m14, _m15,
  _m16, _m17, _m18, _m19, _m20, _m21, _m22, _m23, _m24, _m25, _m26, _m27, _m28, _m29
};

char _messageBuffer[MESSAGE_BUFFER_LENGTH];

void printMessage(int messageIndex)
{
  strcpy_P(_messageBuffer, (PGM_P)pgm_read_word(&(_messages[messageIndex])));
  Serial.print(_messageBuffer);
}
