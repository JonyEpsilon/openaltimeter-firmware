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
#include "Radio.h"
#include "WProgram.h"

#include "Messages.h"

#define RADIO_TIMEOUT 28000
#define RADIO_NOISE_THRESHOLD 75
#define RADIO_LOOP_TIMEOUT 5

Radio::Radio(int inputPin)
{
  _inputPin = inputPin;
}

void Radio::setup()
{
  pinMode(_inputPin, INPUT);
  // enable the pull-up to stop spurious triggering
  digitalWrite(_inputPin, HIGH);
}

uint16_t Radio::getRawValue()
{
  // The way that pulseIn is implemented means that it can generate spurious results when other
  // code is running in ISRs. We can't just disable interrupts during pulse in, as this would
  // break the Arduino millisecond time which needs an ISR to run on time to work.
  // As a workaround, we measure the input twice, and if they don't agree within some
  // tolerance we try again. This gives a little noise immunity too, FWIW.
  uint16_t diff = 1000;
  uint16_t rawVal;
  uint16_t loopCounter = 0;
  while (diff > RADIO_NOISE_THRESHOLD) {
    // if the radio data is very noisy then we'll never get a pair of pusles that agree within tolerance.
    // To stop this jamming the board in an infinite loop we update a counter, and break out of the loop,
    // returning zero, if we've gone around too many times
    if (loopCounter++ > RADIO_LOOP_TIMEOUT) return 0;
    uint16_t rawVal1 = pulseIn(_inputPin, HIGH, RADIO_TIMEOUT);
    // if the above pulseIn times out it will return zero. This usually means that the radio is disconnected.
    // In order to avoid wasting time measuring zero again, we break out and return.
    if (rawVal1 == 0) return 0;
    uint16_t rawVal2 = pulseIn(_inputPin, HIGH, RADIO_TIMEOUT);
    diff = abs(rawVal1 - rawVal2);
    rawVal = (rawVal1 + rawVal2) / 2;
  }  
  return rawVal;
}

uint8_t Radio::getState()
{
  uint16_t rawValue = getRawValue();
  if (rawValue < RADIO_MID_THRESHOLD_LOW) return RADIO_SWITCH_OFF;
  if (rawValue > RADIO_MID_THRESHOLD_HIGH) return RADIO_SWITCH_ON;
  return RADIO_SWITCH_MID;
}

// this gets the servo value as quickly as possible.
// It doesn't make any attempt at noise immunity, so some spurious values
// might be returned, especially if there is heavy interrupt use, like when
// a tune is playing.
uint16_t Radio::getServoValueQuick()
{
  return pulseIn(_inputPin, HIGH, RADIO_TIMEOUT);
}

void Radio::test()
{
  Serial.println("Testing radio ...");
  uint16_t r1 = getRawValue();
  Serial.println(r1);
  // temp code for testing the other radio channel
  uint16_t r2 = pulseIn(9, HIGH, RADIO_TIMEOUT);
  Serial.println(r2);
  // r1 should be between 990 and 1020, and r2 between 1480 and 1520 to pass the test
  if (r1 < 1120 && r1 > 1060 && r2 > 1060 && r2 < 1120) printMessage(TEST_PASS_MESSAGE);
  else printMessage(TEST_FAIL_MESSAGE);
}

