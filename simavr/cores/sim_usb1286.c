/*
	sim_90usb1286.c

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

#include "sim_avr.h"
#include "avr_eeprom.h"
#include "avr_flash.h"
#include "avr_watchdog.h"
#include "avr_extint.h"
#include "avr_ioport.h"
#include "avr_uart.h"
#include "avr_adc.h"
#include "avr_timer.h"
#include "avr_spi.h"
#include "avr_usb.h"
#include "avr_acomp.h"

#define USBRF 5 // missing in avr/iom32u4.h
#define RAMPZ   _SFR_IO8(0x3B)

void usb1286_init(struct avr_t * avr);
void usb1286_reset(struct avr_t * avr);

#define _AVR_IO_H_
#define __ASSEMBLER__
#include "avr/iousb1286.h"

#include "sim_core_declare.h"

const struct mcu_t {
	avr_t			 core;
	avr_eeprom_t 	eeprom;
	avr_flash_t 	selfprog;
	avr_watchdog_t	watchdog;
	avr_extint_t	extint;
	avr_ioport_t	portb, portc, portd;
	avr_uart_t		uart1;
	avr_timer_t		timer0, timer1, timer3;
	avr_spi_t		spi;
	avr_usb_t		usb;
	avr_acomp_t		acomp;
} mcu_usb1286 = {
	.core = {
		.mmcu = "at90usb1286",
		DEFAULT_CORE(4),

		.init = usb1286_init,
		.reset = usb1286_reset,

        .rampz = RAMPZ, // extended program memory access
	},
	AVR_EEPROM_DECLARE(EE_READY_vect),
	AVR_SELFPROG_DECLARE(SPMCSR, SPMEN, SPM_READY_vect),
	AVR_WATCHDOG_DECLARE(WDTCSR, WDT_vect),
    /*
	.extint = {
		AVR_EXTINT_MEGA_DECLARE(0, 'D', PD0, A),
		AVR_EXTINT_MEGA_DECLARE(1, 'D', PD1, A),
		AVR_EXTINT_MEGA_DECLARE(2, 'D', PD2, A),
		AVR_EXTINT_MEGA_DECLARE(3, 'D', PD3, A),
		AVR_EXTINT_MEGA_DECLARE(4, 'C', PC7, B),
		AVR_EXTINT_MEGA_DECLARE(5, 'D', PD4, B),
		AVR_EXTINT_MEGA_DECLARE(6, 'D', PD6, B),
		AVR_EXTINT_MEGA_DECLARE(7, 'D', PD7, B),
	},
	.portb = {
		.name = 'B', .r_port = PORTB, .r_ddr = DDRB, .r_pin = PINB,
		.pcint = {
			.enable = AVR_IO_REGBIT(PCICR, PCIE0),
			.raised = AVR_IO_REGBIT(PCIFR, PCIF0),
			.vector = PCINT0_vect,
		},
		.r_pcint = PCMSK0,
	},
    */
	AVR_IOPORT_DECLARE(b, 'B', B),
	AVR_IOPORT_DECLARE(c, 'C', C),
	AVR_IOPORT_DECLARE(d, 'D', D),

	AVR_UARTX_DECLARE(1, PRR1, PRUSART1),

	.timer0 = {
		.name = '0',
		.disabled = AVR_IO_REGBIT(PRR0, PRTIM0),
		.wgm = { AVR_IO_REGBIT(TCCR0A, WGM00), AVR_IO_REGBIT(TCCR0A, WGM01), AVR_IO_REGBIT(TCCR0B, WGM02) },
		.wgm_op = {
			[0] = AVR_TIMER_WGM_NORMAL8(),
			[1] = AVR_TIMER_WGM_FCPWM8(),
			[2] = AVR_TIMER_WGM_CTC(),
			[3] = AVR_TIMER_WGM_FASTPWM8(),
			[7] = AVR_TIMER_WGM_OCPWM(),
		},
		.cs = { AVR_IO_REGBIT(TCCR0B, CS00), AVR_IO_REGBIT(TCCR0B, CS01), AVR_IO_REGBIT(TCCR0B, CS02) },
		.cs_div = { 0, 0, 3 /* 8 */, 6 /* 64 */, 8 /* 256 */, 10 /* 1024 */, AVR_TIMER_EXTCLK_CHOOSE, AVR_TIMER_EXTCLK_CHOOSE },
		.ext_clock_pin = AVR_IO_REGBIT(PORTD, 7), /* External clock pin */

		.r_tcnt = TCNT0,

		.overflow = {
			.enable = AVR_IO_REGBIT(TIMSK0, TOIE0),
			.raised = AVR_IO_REGBIT(TIFR0, TOV0),
			.vector = TIMER0_OVF_vect,
		},
		.comp = {
			[AVR_TIMER_COMPA] = {
				.r_ocr = OCR0A,
				.com = AVR_IO_REGBITS(TCCR0A, COM0A0, 0x3),
				.com_pin = AVR_IO_REGBIT(PORTB, 7),
				.interrupt = {
					.enable = AVR_IO_REGBIT(TIMSK0, OCIE0A),
					.raised = AVR_IO_REGBIT(TIFR0, OCF0A),
					.vector = TIMER0_COMPA_vect,
				},
			},
			[AVR_TIMER_COMPB] = {
				.r_ocr = OCR0B,
				.com = AVR_IO_REGBITS(TCCR0A, COM0B0, 0x3),
				.com_pin = AVR_IO_REGBIT(PORTD, 0),
				.interrupt = {
					.enable = AVR_IO_REGBIT(TIMSK0, OCIE0B),
					.raised = AVR_IO_REGBIT(TIFR0, OCF0B),
					.vector = TIMER0_COMPB_vect,
				}
			}
		}
	},
	.timer1 = {
		.name = '1',
		.disabled = AVR_IO_REGBIT(PRR0,PRTIM1),
		.wgm = { AVR_IO_REGBIT(TCCR1A, WGM10), AVR_IO_REGBIT(TCCR1A, WGM11),
					AVR_IO_REGBIT(TCCR1B, WGM12), AVR_IO_REGBIT(TCCR1B, WGM13) },
		.wgm_op = {
			[0] = AVR_TIMER_WGM_NORMAL16(),
			[4] = AVR_TIMER_WGM_CTC(),
			[5] = AVR_TIMER_WGM_FASTPWM8(),
			[6] = AVR_TIMER_WGM_FASTPWM9(),
			[7] = AVR_TIMER_WGM_FASTPWM10(),
			[12] = AVR_TIMER_WGM_ICCTC(),
			[14] = AVR_TIMER_WGM_ICPWM(),
			[15] = AVR_TIMER_WGM_OCPWM(),
		},
		.cs = { AVR_IO_REGBIT(TCCR1B, CS10), AVR_IO_REGBIT(TCCR1B, CS11), AVR_IO_REGBIT(TCCR1B, CS12) },
		.cs_div = { 0, 0, 3 /* 8 */, 6 /* 64 */, 8 /* 256 */, 10 /* 1024 */, AVR_TIMER_EXTCLK_CHOOSE, AVR_TIMER_EXTCLK_CHOOSE },
		.ext_clock_pin = AVR_IO_REGBIT(PORTB, 4), /* External clock pin */

		.r_tcnt = TCNT1L,
		.r_tcnth = TCNT1H,
		.r_icr = ICR1L,
		.r_icrh = ICR1H,

		.ices = AVR_IO_REGBIT(TCCR1B, ICES1),
		.icp = AVR_IO_REGBIT(PORTB, 0),

		.overflow = {
			.enable = AVR_IO_REGBIT(TIMSK1, TOIE1),
			.raised = AVR_IO_REGBIT(TIFR1, TOV1),
			.vector = TIMER1_OVF_vect,
		},
		.icr = {
			.enable = AVR_IO_REGBIT(TIMSK1, ICIE1),
			.raised = AVR_IO_REGBIT(TIFR1, ICF1),
			.vector = TIMER1_CAPT_vect,
		},
		.comp = {
			[AVR_TIMER_COMPA] = {
				.r_ocr = OCR1AL,
				.r_ocrh = OCR1AH,	// 16 bits timers have two bytes of it
				.com = AVR_IO_REGBITS(TCCR1A, COM1A0, 0x3),
				.com_pin = AVR_IO_REGBIT(PORTB, 1),
				.interrupt = {
					.enable = AVR_IO_REGBIT(TIMSK1, OCIE1A),
					.raised = AVR_IO_REGBIT(TIFR1, OCF1A),
					.vector = TIMER1_COMPA_vect,
				},
			},
			[AVR_TIMER_COMPB] = {
				.r_ocr = OCR1BL,
				.r_ocrh = OCR1BH,
				.com = AVR_IO_REGBITS(TCCR1A, COM1B0, 0x3),
				.com_pin = AVR_IO_REGBIT(PORTB, 2),
				.interrupt = {
					.enable = AVR_IO_REGBIT(TIMSK1, OCIE1B),
					.raised = AVR_IO_REGBIT(TIFR1, OCF1B),
					.vector = TIMER1_COMPB_vect,
				},
			},
		},
	},
    .timer3 = {
        .name = '3',
        .wgm = { AVR_IO_REGBIT(TCCR3A, WGM30), AVR_IO_REGBIT(TCCR3A, WGM31),
                    AVR_IO_REGBIT(TCCR3B, WGM32), AVR_IO_REGBIT(TCCR3B, WGM33) },
        .wgm_op = {
            [0] = AVR_TIMER_WGM_NORMAL16(),
            // TODO: 1 PWM phase correct 8bit
            //       2 PWM phase correct 9bit
            //       3 PWM phase correct 10bit
            [4] = AVR_TIMER_WGM_CTC(),
            [5] = AVR_TIMER_WGM_FASTPWM8(),
            [6] = AVR_TIMER_WGM_FASTPWM9(),
            [7] = AVR_TIMER_WGM_FASTPWM10(),
            // TODO: 8 PWM phase and freq correct ICR
            //       9 PWM phase and freq correct OCR
            //       10
            //       11
            [12] = AVR_TIMER_WGM_ICCTC(),
            [14] = AVR_TIMER_WGM_ICPWM(),
            [15] = AVR_TIMER_WGM_OCPWM(),
        },
        .cs = { AVR_IO_REGBIT(TCCR3B, CS30), AVR_IO_REGBIT(TCCR3B, CS31), AVR_IO_REGBIT(TCCR3B, CS32)  },
        .cs_div = { 0, 0, 3 /* 8 */, 6 /* 64 */, 8 /* 256 */, 10 /* 1024 */, AVR_TIMER_EXTCLK_CHOOSE,  AVR_TIMER_EXTCLK_CHOOSE },
        .ext_clock_pin = AVR_IO_REGBIT(PORTE, 6), /* External clock pin */

        .r_tcnt = TCNT3L,
        .r_icr = ICR3L,
        .r_icrh = ICR3H,
        .r_tcnth = TCNT3H,

        .ices = AVR_IO_REGBIT(TCCR3B, ICES3),
        .icp = AVR_IO_REGBIT(PORTE, 7),

        .overflow = {
            .enable = AVR_IO_REGBIT(TIMSK3, TOIE3),
            .raised = AVR_IO_REGBIT(TIFR3, TOV3),
            .vector = TIMER3_OVF_vect,
        },
        .icr = {
            .enable = AVR_IO_REGBIT(TIMSK3, ICIE3),
            .raised = AVR_IO_REGBIT(TIFR3, ICF3),
            .vector = TIMER3_CAPT_vect,
        },

        .comp = {
            [AVR_TIMER_COMPA] = {
                .r_ocr = OCR3AL,
                .r_ocrh = OCR3AH,   // 16 bits timers have two bytes of it
                .com = AVR_IO_REGBITS(TCCR3A, COM3A0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTE, PE3),
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK3, OCIE3A),
                    .raised = AVR_IO_REGBIT(TIFR3, OCF3A),
                    .vector = TIMER3_COMPA_vect,
                }
            },
            [AVR_TIMER_COMPB] = {
                .r_ocr = OCR3BL,
                .r_ocrh = OCR3BH,
                .com = AVR_IO_REGBITS(TCCR3A, COM3B0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTE, PE4),
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK3, OCIE3B),
                    .raised = AVR_IO_REGBIT(TIFR3, OCF3B),
                    .vector = TIMER3_COMPB_vect,
                }
            },
            [AVR_TIMER_COMPC] = {
                .r_ocr = OCR3CL,
                .r_ocrh = OCR3CH,
                .com = AVR_IO_REGBITS(TCCR3A, COM3C0, 0x3),
                .com_pin = AVR_IO_REGBIT(PORTE, PE5),
                .interrupt = {
                    .enable = AVR_IO_REGBIT(TIMSK3, OCIE3C),
                    .raised = AVR_IO_REGBIT(TIFR3, OCF3C),
                    .vector = TIMER3_COMPC_vect,
                }
            }
        },
    },
	AVR_SPI_DECLARE(0, 0),
	.usb = {
		.name = '1',
		.disabled = AVR_IO_REGBIT(PRR1, PRUSB),
		.usbrf = AVR_IO_REGBIT(MCUSR, USBRF),
		.r_usbcon = USBCON,
		.r_pllcsr = PLLCSR,
		.usb_com_vect = USB_COM_vect,
		.usb_gen_vect = USB_GEN_vect,
	},
	.acomp = {
		.r_acsr = ACSR,
		.acis = { AVR_IO_REGBIT(ACSR, ACIS0), AVR_IO_REGBIT(ACSR, ACIS1) },
		.acic = AVR_IO_REGBIT(ACSR, ACIC),
		.aco = AVR_IO_REGBIT(ACSR, ACO),
		.acbg = AVR_IO_REGBIT(ACSR, ACBG),
		.disabled = AVR_IO_REGBIT(ACSR, ACD),

		.timer_name = '1',

		.ac = {
			.enable = AVR_IO_REGBIT(ACSR, ACIE),
			.raised = AVR_IO_REGBIT(ACSR, ACI),
			.vector = ANALOG_COMP_vect,
		}
	}
};

static avr_t * make()
{
	return avr_core_allocate(&mcu_usb1286.core, sizeof(struct mcu_t));
}

avr_kind_t usb1286 = {
	.names = {"at90usb1286"},
	.make = make
};

void usb1286_init(struct avr_t * avr)
{
	struct mcu_t * mcu = (struct mcu_t*)avr;

	avr_eeprom_init(avr, &mcu->eeprom);
	avr_flash_init(avr, &mcu->selfprog);
	avr_extint_init(avr, &mcu->extint);
	avr_watchdog_init(avr, &mcu->watchdog);
	avr_ioport_init(avr, &mcu->portb);
	avr_ioport_init(avr, &mcu->portc);
	avr_ioport_init(avr, &mcu->portd);
	avr_uart_init(avr, &mcu->uart1);
	avr_timer_init(avr, &mcu->timer0);
	avr_timer_init(avr, &mcu->timer1);
	avr_timer_init(avr, &mcu->timer3);
	avr_spi_init(avr, &mcu->spi);
	//avr_usb_init(avr, &mcu->usb);
	avr_acomp_init(avr, &mcu->acomp);
}

void usb1286_reset(struct avr_t * avr)
{
//	struct mcu_t * mcu = (struct mcu_t*)avr;
}
