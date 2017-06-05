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
#if defined USE_RECEIVE_BUFFER || defined USE_TRANSMIT_BUFFER
#include "buffer.h"
#endif

#ifdef USE_TRANSMIT_BUFFER
static uint8_t transmitBuffer[TRANSMIT_BUFFER_SIZE];
#endif
#ifdef USE_RECEIVE_BUFFER
static uint8_t receiveBuffer[RECEIVE_BUFFER_SIZE];
#endif

/* Local Prototypes */
static void sendchDirect(uint8_t c);
static uint8_t getchDirect(void);

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
/* Enable USART transmitter data register empty interrupt */
    UART_CONTROL_REG |= _BV(DATA_REGISTER_EMPTY_IE);
#endif
#ifdef USE_RECEIVE_BUFFER
    buffer_init(receiveBuffer,RECEIVE_BUFFER_SIZE);
#endif
#ifdef USE_TRANSMIT_BUFFER
    buffer_init(transmitBuffer,TRANSMIT_BUFFER_SIZE);
#endif
}

/*-----------------------------------------------------------------------------*/
/* Send a character when the Tx is ready

The function waits until CTS is asserted low then waits until the UART indicates
that the character has been sent.
*/

void sendch(uint8_t c)
{
#ifdef USE_TRANSMIT_BUFFER
    buffer_put(transmitBuffer,c);
#ifdef USE_TRANSMIT_INTERRUPTS
/* Enable transmit interrupt to trigger a transmission. */
    UART_CONTROL_REG |= _BV(DATA_REGISTER_EMPTY_IE);
#endif
#else
    sendchDirect(c);
#endif
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready (blocking)

The function asserts RTS low then waits for the receive complete bit to be set.
RTS is then cleared high. The character is then retrieved. If no character is
present, the function blocks.

returns: uint8_t. The received character.
*/

uint8_t getchb(void)
{
    unsigned int ch;
    do
        ch = getch();
    while (ch == 0x0100);
    return (uint8_t) (ch & 0xFF);
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready (non blocking)

The function asserts RTS low then waits for the receive complete bit to be set.
RTS is then cleared high. The function returns with RTS high if no character is
present, otherwise the character is retrieved and RTS is cleared.

returns: unsigned int. The upper byte is zero or NO_DATA if no character present.
*/

uint16_t getch(void)
{
#ifdef USE_RECEIVE_BUFFER
    uint16_t ch = buffer_get(receiveBuffer);
    if (ch == 0x0100) ch = NO_DATA;
#else
    uint16_t ch;
    ch = getchDirect();
#endif
    return ch;
}

/*-----------------------------------------------------------------------------*/
/* Send a character directly

The function waits until CTS is asserted low then waits until the UART indicates
that the character has been sent.
*/

void sendchDirect(uint8_t c)
{
#ifdef USE_HARDWARE_FLOW
    while (inb(UART_CTS_PORT) & _BV(UART_CTS_PIN));     /* wait for clear to send */
#endif
    UART_DATA_REG = c;                                  /* send */
    while (!(UART_STATUS_REG & _BV(TRANSMIT_COMPLETE_BIT)));    /* wait till gone */
    sbi(UART_STATUS_REG,TRANSMIT_COMPLETE_BIT);         /* force reset TXCflag */
}

/*-----------------------------------------------------------------------------*/
/* Get a character directly

The function asserts RTS low then waits for the receive complete bit to be set.
RTS is then cleared high. The character is then retrieved. If no character is
present, the function blocks.

returns: uint8_t. The received character.
*/

uint8_t getchDirect(void)
{
#ifdef USE_HARDWARE_FLOW
    cbi(UART_RTS_PORT,UART_RTS_PIN);                        /* Enable RTS */
#endif
#ifndef USE_TRANSMIT_INTERRUPTS
    while (!(UART_STATUS_REG & _BV(RECEIVE_COMPLETE_BIT)));
#endif
#ifdef USE_HARDWARE_FLOW
    sbi(UART_RTS_PORT,UART_RTS_PIN);                        /* Disable RTS */
#endif
    return UART_DATA_REG;
}

#ifdef USE_RECEIVE_INTERRUPTS
/*-----------------------------------------------------------------------------*/
/* Serial Receiver ISR

Pulls in the character from the serial receive interface and places it in the
buffer. No error checking is done so buffer full conditions result in lost
characters.
*/

ISR(UART1_RECEIVE_ISR)
{
    buffer_put(receiveBuffer,getchDirect());  
}
#endif

#ifdef USE_TRANSMIT_INTERRUPTS
/*-----------------------------------------------------------------------------*/
/* Serial Transmitter ISR

Retrieves a character from the transmit buffer and sends it to the serial
transmit interface. If no character is present in the buffer this simply
returns. The transmitter empty interrupt must be enabled when a character is
placed in the transmit buffer.
*/

ISR(UART1_TRANSMIT_ISR)
{
    uint16_t c = buffer_get(transmitBuffer);

    if (c != 0x0100)
    {
        sendchDirect((char) (c & 0xFF));
    }
    else
    {
/* tx buffer empty, disable UDRE interrupt */
        UART_CONTROL_REG &= ~_BV(DATA_REGISTER_EMPTY_IE);
    }
}
#endif

