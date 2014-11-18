/* AVR/XBee Bootloader Defines

For AVR microcontrollers without bootloader section.

This file assigns registers, particular to an AVR type, to common constants.

I/O pin values for controlling the bootloader operation are given at the end.

Software: AVR-GCC 4.8.2
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

#include	<avr/io.h>

/* Choose whether to use hardware flow control for serial comms. */
//#define USE_HARDWARE_FLOW

/* These are the defines for the selected device and bootloader system */
#define F_CPU               1000000
#define BAUD                9600

/* These defines control how the bootloader interacts with hardware */
/* Use the defined input pin to decide if the application will be entered
automatically */
#define AUTO_ENTER_APP      1

/* Use the defined output pin to force the XBee to stay awake while in the
bootloader. This is valid for the XBee sleep mode 1 only. The application should
move it to other modes if necessary. Note that using this may fail because the
output pins may be forced to an undesired level during programming. */
#define XBEE_STAY_AWAKE     1

/* Simple serial I/O (must define cpu frequency and baudrate before this
include) */
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

/* UART Error Definitions. */
#define NO_DATA                 0x01
#define BUFFER_OVERFLOW         0x02
#define OVERRUN_ERROR           0x04
#define FRAME_ERROR             0x08
#define STATE_MACHINE           0x10
#define CHECKSUM                0x11

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
#define DOUBLE_RATE             U2X

/* UART Flow control ports */
#define UART_CTS_PORT           PINA
#define UART_CTS_PORT_DIR       DDRA
#define UART_CTS_PIN            1
#define UART_RTS_PORT           PORTA
#define UART_RTS_PORT_DIR       DDRA
#define UART_RTS_PIN            0

/* definitions for peripheral power control */
#define PRR_USART0              PRUSART

/* definitions for digital input control */
#define DI_DR0                  DIDR0

/* definitions for interrupt control */
#define IMSK                    GIMSK
#define INT_CR                  MCUCR
#define WDT_CSR                 WDTCR

/* definitions for analogue comparator control */
#define AC_SR0                  ACSR
#define AC_D0                   ACD

/* definitions for SPM control */
#define	SPMCR_REG	            SPMCSR
/* Pagesize and addresses are in bytes (note the datasheets use word values).
These are defined from avr-libc io.h based on processor choice. */
#define MEMORY_SIZE             FLASHEND
#define	APP_START	            0x0000
#define	APP_END	                BASEADDR
#define	PAGESIZE	            SPM_PAGESIZE
#define PAGES                   (APP_END / PAGESIZE)
#define PAGE_FLAGS              (PAGES >> 3)

/* define pin for remaining in bootloader */
#define PROG_PORT_DIR           DDRB
#define PROG_PORT               PINB
#define PROG_PIN                2

/* define pin for forcing the XBee to stay awake (sleep_rq) */
#define WAKE_PORT_DIR           DDRB
#define WAKE_PORT               PORTB
#define WAKE_PIN                3

/* Battery Measurement Control */
#define VBAT_PORT_DIR           DDRD
#define VBAT_PORT               PORTD
#define VBAT_PIN                5

/* Sleep request Control */
#define SLEEP_RQ_PORT_DIR       DDRB
#define SLEEP_RQ_PORT           PORTB
#define SLEEP_RQ_PIN            3

/* Test pin */
#define TEST_PORT_DIR           DDRB
#define TEST_PORT               PORTB
#define TEST_PIN                1

