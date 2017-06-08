/* AVR/XBee Defines for the ATTiny441-ATTiny841 series

This uses the watermeter board described in Kicad-main.

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

#if ((MCU_TYPE==841) && (defined USE_HARDWARE_FLOW))
#warning "Cannot use hardware flow control for this particular project \
due to lack of outputs."
#undef USE_HARDWARE_FLOW
#endif

/* These are the defines for the selected device and bootloader system */
#define F_CPU               8000000
#define BAUD                9600

/* 0.128ms clock from 8MHz clock
Timer clock scale value 5 gives scale of 1024, (see timer.c)
This gives a 32ms overflow interrupt.*/
#define RTC_SCALE               5

/* Simple serial I/O. CPU frequency and baudrate must be defined BEFORE this
include as setbaud.h sets a default value. */
#include <util/setbaud.h>

/* definitions for UART control */
#define	BAUD_RATE_HIGH_REG	    UBRR0H
#define	BAUD_RATE_LOW_REG	    UBRR0L
#define	UART_CONTROL_REG	    UCSR0B
#define	UART_FORMAT_REG	        UCSR0C
#define	UART_STATUS_REG	        UCSR0A
#define	UART_DATA_REG	        UDR0
#define DOUBLE_RATE             U2X0
#define FRAME_SIZE              UCSZ00
#define	ENABLE_TRANSMITTER_BIT	TXEN0
#define	ENABLE_RECEIVER_BIT	    RXEN0
#define	TRANSMIT_COMPLETE_BIT	TXC0
#define	RECEIVE_COMPLETE_BIT	RXC0
#define TRANSMIT_COMPLETE_IE    TXCIE0
#define RECEIVE_COMPLETE_IE     RXCIE0
#define DATA_REGISTER_EMPTY_IE  UDRIE0

/* ISR definitions */
#define UART1_TRANSMIT_ISR      USART0_UDRE_vect
#define UART1_RECEIVE_ISR       USART0_RX_vect

/* definitions for peripheral power control */
#define PRR_USART0              PRUSART0

/* definitions for digital input control */
#define DI_DR0                  DIDR0
#define DI_DR1                  DIDR1

/* definitions for interrupt control.
Pin Change Interrupts needed to wake from sleep.
PCINT1 is for PCINT11-8, PCINT0 is for PCINT7-0 */
#define IMSK                    GIMSK
#define INT_CR                  MCUCR
#define WDT_CSR                 WDTCSR
#define PC_IER                  GIMSK
#define PC_MSK                  PCMSK1
#define PC_INT                  1               /* Mask bit for PCINT9 */
#define PC_IE                   PCIE1
#define COUNT_ISR               PCINT1_vect

/* definitions for analogue comparator control */
#define AC_SR0                  ACSR0A
#define AC_SR1                  ACSR1A
#define AC_D0                   ACD0
#define AC_D1                   ACD1

/* definitions for ADC control */
#define ADC_ONR                 ADCSRA
#define AD_EN                   ADEN

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
#define COUNT_PORT_DIR          DDRB
#define COUNT_PORT              PINB
#define COUNT_PORT_PUP          PORTB
#define COUNT_PIN               1

/* Pin for remaining in bootloader */
#define PROG_PORT_DIR           DDRB
#define PROG_PORT               PINB
#define PROG_PORT_PUP           PORTB
#define PROG_PIN                0

/* Battery Measurement Control */
#define VBATCON_PORT_DIR        DDRA
#define VBATCON_PORT            PORTA
#define VBATCON_PIN             7

/* Input to indicate XBee is awake (on) */
/* On/Sleep */
#define ON_SLEEP_PORT_DIR       DDRA
#define ON_SLEEP_PORT           PINA
#define ON_SLEEP_PORT_PUP       PORTA
#define ON_SLEEP_PIN            0

/* Pin for forcing the XBee to stay awake */
/* Sleep request Control */
#define SLEEP_RQ_PORT_DIR       DDRA
#define SLEEP_RQ_PORT           PORTA
#define SLEEP_RQ_PIN            3

/* Output to force XBee reset */
/* XBee Reset */
#define XBEE_RESET_PORT_DIR     DDRB
#define XBEE_RESET_PORT         PORTB
#define XBEE_RESET_PIN          2

/* Unused I/O ports, for setting  pullups */
/* All used in this device */

/* Disable Digital Input Buffers on comparators and A/D converters. */
/* None used in this device */

