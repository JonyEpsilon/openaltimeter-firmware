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
#include "Beeper.h"
#include "WProgram.h"

#include "Messages.h"

#include <util/delay.h>

int _beeperDigitalPin;
int16_t* _beeperTuneData;
int16_t _beeperTunePointer;
int16_t _beeperTuneCountdown;
boolean _beeperTunePlaying;

void Beeper::setup(int digitalPin)
{
  _beeperDigitalPin = digitalPin;
  pinMode(_beeperDigitalPin, OUTPUT);
  digitalWrite(_beeperDigitalPin, LOW);
}

void Beeper::beep(uint16_t frequency, uint16_t duration)
{
  tone(_beeperDigitalPin, frequency, duration);
}

void Beeper::playTune(int16_t* tune)
{
  _beeperTunePlaying = true;
  // store a reference to the tune
  _beeperTuneData = tune;
  // start the tune
  tone(_beeperDigitalPin, tune[0]);
  _beeperTunePointer = 0;
  _beeperTuneCountdown = tune[1];
  // set up timer 1 to call our interrupt handler periodically
  // no output compare matching, counter mode: phase and frequency correct, ICR1
  // prescaler divides by 64, so 1/8MHz clock
  TCCR1A = 0;
  TCCR1B = _BV(WGM13) | _BV(CS11) | _BV(CS10);
  // set the timer period. Base period specified in us, counter period is 8us, and
  // in this mode the counter counts up to ICR1 and back down before triggering the
  // interrupt.
  ICR1 = TUNE_BASE_PERIOD / (8 * 2);
  // enable the interrupt
  TIMSK1 = _BV(TOIE1);
}

ISR(TIMER1_OVF_vect)
{
  // check whether it's time to update the tune
  if ((--_beeperTuneCountdown) == 0)
  {
    _beeperTunePointer++;
    // has the tune finished?
    if (_beeperTuneData[2 * _beeperTunePointer] == TUNE_END)
    {
      Beeper::stopTune();
      return;
    }
    // should we loop the tune?
    if (_beeperTuneData[2 * _beeperTunePointer] == TUNE_LOOP)
    {
      // reset the tune pointer and set the counter such that the next ISR call will change the tone.
      _beeperTunePointer = -1;
      _beeperTuneCountdown = 1;
      return;
    }
    // not ended, not looping, so output the next note and restart the countdown
    int16_t nextNote =  _beeperTuneData[2 * _beeperTunePointer];
    if (nextNote == NOTE_REST) noTone(_beeperDigitalPin);
    else tone(_beeperDigitalPin, nextNote);
    _beeperTuneCountdown = _beeperTuneData[(2 * _beeperTunePointer) + 1];
  }
}


void Beeper::stopTune()
{
  // stop the sound and disable our interrupt.
  noTone(_beeperDigitalPin);
  TIMSK1 = 0;
  _beeperTunePlaying = false;
}

void Beeper::waitForTuneToEnd()
{
  while (_beeperTunePlaying) delay(10);
  delay(100);
}

void Beeper::outputInteger(int integer)
{
  Serial.print("Outputting ");
  Serial.println(integer);
  // 8 digits maximum
  char intString[8];
  sprintf(intString, "%i", integer);
  int intLength = strlen(intString);
  char digits[intLength];
  // there's probably some function in the c stdlib to do this, but I can't find it.
  // the - 48 converts from ASCII to decimal digits.
  for (int i = 0; i < intLength; i++) digits[i] = intString[i] - 48;
  // calculate the length of the "tune" that we'll output. Each digit requires (2 * n) steps (beep-rest)
  // and there's a long rest between digits, and an end marker. We need to take care of zero as a special
  // case (it has 8 steps).
  int toneLength = 0;
  for (int i = 0; i < intLength; i++)
  {
    if (digits[i] == 0) toneLength += 8;
    else toneLength += ((2 * digits[i]) + 1);
  }
  toneLength++;
  // now construct the tone sequence. The *2 is because each step is specified by a note and a duration.
  int16_t intTune[toneLength * 2];
  int tunePointer = 0;
  for (int i = 0; i < intLength; i++)
  {
    if (digits[i] != 0)
    {
      for (int j = 0; j < digits[i]; j++)
      {
        intTune[tunePointer++] = BEEPER_BEEP_FREQUENCY;
        intTune[tunePointer++] = BEEPER_INTEGER_TONE_DURATION;
        intTune[tunePointer++] = NOTE_REST;
        intTune[tunePointer++] = BEEPER_INTEGER_REST_DURATION;
      }
    }
    else
    {
        intTune[tunePointer++] = BEEPER_BEEP_FREQUENCY;
        intTune[tunePointer++] = BEEPER_INTEGER_TONE_DURATION / 2;
        intTune[tunePointer++] = NOTE_REST;
        intTune[tunePointer++] = BEEPER_INTEGER_REST_DURATION / 2;
        intTune[tunePointer++] = BEEPER_BEEP_FREQUENCY;
        intTune[tunePointer++] = BEEPER_INTEGER_TONE_DURATION / 2;
        intTune[tunePointer++] = NOTE_REST;
        intTune[tunePointer++] = BEEPER_INTEGER_REST_DURATION / 2;      
    }
    intTune[tunePointer++] = NOTE_REST;
    intTune[tunePointer++] = BEEPER_INTEGER_PAUSE_DURATION;
  }
  intTune[tunePointer++] = TUNE_END;
  playTune(intTune);
  waitForTuneToEnd(); 
  delay(300);
}

void Beeper::test()
{
  printMessage(BEEPER_TEST_MESSAGE);
  int16_t startupTune[] = STARTUP_TUNE;
  playTune(startupTune);
  waitForTuneToEnd();
//  outputInteger(345);
  printMessage(DONE_MESSAGE);
}
