/* AVR/XBee Defines

For AVR microcontrollers with a bootloader section.

This file assigns registers, particular to an AVR type, to common constants.
It is valid for bootloader and application firmware.

I/O pin values for controlling the bootloader operation are given at the end.

Software: AVR-GCC 4.8.2
Target:   Any AVR with sufficient output ports and a timer
Tested:   ATMega168 at 8MHz internal clock.
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

#include	<avr/io.h>

/* Choose whether to use hardware flow control for serial comms.
Needed for the bootloader as the upload is extensive. */
#define USE_HARDWARE_FLOW

/* These are the defines for the selected device and bootloader system */
#define F_CPU               8000000
#define BAUD                38400
#define BOOTLOADER_SIZE     2048

/* These defines control how the bootloader interacts with hardware */
/* Use the defined input pin to decide if the application will be entered automatically */
#define AUTO_ENTER_APP      1
/* Use the defined output pin to force the XBee to stay awake while in the bootloader.
This is valid for the XBee sleep mode 1 only. The application should move it to other modes
if necessary. Note that using this may fail because the output pins may be forced
to an undesired level during programming. */
#define XBEE_STAY_AWAKE     1

// Simple serial I/O (must define cpu frequency and baudrate before this include) */
#include <util/setbaud.h>

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

/* definitions for UART control */
#define	BAUD_RATE_HIGH_REG	    UBRR0H
#define	BAUD_RATE_LOW_REG	    UBRR0L
#define	UART_CONTROL_REG	    UCSR0B
#define	UART_FORMAT_REG	        UCSR0C
#define FRAME_SIZE              UCSZ00
#define	ENABLE_TRANSMITTER_BIT	TXEN0
#define	ENABLE_RECEIVER_BIT	    RXEN0
#define	UART_STATUS_REG	        UCSR0A
#define	TRANSMIT_COMPLETE_BIT	TXC0
#define	RECEIVE_COMPLETE_BIT	RXC0
#define	UART_DATA_REG	        UDR0
#define DOUBLE_RATE             U2X0

/* UART Flow control ports */
#define UART_CTS_PORT           PIND
#define UART_CTS_PORT_DIR       DDRD
#define UART_CTS_PIN            2
#define UART_RTS_PORT           PORTD
#define UART_RTS_PORT_DIR       DDRD
#define UART_RTS_PIN            3

/* definitions for SPM control */
#define	SPMCR_REG	            SPMCSR
/* Pagesize and addresses are in bytes (note the datasheets use word values).
These are defined from avr-libc io.h based on processor choice. */
#define MEMORY_SIZE             FLASHEND
#define	APP_START	            0x0000
#define	APP_END	                (MEMORY_SIZE - BOOTLOADER_SIZE)
#define	PAGESIZE	            SPM_PAGESIZE
#define PAGES                   (MEMORY_SIZE / PAGESIZE)
#define PAGE_FLAGS              (PAGES >> 3)

/* define pin for remaining in bootloader */
#define PROG_PORT_DIR           DDRB
#define PROG_PORT               PINB
#define PROG_PIN                2

/* define pin for forcing the XBee to stay awake (sleep_rq) */
#define WAKE_PORT_DIR           DDRB
#define WAKE_PORT               PORTB
#define WAKE_PIN                3
