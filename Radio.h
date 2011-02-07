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

class Radio
{
  public:
    Radio(int inputPin);
    void setup();
    uint16_t getRawValue();
    uint8_t getState();
	void test();
  private:
    int _inputPin;
};

#endif /*RADIO_H*/
