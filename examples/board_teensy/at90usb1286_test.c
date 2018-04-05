/*
	atmega48_i2ctest.c

	Copyright 2008-2011 Michel Pollet <buserror@gmail.com>

 	This file is part of simavr.

	simavr is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	simavr is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with simavr.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// for linker, emulator, and programmer's sake
#include "avr_mcu_section.h"
AVR_MCU(F_CPU, "at90usb1286");

#include <stdio.h>

int main()
{
	sei();

    DDRB = 0xFF;

    uint8_t i = 0;
    while(1)
    {
        i += 1;
        PORTB = i;
        if(1 >= 200)
            break;
    }

	cli();

	sleep_mode();
}
