/* AVR/XBee Defines for the ATMega48 series

This uses the test board described in gEDA-XBee-Test.

For AVR microcontrollers with a bootloader section.
The bootloader start address BASEADDR must be given in the makefile.

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
#define F_CPU                   8000000
#define BAUD                    38400

/* 0.128ms clock from 8MHz clock
Timer clock scale value 5 gives scale of 1024, (see timer.c)
This gives a 32ms overflow interrupt.*/
#define RTC_SCALE               5

/* These defines control how the bootloader interacts with hardware */
/* Use the defined input pin to decide if the application will be entered
automatically */
#define AUTO_ENTER_APP          1

/* Use the defined output pin to force the XBee to stay awake while in the
bootloader. This is valid for the XBee sleep mode 1 only. The application
should move it to other modes if necessary. Note that using this may fail
because the output pins may be forced to an undesired level during programming. */
#define XBEE_STAY_AWAKE         1

// Simple serial I/O (must define cpu frequency and baudrate before this include) */
#include <util/setbaud.h>

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

/* definitions for peripheral power control */
#define PRR_USART0              PRUSART0

/* definitions for digital input control */
#define DI_DR0                  DIDR0
#define DI_DR1                  DIDR1

/* definitions for interrupt control. PCMSK2 bit 5 sets PCINT21 on PD5. */
#define IMSK                    EIMSK
#define INT_CR                  EICRA
#define WDT_CSR                 WDTCSR
#define PC_IER                  PCICR
#define PC_MSK                  PCMSK2
#define PC_INT                  5
#define PC_IE                   PCIE2
#define COUNT_ISR               PCINT2_vect

/* definitions for analogue comparator control */
#define AC_SR0                  ACSR
#define AC_D0                   ACD

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
#define PAGES                   (BASEADDR / PAGESIZE)
#define PAGE_FLAGS              (PAGES >> 3)

/* Count Signal pin */
#define COUNT_PORT_DIR          DDRD
#define COUNT_PORT              PIND
#define COUNT_PIN               5

/* define pin for remaining in bootloader */
#define PROG_PORT_DIR           DDRB
#define PROG_PORT               PINB
#define PROG_PIN                2

/* Battery Measurement Control */
#define VBATCON_PORT_DIR        DDRC
#define VBATCON_PORT            PORTC
#define VBATCON_PIN             5

/* Battery Measurement Input ADC1 */
#define VBAT_PORT_DIR           DDRC
#define VBAT_PORT               PORTC
#define VBAT_PIN                1

/* Analogue Measurement Input ADC0 */
#define V_PORT_DIR              DDRC
#define V_PORT                  PORTC
#define V_PIN                   0

/* Input to indicate XBee is awake (on) */
/* On/Sleep */
#define ON_SLEEP_PORT_DIR       DDRB
#define ON_SLEEP_PORT           PORTB
#define ON_SLEEP_PIN            4

/* Pin for forcing the XBee to stay awake */
/* Sleep request Control */
#define SLEEP_RQ_PORT_DIR       DDRB
#define SLEEP_RQ_PORT           PORTB
#define SLEEP_RQ_PIN            3

/* Output to force XBee reset */
/* XBee Reset */
#define XBEE_RESET_PORT_DIR     DDRB
#define XBEE_RESET_PORT         PORTB
#define XBEE_RESET_PIN          5

/* Test pin PC4 */
#define TEST_PORT_DIR           DDRC
#define TEST_PORT               PORTC
#define TEST_PIN                4

