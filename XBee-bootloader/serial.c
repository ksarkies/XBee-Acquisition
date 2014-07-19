/*  Serial routines for the AVR/XBee bootloader */

/****************************************************************************
 *   Copyright (C) 2013 by Ken Sarkies ksarkies@internode.on.net            *
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

#include "defines.h"

/*-----------------------------------------------------------------------------*/
/* Initialise the UART, setting baudrate, Rx/Tx enables, and flow controls

Baud rate is derived from the header call to setbaud.h.
*/

void uartInit(void)
{
    BAUD_RATE_LOW_REG = UBRRL_VALUE;
    BAUD_RATE_HIGH_REG = UBRRH_VALUE;
    UART_FORMAT_REG = (3 << FRAME_SIZE);                // Set 8 bit frames
    UART_CONTROL_REG |= _BV(ENABLE_RECEIVER_BIT) |
                        _BV(ENABLE_TRANSMITTER_BIT);    // enable receive and transmit 
    cbi(UART_CTS_PORT_DIR,UART_CTS_PIN);                // Set flow control pins CTS input
    sbi(UART_RTS_PORT_DIR,UART_RTS_PIN);                // RTS output
    cbi(UART_RTS_PORT,UART_RTS_PIN);                    // RTS cleared to enable
}

/*-----------------------------------------------------------------------------*/
/* Send a character when the Tx is ready

*/

void sendch(unsigned char c)
{
        while (inb(UART_CTS_PORT) & _BV(UART_CTS_PIN));     // wait for clear to send
        UART_DATA_REG = c;                                  // send
        while (!(UART_STATUS_REG & _BV(TRANSMIT_COMPLETE_BIT)));    // wait till gone
        UART_STATUS_REG |= _BV(TRANSMIT_COMPLETE_BIT);      // reset TXCflag
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready

*/

unsigned char getch(void)
{
    cbi(UART_RTS_PORT,UART_RTS_PIN);                        // Enable RTS
    while (!(UART_STATUS_REG & _BV(RECEIVE_COMPLETE_BIT)));
    sbi(UART_RTS_PORT,UART_RTS_PIN);                        // Disable RTS
    return UART_DATA_REG;
}

