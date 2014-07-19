/**
@mainpage AVR XBee Node test program
@version 0.1
@author Ken Sarkies (www.jiggerjuice.net)
@date 18 July 2014

@brief Code for the power drain testing of a node using XBee and
1MHz Atmel MCU ATTiny4313.

@details
This is a series of tests to determine the power drain of the AVR in various
modes and with various peripherals powered off.

1. All peripherals off, AVR ticking over.

@note
Compiler: avr-gcc (GCC) 4.8.2
@note
Target: ATTiny4313
@note
Tested: ATTiny4313 at 1.0MHz

*/
/****************************************************************************
 *   Copyright (C) 2014 by Ken Sarkies                                      *
 *   ksarkies@trinity.asn.au                                                *
 *                                                                          *
 *   This file is part of XBee-acquisition                                  *
 *                                                                          *
 *   serial-programmer is free software; you can redistribute it and/or     *
 *   modify it under the terms of the GNU General Public License as         *
 *   published by the Free Software Foundation; either version 2 of the     *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   XBee-acquisition  is distributed in the hope that it will be useful,   *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with XBee-acquisition. If not, write to the Free Software        *
 *   Foundation, Inc., http://www.fsf.org/                                  *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.              *
 ***************************************************************************/

#include <inttypes.h>
#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

// Definitions of microcontroller registers and other characteristics
#define	_ATtiny4313
#ifdef	__ICCAVR__
#include "iotn4313.h"
#elif	__GNUC__
#include <avr/io.h>
#endif

#define TRUE 1
#define FALSE 0
/* Convenience macros (we don't use them all) */
#define  _BV(bit) (1 << (bit))
#define  inb(sfr) _SFR_BYTE(sfr)
#define  inw(sfr) _SFR_WORD(sfr)
#define  outb(sfr, val) (_SFR_BYTE(sfr) = (val))
#define  outw(sfr, val) (_SFR_WORD(sfr) = (val))
#define  cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define  sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define  high(x) ((uint8_t) (x >> 8) & 0xFF)
#define  low(x) ((uint8_t) (x & 0xFF))

#include "xbee-node-test.h"

/*---------------------------------------------------------------------------*/
int main(void)
{

/*  Initialise hardware  */
#if (TEST<4)
/* Set PRR to disable all peripherals.
Set input ports to pullups and disable digital input buffers on AIN inputs. */

    outb(PRR,0x0F);     /* power down timer/counters, USI, USART */
    outb(DDRA,0);       /* set as inputs */
    outb(PORTA,0x07);   /* set pullups   */
    outb(DDRB,0);       /* set as inputs */
    outb(PORTB,0xFF);   /* set pullups   */
    outb(DDRD,0);       /* set as inputs */
    outb(PORTD,0x1F);   /* set pullups   */
    sbi(ACSR,ACD);      /* turn off Analogue Comparator */
    outb(DIDR,3);       /* turn off digital input buffers */
    wdt_disable();      /* watchdog timer turn off */

/* Brownout detector must be disabled in fuses */

#endif

#if (TEST==2)
    cbi(PRR,PRUSART);   /* Power up the USART */
#elif (TEST==3)
/* This locks the CPU by disabling all interrupts. */
    wdt_enable(9);     /* watchdog timer turn on */
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    cli();
    sleep_enable();
    sleep_cpu();
#endif

/*---------------------------------------------------------------------------*/
/* Main loop forever - do nothing. */
    {
        for(;;)
        {
        }
    }
}


