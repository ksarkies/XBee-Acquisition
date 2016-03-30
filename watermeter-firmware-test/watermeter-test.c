/**
@mainpage AVR Watermeter Node Firmware
@version 0.0
@author Ken Sarkies (www.jiggerjuice.net)
@date 18 July 2014

@brief Code for the watermeter board without XBee for count tests

This code forms the interface between an extermal serial terminal (PC), and a
counter signal providing count and battery voltage measurements.

The code is written to make use of sleep modes and other techniques to
minimize power consumption. The AVR is woken by a count signal, updates the
running total, and sleeps.

The watchdog timer is used to wake the AVR at intervals to transmit the count
and battery voltage to the serial interface.

@note
Fuses: Disable the "WDT Always On" fuse and disable the BOD fuse.
@note
Software: AVR-GCC 4.8.2
@note
Target:   AVR with sufficient output ports and a USART (USI not supported)
@note
Tested:   ATTiny841 at 1MHz internal clock.

*/
/****************************************************************************
 *   Copyright (C) 2014 by Ken Sarkies ksarkies@internode.on.net            *
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

#include <inttypes.h>
#include <string.h>
#include <stdbool.h>
#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "../libs/defines.h"
#include "../libs/serial.h"
#include <util/delay.h>
#include "watermeter-test.h"

#define TRUE 1
#define FALSE 0
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

/* Global variables */

uint32_t counter;               /* Event Counter */
uint8_t wdtCounter;             /* Data Transmission Timer */

/*---------------------------------------------------------------------------*/
/** @brief      Main Program

An interrupt service routine records a transition on the counter input. This
wakes the AVR which is sent back to sleep when the counts have settled.

The watchdog timer interrupt service routine provides a time tick of 1 second.
This wakes the AVR which Transmits any count and period measurement of the
photointerrupter signal.
*/

int main(void)
{

/*  Initialise hardware */
    hardwareInit();
    wdtInit(WDT_TIME);
/** Initialize the UART library, pass the baudrate and avr cpu clock 
(uses the macro UART_BAUD_SELECT()). Set the baudrate to a predefined value. */
    uartInit();

/* Initialise process counter */
    counter = 0;

/* Initialise watchdog timer count */
    wdtCounter = 0;

    sei();
/*---------------------------------------------------------------------------*/
/* Main loop forever. */
    for(;;)
    {
/* Power down the AVR to deep sleep until an interrupt occurs */
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sleep_cpu();

/* On waking, note the count, wait a bit, and check if it has advanced. If
not, return to sleep. Otherwise keep awake until the counts have settled.
This will avoid rapid wake/sleep cycles when counts are changing.
Counter is a global and is changed in the ISR. */
        uint32_t lastCount;
        do
        {
            lastCount = counter;

/* Any interrupt will wake the AVR. If it is a WDT timer overflow event,
1 second will be too short to do anything useful, so go back to sleep again
until enough such events have occurred. */
            if (wdtCounter > ACTION_COUNT)
            {
/* Now ready to initiate a data transmission. */
                wdtCounter = 0; /* Reset the WDT counter for next time */
/* Transmit the count value preceded by 7 blank csv fields. */
                sendch(',');
                sendch(',');
                sendch(',');
                sendch(',');
                sendch(',');
                sendch(',');
                sendDataCommand(',',lastCount);
                sendch('\n');
/* We can now subtract the transmitted count from the current counter value
and go back to sleep. */
                counter -= lastCount;
                lastCount = 0;
            }
            _delay_ms(1);
        }
        while (counter != lastCount);
    }
}

/****************************************************************************/
/** @brief Initialize the hardware for process measurement

Set unused ports to inputs and disable power to all unused peripherals.
Set the process counter interrupt to INT0.
*/
void hardwareInit(void)
{
/* Set PRR to disable all peripherals except USART.
Set input ports to pullups and disable digital input buffers on AIN inputs. */

#ifdef ADC_ONR
    cbi(ADC_ONR,AD_EN); /* Power off ADC */
#endif
    outb(PRR,0xFF);     /* power down all controllable peripherals */
    cbi(PRR,PRR_USART0);/* power up USART */
#ifdef PORTA
    outb(DDRA,0);       /* set as inputs */
    outb(PORTA,0x07);   /* set pullups   */
#endif
#ifdef PORTB
    outb(DDRB,0);       /* set as inputs */
    outb(PORTB,0xFF);   /* set pullups   */
#endif
#ifdef PORTC
    outb(DDRC,0);       /* set as inputs */
    outb(PORTC,0xFF);   /* set pullups   */
#endif
#ifdef PORTD
    outb(DDRD,0);       /* set as inputs */
    outb(PORTD,0x1F);   /* set pullups   */
#endif
#ifdef AC_SR0
    sbi(AC_SR0,AC_D0);  /* turn off Analogue Comparator 0 */
#endif
#ifdef AC_SR1
    sbi(AC_SR1,AC_D1);  /* turn off Analogue Comparator 1 */
#endif
#ifdef DID_R0
    outb(DI_DR0,3);     /* turn off digital input buffers */
#endif
#ifdef DID_R1
    outb(DI_DR1,3);
#endif

/* Set output ports to desired directions and initial settings */

#ifdef TEST_PIN
    sbi(TEST_PORT_DIR,TEST_PIN);
    cbi(TEST_PORT,TEST_PIN);            /* Test port */
#endif
#ifdef VBAT_PIN
    sbi(VBAT_PORT_DIR,VBAT_PIN);        /* Battery Measure Request */
    cbi(VBAT_PORT,VBAT_PIN);
#endif
#ifdef SLEEP_RQ_PIN
    sbi(SLEEP_RQ_PORT_DIR,SLEEP_RQ_PIN);/* XBee Sleep Request */
    sbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Set to keep XBee off */
#endif
#ifdef COUNT_PIN
    cbi(COUNT_PORT_DIR,COUNT_PIN);      /* XBee counter input pin */
    sbi(COUNT_PORT,COUNT_PIN);          /* Set pullup */
#endif
#ifdef ON_SLEEP_PIN
    cbi(ON_SLEEP_PORT_DIR,ON_SLEEP_PIN);/* XBee On/Sleep Status input pin */
#endif

/* Counter: Use PCINT for the asynchronous pin change interrupt on the
count signal line. */
    sbi(PC_MSK,PC_INT);
    sbi(IMSK,PC_IE);

}

/****************************************************************************/
/** @brief Initialize the watchdog timer to interrupt on maximum delay

The watchdog timer is set to interrupt rather than reset so that it can wakeup
the AVR. Keeping WDTON disabled (default) will allow the settings to be
changed without restriction. The watchdog timer is initially disabled after
reset.

The timeout settings give the same time interval for each AVR, regardless of the
clock frequency used by the WDT.

The interrupt enable mode is set. The WDE bit is left off to ensure that the WDT
remains in interrupt mode.

IMPORTANT: Disable the "WDT Always On" fuse.

@param[in] uint8_t waketime: a register setting, 9 or less (see datasheet).
*/
void wdtInit(const uint8_t waketime)
{
    uint8_t timeout = waketime;
    outb(WDT_CSR,0);     /* Clear the WDT register */
    if (timeout > 9) timeout = 9;
    uint8_t wdtcsrSetting = (timeout & 0x07);
    if (timeout > 7) wdtcsrSetting |= _BV(WDP3);
    outb(WDT_CSR,wdtcsrSetting | _BV(WDIE));
}

/*--------------------------------------------------------------------------*/
/** @brief Convert a 32 bit value to ASCII hex form and send with a command.

@param[in] int8_t command: ASCII command character to prepend to message.
@param[in] int32_t datum: integer value to be sent.
*/

void sendDataCommand(const uint8_t command, const uint32_t datum)
{
    uint8_t buffer[12];
    uint8_t i;
    uint32_t value = datum;
    for (i = 0; i < 10; i++)
    {
        buffer[10-i] = "0123456789ABCDEF"[value & 0x0F];
        value >>= 4;
    }
    buffer[11] = 0;             /* String terminator */
    buffer[0] = command;
    for (uint8_t i=0; i < 11; i++)
    {
        sendch(buffer[i]);
    }
}

/****************************************************************************/
/** @brief Interrupt on Count Signal.

Determine if a change in the count signal level has occurred. An upward
change will require a count to be registered. A downward change is ignored.

Sample twice to ensure that this isn't a false alarm.

The count is suppressed if the muteCounter is non zero. This is intended to
follow a transmission. The specific phenomenon dealt with is the presence
of a short positive pulse at the time of a transmission, when the counter
input is at low level.
*/
ISR(PCINT1_vect)
{
    uint8_t countSignal = (inb(COUNT_PORT) & _BV(COUNT_PIN));
    if (countSignal > 0)
    {
        _delay_us(100);
        countSignal = (inb(COUNT_PORT) & _BV(COUNT_PIN));
        if (countSignal > 0) counter++;
    }
}

/****************************************************************************/
/** @brief Interrupt on Watchdog Timer.

Increment the counter to signal state of WDT.
*/
#if (MCU_TYPE==4313)
ISR(WDT_OVERFLOW_vect)
#else
ISR(WDT_vect)
#endif
{
    wdtCounter++;
}

