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

#ifndef BEEPER_H
#define BEEPER_H

#include "WProgram.h"
#include "config.h"

// this is the basic unit of tune length, in microseconds
#define TUNE_BASE_PERIOD 25000

// the beeper is a namespace, rather than a class, as otherwise it gets messy trying to use ISRs
namespace Beeper
{
  extern void setup(int digitalPin);
  extern void beep(uint16_t frequency, uint16_t duration);
  extern void playTune(int16_t* tune);
  extern void stopTune();
  extern void waitForTuneToEnd();
  extern void outputInteger(int integer);
  extern void test();
};

#endif /*BEEPER_H*/
