/* vim: set sts=4:sw=4:ts=4:noexpandtab
	simusb.c

	Copyright 2012 Torbjorn Tyridal <ttyridal@gmail.com>

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

#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>

#include <pthread.h>

#include "sim_avr.h"
#include "avr_ioport.h"
#include "sim_elf.h"
#include "sim_hex.h"
#include "sim_gdb.h"
#include "sim_vcd_file.h"

avr_t * avr = NULL;
avr_vcd_t vcd_file;

int main(int argc, char *argv[])
{
	const char * pwd = dirname(argv[0]);

	avr = avr_make_mcu_by_name("at90usb1286");
	if (!avr) {
		fprintf(stderr, "%s: Error creating the AVR core\n", argv[0]);
		exit(1);
	}
	//avr->reset = NULL;
	avr_init(avr);
	avr->frequency = 16000000;
    avr->log = 11;

    {
        char path[1024];
        uint32_t base, size;
        snprintf(path, sizeof(path), "%s/../%s", pwd, "at90usb1286_test.hex");

        uint8_t * boot = read_ihex_file(path, &size, &base);
        if (!boot) {
            fprintf(stderr, "%s: Unable to load %s\n", argv[0], path);
            exit(1);
        }
        printf("Bootloader %04x: %d\n", base, size);
        memcpy(avr->flash + base, boot, size);
        free(boot);
        avr->pc = base;
        avr->codeend = avr->flashend;
    }

	// even if not setup at startup, activate gdb if crashing
	avr->gdb_port = 1234;
	if (0) {
		//avr->state = cpu_Stopped;
		avr_gdb_init(avr);
	}

    printf("starting\n");

     while (1) {
        int state = avr_run(avr);
        if ( state == cpu_Done || state == cpu_Crashed)
            break;
    }
}
