/**
@mainpage Watermeter Echo Test
@version 0.0.0
@author Ken Sarkies (www.jiggerjuice.info)
@date 19 April 2016
@brief Code for an ATTiny841 AVR

This simply echoes characters back from the watermeter AVR to test the operation
of the serial port.

The testboard must be used to pass serial signals. The XBee must not be present.

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
#include <util/delay.h>
#include "../../libs/serial.h"
#include "../../libs/defines.h"
#include "watermeter-echo-test.h"

/*****************************************************************************/
/* Global Variables */

/* Local Prototypes */

static void inline hardwareInit(void);

/*****************************************************************************/
/** @brief Main Program */

int main(void)
{
    wdt_disable();          /* Stop watchdog timer */
    hardwareInit();         /* Initialize the processor specific hardware */
    uartInit();             /* Baud rate is given in defines.h */

/* Main loop */
    for(;;)
    {

/* Wait for data to appear */
        uint16_t inputChar = getchDirect();
        uint8_t messageError = high(inputChar);
        if (messageError != NO_DATA)
        {
            sendchDirect(low(inputChar));
        }
    }
}

/****************************************************************************/
/** @brief Initialize the hardware for process measurement and XBee control

*/
void hardwareInit(void)
{
/* Hold battery monitor control low for lower power drain. */
    cbi(VBATCON_PORT_DIR,VBATCON_PIN);
    cbi(VBATCON_PORT,VBATCON_PIN);      /* Unset pullup */
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
