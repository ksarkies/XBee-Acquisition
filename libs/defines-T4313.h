/* AVR/XBee Bootloader Defines

For AVR microcontrollers without bootloader section.

This file assigns registers, particular to an AVR type, to common constants.

I/O pin values for controlling the bootloader operation are given at the end.

Software: AVR-GCC 4.8.2
Tested:   ATTiny4313 at 1MHz internal clock.
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

#include	<avr/io.h>
#include    "project.h"

/* Choose whether to use hardware flow control for serial comms. */
//#define USE_HARDWARE_FLOW

/* These are the defines for the selected device and bootloader system */
#define F_CPU               8000000
#define BAUD                38400

/* 0.128ms clock from 8MHz clock
Timer clock scale value 5 gives scale of 1024, (see timer.c)
This gives a 32ms overflow interrupt.*/
#define RTC_SCALE               5

/* Simple serial I/O. CPU frequency and baudrate must be defined BEFORE this
include as setbaud.h sets a default value. */
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
#define PC_IER                  GIMSK
#define PC_MSK                  PCMSK1
#define PC_INT                  1
#define PC_IE                   PCIE1
#define COUNT_ISR               PCINT1_vect

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

/* Count Signal pin */
#define COUNT_PORT_DIR          DDRD
#define COUNT_PORT              PIND
#define COUNT_PIN               2

/* define pin for remaining in bootloader */
#define PROG_PORT_DIR           DDRB
#define PROG_PORT               PINB
#define PROG_PIN                4

/* Battery Measurement Control */
#define VBAT_PORT_DIR           DDRD
#define VBAT_PORT               PORTD
#define VBAT_PIN                5

/* Sleep request Control */
#define SLEEP_RQ_PORT_DIR       DDRB
#define SLEEP_RQ_PORT           PORTB
#define SLEEP_RQ_PIN            3

/* Output to force XBee reset */
/* XBee Reset */
#define XBEE_RESET_PORT_DIR     DDRB
#define XBEE_RESET_PORT         PORTB
#define XBEE_RESET_PIN          2

/* On/Sleep Status */
#define ON_SLEEP_PORT_DIR       DDRB
#define ON_SLEEP_PORT           PINB
#define ON_SLEEP_PIN            1

/* Test pin */
#define TEST_PORT_DIR           DDRB
#define TEST_PORT               PORTB
#define TEST_PIN                0

