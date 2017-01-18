/*  Serial routines for the AVR UART

The hardware ports, registers and bit names are defined in the defines-*.h
files.
*/

/****************************************************************************
 *   Copyright (C) 2007 by Ken Sarkies (www.jiggerjuice.info)               *
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
#include "serial.h"

/*-----------------------------------------------------------------------------*/
/* Initialise the UART, setting baudrate, Rx/Tx enables, and flow controls

Baud rate is derived from the header call to the avr-libc setbaud.h.
*/

void uartInit(void)
{
    BAUD_RATE_LOW_REG = UBRRL_VALUE;
    BAUD_RATE_HIGH_REG = UBRRH_VALUE;
#if USE_2X
    sbi(UART_STATUS_REG,DOUBLE_RATE);
#else
    cbi(UART_STATUS_REG,DOUBLE_RATE);
#endif
    UART_FORMAT_REG = (3 << FRAME_SIZE);                /* Set 8 bit frames */
    UART_CONTROL_REG |= _BV(ENABLE_RECEIVER_BIT) |
                        _BV(ENABLE_TRANSMITTER_BIT);    /* enable receive and transmit */
#ifdef USE_HARDWARE_FLOW
    cbi(UART_CTS_PORT_DIR,UART_CTS_PIN);                /* Set flow control pins CTS input */
    sbi(UART_RTS_PORT_DIR,UART_RTS_PIN);                /* RTS output */
    cbi(UART_RTS_PORT,UART_RTS_PIN);                    /* RTS cleared to enable */
#endif
#ifdef USE_RECEIVE_INTERRUPTS
/* Enable USART receive complete interrupt */
    UART_CONTROL_REG |= _BV(RECEIVE_COMPLETE_IE);
#endif
#ifdef USE_TRANSMIT_INTERRUPTS
/* Enable USART receive complete interrupt */
    UART_CONTROL_REG |= _BV(TRANSMIT_COMPLETE_IE);
#endif
}

/*-----------------------------------------------------------------------------*/
/* Send a character when the Tx is ready

The function waits until CTS is asserted low then waits until the UART indicates
that the character has been sent.
*/

void sendch(unsigned char c)
{
#ifdef USE_HARDWARE_FLOW
        while (inb(UART_CTS_PORT) & _BV(UART_CTS_PIN));     /* wait for clear to send */
#endif
        UART_DATA_REG = c;                                  /* send */
        while (!(UART_STATUS_REG & _BV(TRANSMIT_COMPLETE_BIT)));    /* wait till gone */
        sbi(UART_STATUS_REG,TRANSMIT_COMPLETE_BIT);         /* force reset TXCflag */
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready (blocking)

The function asserts RTS low then waits for the receive complete bit to be set.
RTS is then cleared high. The character is then retrieved. If no character is
present, the function blocks.

returns: unsigned char. The received character.
*/

unsigned char getchb(void)
{
#ifdef USE_HARDWARE_FLOW
    cbi(UART_RTS_PORT,UART_RTS_PIN);                        /* Enable RTS */
#endif
#ifndef USE_INTERRUPTS
    while (!(UART_STATUS_REG & _BV(RECEIVE_COMPLETE_BIT)));
#endif
#ifdef USE_HARDWARE_FLOW
    sbi(UART_RTS_PORT,UART_RTS_PIN);                        /* Disable RTS */
#endif
    return UART_DATA_REG;
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready (non blocking)

The function asserts RTS low then waits for the receive complete bit to be set.
RTS is then cleared high. The function returns with RTS high if no character is
present, otherwise the character is retrieved and RTS is cleared.

returns: unsigned int. The upper byte is zero or NO_DATA if no character present.
*/

unsigned int getch(void)
{
#ifdef USE_HARDWARE_FLOW
    cbi(UART_RTS_PORT,UART_RTS_PIN);                        /* Enable RTS */
#endif
#ifndef USE_INTERRUPTS
    if (!(UART_STATUS_REG & _BV(RECEIVE_COMPLETE_BIT)))
        return (NO_DATA << 8);
#endif
#ifdef USE_HARDWARE_FLOW
    sbi(UART_RTS_PORT,UART_RTS_PIN);                        /* Disable RTS */
#endif
    return UART_DATA_REG;
}

