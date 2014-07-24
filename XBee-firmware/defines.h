/* AVR/XBee Defines

This file assigns registers, particular to an AVR type, to common constants.

I/O pin values for controlling the boorloader operation are given at the end.

Software: AVR-GCC 4.8.2
Target:   Any AVR with sufficient output ports and a timer
Tested:   ATTiny4313 at 1MHz internal clock.
*/

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

/* baud rate register value calculation */
#ifndef F_CPU
#define F_CPU                   1000000
#endif
#define BAUD                    9600

/* Simple serial I/O (must define cpu frequency and baudrate before this include) */
#include <util/setbaud.h>

/* definitions for UART control */
#define	BAUD_RATE_HIGH_REG	    UBRRH
#define	BAUD_RATE_LOW_REG	    UBRRL
#define	UART_CONTROL_REG	    UCSRB
#define	UART_FORMAT_REG	        UCSRC
#define FRAME_SIZE              UCSZ0
#define	ENABLE_TRANSMITTER_BIT	TXEN
#define	ENABLE_RECEIVER_BIT	    RXEN
#define	UART_STATUS_REG	        UCSRA
#define	TRANSMIT_COMPLETE_BIT	TXC
#define	RECEIVE_COMPLETE_BIT	RXC
#define	UART_DATA_REG	        UDR

/* UART Flow control ports */
#define UART_CTS_PORT           PINA
#define UART_CTS_PORT_DIR       DDRA
#define UART_CTS_PIN            1
#define UART_RTS_PORT           PORTA
#define UART_RTS_PORT_DIR       DDRA
#define UART_RTS_PIN            0

/* Battery Measurement Control */
#define VBAT_PORT_DIR           DDRD
#define VBAT_PORT               PORTD
#define VBAT_PIN                5

/* Test pin */
#define TEST_PORT_DIR           DDRD
#define TEST_PORT               PORTD
#define TEST_PIN                3

