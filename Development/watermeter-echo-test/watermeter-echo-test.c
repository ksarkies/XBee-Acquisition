/**
@mainpage Watermeter Echo Test
@version 0.0.0
@author Ken Sarkies (www.jiggerjuice.info)
@date 19 April 2016
@brief Code for an ATTiny841 AVR

This simply echoes characters back from the watermeter AVR to test the operation
of the serial port.

@note
Software: AVR-GCC 4.8.2
@note
Target:   Any AVR with sufficient output ports and a timer
@note
Tested:   ATTiny841 at 8MHz internal clock.
 */
/****************************************************************************
 *   Copyright (C) 2013 by Ken Sarkies (www.jiggerjuice.info)               *
 *                                                                          *
 *   This file is part of XBee-Acquisition                                  *
 *                                                                          *
 * Licensed under the Apache License, Version 2.0 (the "License");          *
 * you may not use this file except in compliance with the License.         *
 * You may obtain a copy of the License at                                  *
 *                                                                          *
 *     http://www.apache.org/licenses/LICENSE-2.0                           *
 *                                                                          *
 * Unless required by applicable law or agreed to in writing, software      *
 * distributed under the License is distributed on an "AS IS" BASIS,        *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. *
 * See the License for the specific language governing permissions and      *
 * limitations under the License.                                           *
 ***************************************************************************/

#include <inttypes.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "../../libs/defines.h"
#include <util/delay.h>
#include "../../libs/serial.h"
#include "../../libs/timer.h"
#include "watermeter-echo-test.h"

/** Convenience macros (we don't use them all) */
#define TRUE 1
#define FALSE 0

#define  _BV(bit) (1 << (bit))
#define inb(sfr) _SFR_BYTE(sfr)
#define inw(sfr) _SFR_WORD(sfr)
#define outb(sfr, val) (_SFR_BYTE(sfr) = (val))
#define outw(sfr, val) (_SFR_WORD(sfr) = (val))
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define high(x) ((uint8_t) (x >> 8) & 0xFF)
#define low(x) ((uint8_t) (x & 0xFF))

/*****************************************************************************/
/* Global Variables */

/* Local Prototypes */

static void inline hardwareInit(void);

/*****************************************************************************/
/** @brief Main Program */

int main(void)
{
    wdt_disable();                  /* Stop watchdog timer */
    hardwareInit();                 /* Initialize the processor specific hardware */
    uartInit();

/* Main loop */
    for(;;)
    {

/* Wait for data to appear */
        uint16_t inputChar = getch();
        uint8_t messageError = high(inputChar);
        if (messageError != NO_DATA)
        {
            sendch(low(inputChar));
        }
    }
}

/****************************************************************************/
/** @brief Initialize the hardware for process measurement and XBee control

*/
void hardwareInit(void)
{
/* PB3 is XBee sleep request output. */
#ifdef SLEEP_RQ_PIN
    sbi(SLEEP_RQ_PORT_DIR,SLEEP_RQ_PIN);/* XBee Sleep Request */
    cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Set to keep XBee on */
#endif
/* PB4 is XBee on/sleep input. High is XBee on, low is asleep. */
#ifdef ON_SLEEP_PIN
    cbi(ON_SLEEP_PORT_DIR,ON_SLEEP_PIN);/* XBee On/Sleep Status input pin */
#endif
/* PB5 is XBee reset output. Pulse low to reset. */
#ifdef XBEE_RESET_PIN
    sbi(XBEE_RESET_PORT_DIR,XBEE_RESET_PIN);/* XBee Reset */
    sbi(XBEE_RESET_PORT,XBEE_RESET_PIN);    /* Set to keep XBee on */
#endif
/* PC0 is the board analogue input. */
/* PC1 is the battery monitor analogue input. */
/* PC5 is the battery monitor control output. Hold low for lower power drain. */
    cbi(VBAT_PORT_DIR,VBAT_PIN);
    cbi(VBAT_PORT,VBAT_PIN);            /* Unset pullup */
/* PD5 is the counter input. */
#ifdef COUNT_PIN
    cbi(COUNT_PORT_DIR,COUNT_PIN);      /* XBee counter input pin */
    sbi(COUNT_PORT,COUNT_PIN);          /* Set pullup */
#endif
/* General output for a LED to be activated by the mirocontroller as desired. */
#ifdef TEST_PORT_DIR
    sbi(TEST_PORT_DIR,TEST_PIN);
    sbi(TEST_PORT,TEST_PIN);            /* Set pullup */
#endif
}

/****************************************************************************/
