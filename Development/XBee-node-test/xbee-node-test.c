/**
@mainpage XBee AVR Node Test
@version 0.0.0
@author Ken Sarkies (www.jiggerjuice.info)
@date 25 September 2014
@brief Code for an ATTiny841/ATMega48 AVR with an XBee in Remote Low Power Node

A message containing counts from a digital input is transmitted every few
seconds. Battery voltage is also transmitted from a voltage divider connected
to the XBee and read from ADC1. Options for XBee sleep and battery measurement
are provided at compile time.

The XBee is tested for association with the coordinator. The program does not
progress until association has occurred.

This code forms the core of an interface between an XBee networking device
using ZigBee stack, and a data acquisition unit making a variety of
measurements for communication to a base controller.

This application works with AVRs having a bootloader block or for situations
where a bootloader is not required. It has no code referring to a bootloader.

The board targetted is the test board developed for the project using the
ATMega48 series microcontrollers. See the hardwareInit() function for
documented I/O ports.

The ports and clock scale factor are defined in the libs/defines headers.

Count interrupt via PCINT2 is handled.

@note
CTS must be set in the XBee and USE_HARDWARE_FLOW also enabled.
@note
Software: AVR-GCC 4.8.2
@note
Target:   Any AVR with sufficient output ports and a timer
@note
Tested:   ATMega48 series, ATTiny841 at 8MHz internal clock.
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

#include <string.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <avr/sleep.h>
#include "../../libs/defines.h"
#include "../../libs/buffer.h"
#include "../../libs/serial.h"
#include "../../libs/timer.h"
#include "../../libs/xbee.h"
#include <util/delay.h>
#include "xbee-node-test.h"

/** Convenience macros (we don't use them all) */

#define  _BV(bit) (1 << (bit))
#define inb(sfr) _SFR_BYTE(sfr)
#define inw(sfr) _SFR_WORD(sfr)
#define outb(sfr, val) (_SFR_BYTE(sfr) = (val))
#define outw(sfr, val) (_SFR_WORD(sfr) = (val))
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define inbit(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))
#define toggle(sfr, bit) (_SFR_BYTE(sfr) ^= _BV(bit))
#define high(x) ((uint8_t) (x >> 8) & 0xFF)
#define low(x) ((uint8_t) (x & 0xFF))

/*****************************************************************************/
/* Global Variables */
/** Real Time Clock Structure

The 32 bit time can also be accessed as four bytes. Time scale is defined in
the information block.
*/

#define BUFFER_SIZE 60

/* timeCount measures off timer interrupt ticks to provide an extended time
between transmissions */
static uint8_t timeCount;
/* Counter keep track of external transitions on the digital input */
static uint32_t counter;

static uint32_t timeValue;
static uint8_t coordinatorAddress64[8];
static uint8_t coordinatorAddress16[2];

static bool transmitMessage;

/****************************************************************************/
/* Local Prototypes */

void hardwareInit(void);
void wdtInit(const uint8_t waketime);
void sendDataCommand(const uint8_t command, const uint32_t datum);
void sendMessage(const char* data);
void sleepXBee(void);
void wakeXBee(void);

/*****************************************************************************/
/** @brief Main Program */

int main(void)
{
    timeCount = 0;
    wdt_disable();                  /* Stop watchdog timer */
    hardwareInit();                 /* Initialize the processor specific hardware */
    uartInit();
    initBuffers();                  /* Set up communications buffers if used */
    timer0Init(0,RTC_SCALE);        /* Configure the timer */
    timeValue = 0;                  /* Reset timer */
/* Initialise process counter */
    counter = 0;

/* Set the coordinator addresses. All zero 64 bit address with "unknown" 16 bit
address avoids knowing the actual address, but may cause an address discovery
event. */
    for  (uint8_t i=0; i < 8; i++) coordinatorAddress64[i] = 0x00;
    coordinatorAddress16[0] = 0xFE;
    coordinatorAddress16[1] = 0xFF;

/* Check for association indication from the XBee.
Don't start until it is associated. */
    while (! checkAssociated());

/*---------------------------------------------------------------------------*/
/* Main loop */

/* Start off with XBee asleep and wait for interrupt to wake it up */
#ifdef SLEEP_XBEE
    sleepXBee();
#endif

    transmitMessage = false;
    for(;;)
    {
        sei();                          /* Enable global interrupts */
        wdt_reset();

/* Check for the timer interrupt to indicate it is time for a message to go out.
Wakeup the XBee and send putting it back to sleep afterwards. */
        if (transmitMessage)
        {
            transmitMessage = false;
#ifdef TEST_PORT
            sbi(TEST_PORT,TEST_PIN);
#endif
#ifdef SLEEP_XBEE
            wakeXBee();
#endif

/* First check for association indication from the XBee.
Don't proceed until it is associated but go back to sleep as the coordinator
may be unreachable. */
            if (checkAssociated())
            {
                uint32_t batteryVoltage = 0;
/* Read the battery voltage from the XBee. */
#ifdef BATTERY_MEASURE
                uint8_t data[12];
                int8_t dataLength = readXBeeIO(data);
                if (dataLength > 0) batteryVoltage = getXBeeADC(data,1);
#endif
                uint8_t txCommand = 'D';
                sendDataCommand(txCommand,counter+(batteryVoltage << 16));
                counter = 0;                /* Reset counter value */
            }
#ifdef SLEEP_XBEE
            sleepXBee();
#endif
#ifdef TEST_PORT
            _delay_ms(200);
            cbi(TEST_PORT,TEST_PIN);
#endif
        }
    }
}

/****************************************************************************/
/** @brief Convert a 32 bit value to ASCII hex form and send with a command.

Also compute an 8-bit modular sum checksum from the data and convert to hex
for transmission. This is sent at the beginning of the string.

@param[in] int8_t command: ASCII command character to prepend to message.
@param[in] int32_t datum: integer value to be sent.
*/

void sendDataCommand(const uint8_t command, const uint32_t datum)
{
    char buffer[12];
    uint8_t i;
    char checksum = -(datum + (datum >> 8) + (datum >> 16) + (datum >> 24));
    uint32_t value = datum;
    for (i = 0; i < 10; i++)
    {
        if (i == 8) value = checksum;
        buffer[10-i] = "0123456789ABCDEF"[value & 0x0F];
        value >>= 4;
    }
    buffer[11] = 0;             /* String terminator */
    buffer[0] = command;
    sendMessage(buffer);
}

/****************************************************************************/
/** @brief Send a string message

Send a string message.

@param[in]  uint8_t* data: pointer to a string of data (ending in 0).
*/
void sendMessage(const char* data)
{
    sendTxRequestFrame(coordinatorAddress64, coordinatorAddress16,0,
                       strlen(data),(uint8_t*)data);
}

/****************************************************************************/
/** @brief Initialize the hardware for process measurement and XBee control

*/
void hardwareInit(void)
{
/* XBee Sleep Request ouput pin */
#ifdef SLEEP_RQ_PIN
    sbi(SLEEP_RQ_PORT_DIR,SLEEP_RQ_PIN);        /* XBee Sleep Request */
    cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);            /* Set to keep XBee on */
#endif
/* XBee On/Sleep Status input pin */
#ifdef ON_SLEEP_PIN
    cbi(ON_SLEEP_PORT_DIR,ON_SLEEP_PIN);
#endif
/* XBee reset output. Pulse low to reset. */
#ifdef XBEE_RESET_PIN
    sbi(XBEE_RESET_PORT_DIR,XBEE_RESET_PIN);    /* XBee Reset output pin */
    sbi(XBEE_RESET_PORT,XBEE_RESET_PIN);        /* Set to keep XBee on */
#endif
/* Battery Measurement Enable output pin */
#ifdef VBATCON_PIN
    sbi(VBATCON_PORT_DIR,VBATCON_PIN);          /* Battery Measure Enable */
    sbi(VBATCON_PORT,VBATCON_PIN);              /* Turn on */
#endif
/* Battery Measurement Input */
#ifdef VBAT_PIN
    cbi(VBAT_PORT_DIR,VBAT_PIN);
    cbi(VBAT_PORT,VBAT_PIN);
#endif
/* Counter input */
#ifdef COUNT_PIN
    cbi(COUNT_PORT_DIR,COUNT_PIN);
    sbi(COUNT_PORT,COUNT_PIN);                  /* Set to pullup */
#endif
/* General output for a LED to be activated by the microcontroller as desired. */
#ifdef TEST_PIN
    sbi(TEST_PORT_DIR,TEST_PIN);
    sbi(TEST_PORT,TEST_PIN);                    /* Turn on to start */
#endif

/* Counter: Use PCINT for the asynchronous pin change interrupt on the
count signal line. */
    sbi(PC_MSK,PC_INT);                         /* Mask */
    sbi(PC_IER,PC_IE);                          /* Enable */
}

/****************************************************************************/
/** @brief Sleep the XBee

This sets the Sleep_RQ pin high. If the XBee is in pin hibernate mode, it will
hold it asleep indefinitely until the Sleep_RQ pin is set low again. If the XBee
is in cyclic/pin wake mode, this will have no effect.

The XBee may take some time before sleeping, however no operations are dependent
on this time.
*/
inline void sleepXBee(void)
{
    sbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Request XBee Sleep */
}

/****************************************************************************/
/** @brief Wake the XBee

If the XBee is asleep, toggle the Sleep_RQ pin high then low. If the XBee is in
pin hibernate mode, this will hold it awake until the Sleep_RQ pin is set high
again or until the XBee wake period has expired. This is the mode the XBee
should be using in this application. Set the XBee wake period sufficiently long
if better control of wake time is desired.

The XBee should wake in a short time, about 100ms maximum.
*/
inline void wakeXBee(void)
{
    cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Request or set XBee Wake */
    while (inbit(ON_SLEEP_PORT,ON_SLEEP_PIN) == 0);
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
ISR(COUNT_ISR)
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
/** @brief Timer 0 ISR.

This ISR sends a dummy data record to the coordinator and toggles PC4
where there should be an LED.

If the clock scale factor is 5 (divide by 1024) and processor clock is 8MHz, a
transmission is sent every 8.4 seconds.
*/

ISR(TIMER0_OVF_vect)
{
    timeValue++;
    timeCount++;
    if (timeCount == 0)
        transmitMessage = true;
}

/****************************************************************************/
/** @brief USART ISR.

This ISR triggers when the Rx Complete flag is set, and grabs the incoming byte
and passes it to a buffer via a callback in the xbee library.
*/

#ifdef USE_RECEIVE_BUFFER
ISR(USART_RX_vect)
{
    put_receive_buffer(low(getch()));
}
#endif

/****************************************************************************/
