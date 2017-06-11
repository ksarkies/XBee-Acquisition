/**
@mainpage Serial Interrupt Library Test
@version 0.0.0
@author Ken Sarkies (www.jiggerjuice.info)
@date 8 June 2017
@brief Code for an ATTiny841/ATMega48 AVR

This firmware is used to test and debug the USART serial library used in the
XBee project. Characters sent to the port are echoed back.

@note
Software: AVR-GCC 4.8.2
@note
Target:   Any AVR with sufficient output ports and a timer
@note
Tested:   ATMega48 series, ATTiny841 at 8MHz internal clock.
 */
/****************************************************************************
 *   Copyright (C) 2017 by Ken Sarkies (www.jiggerjuice.info)               *
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

#include <string.h>
#include <avr/sfr_defs.h>
#include "../../libs/defines.h"
#include "../../libs/buffer.h"
#include "../../libs/serial.h"
#include <util/delay.h>
#include "serial-interrupt-test.h"

/*---------------------------------------------------------------------------*/
/* Global Variables */

/*---------------------------------------------------------------------------*/
/* Local Prototypes */

void hardwareInit(void);

/*---------------------------------------------------------------------------*/
/** @brief Main Program */

int main(void)
{
    hardwareInit();         /* Initialize the processor specific hardware */
    uartInit();
    sei();
    for (;;)
    {
        uint16_t ch = getchb();
        sendch((uint8_t) (ch & 0xFF));
        sendch('-');
    }   
}

/*---------------------------------------------------------------------------*/
/** @brief Initialize the hardware

*/
void hardwareInit(void)
{
}

