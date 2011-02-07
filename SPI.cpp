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
#include "SPI.h"
#include <avr/io.h>

#include "WProgram.h"

void SPI::setup()
{
  volatile char IOReg;
  // set SCLK and MOSI as outputs, MISO as input
  pinMode(SPI_MOSI_PIN, OUTPUT);
  pinMode(SPI_SCLK_PIN, OUTPUT);
  pinMode(SPI_MISO_PIN, INPUT);
  // enable the MISO pullup resistor
  digitalWrite(SPI_MISO_PIN, HIGH);
  // enable SPI in Master Mode with SCK = CK/4
  SPCR = _BV(SPE) | _BV(MSTR);
  // clear the SPI registers
  IOReg = SPSR;
  IOReg = SPDR;
}

void SPI::assertSS(int ssPin)
{
  digitalWrite(ssPin, LOW);
}

void SPI::deassertSS(int ssPin)
{
  digitalWrite(ssPin, HIGH);
}

unsigned char SPI::exchangeByte(unsigned char data)
{
    SPDR = data;
    loop_until_bit_is_set(SPSR, SPIF);
 
    return SPDR;
}

SPI Spi;
