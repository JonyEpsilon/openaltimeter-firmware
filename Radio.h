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

#ifndef RADIO_H
#define RADIO_H

#include "WProgram.h"
#include "config.h"

#define RADIO_SWITCH_OFF 0
#define RADIO_SWITCH_MID 1
#define RADIO_SWITCH_ON 2
// this value is never returned by the radio code, so can be used to ensure that
// a function can't be activated (used by the 2- and 3-position switch configuration
// code in Firmware.pde)
#define RADIO_SWITCH_IMPOSSIBLE 3

class Radio
{
  public:
    Radio(int inputPin);
    void setup();
    uint16_t getRawValue();
    uint8_t getState();
    uint16_t getServoValueQuick();
    void test();
  private:
    int _inputPin;
};

#endif /*RADIO_H*/
