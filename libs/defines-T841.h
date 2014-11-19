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
#define	BAUD_RATE_HIGH_REG	    UBRR0H
#define	BAUD_RATE_LOW_REG	    UBRR0L
#define	UART_CONTROL_REG	    UCSR0B
#define	UART_FORMAT_REG	        UCSR0C
#define	UART_STATUS_REG	        UCSR0A
#define FRAME_SIZE              UCSZ00
#define	ENABLE_TRANSMITTER_BIT	TXEN0
#define	ENABLE_RECEIVER_BIT	    RXEN0
#define	TRANSMIT_COMPLETE_BIT	TXC0
#define	RECEIVE_COMPLETE_BIT	RXC0
#define	UART_DATA_REG	        UDR0
#define DOUBLE_RATE             U2X0

/* definitions for peripheral power control */
#define PRR_USART0              PRUSART0

/* definitions for digital input control */
#define DI_DR0                  DIDR0
#define DI_DR1                  DIDR1

/* definitions for interrupt control */
#define IMSK                    GIMSK
#define INT_CR                  MCUCR
#define WDT_CSR                 WDTCSR
#define PC_MSK                  PCMSK1
#define PC_INT                  1
#define PC_IE                   PCIE1

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
#define COUNT_PIN               1

/* Pin for remaining in bootloader */
#define PROG_PORT_DIR           DDRB
#define PROG_PORT               PINB
#define PROG_PIN                0

/* Battery Measurement Control */
#define VBAT_PORT_DIR           DDRA
#define VBAT_PORT               PORTA
#define VBAT_PIN                7

/* Sleep request Control */
#define SLEEP_RQ_PORT_DIR       DDRA
#define SLEEP_RQ_PORT           PORTA
#define SLEEP_RQ_PIN            3

/* On/Sleep Status */
#define ON_SLEEP_PORT_DIR       DDRA
#define ON_SLEEP_PORT           PINA
#define ON_SLEEP_PIN            0


