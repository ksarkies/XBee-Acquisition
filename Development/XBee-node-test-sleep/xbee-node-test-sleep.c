/**
@mainpage AVR XBee Node Test with Sleep Mode
@version 0.0
@author Ken Sarkies (www.jiggerjuice.info)
@date 14 January 2017

@brief Code for an AVR with an XBee in a Remote Low Power Node

A message containing counts from a digital input is transmitted every few
seconds.

This code forms the interface between an XBee networking device using ZigBee
stack, and a counter signal providing count and battery voltage
measurements for communication to a base controller.

The code is written to make use of sleep modes and other techniques to
minimize power consumption. The AVR is woken by a count signal, updates the
running total, and sleeps. This does not receive any messages.

The watchdog timer is used to wake the AVR at intervals to transmit the count
to the XBee. The XBee is then woken again after a short interval to pass on any
base station messages to the AVR.

@note
CTS must be set in the XBee and USE_HARDWARE_FLOW also enabled.
@note
Fuses: Disable the "WDT Always On" fuse and disable the BOD fuse.
@note
Software: AVR-GCC 4.8.2
@note
Target:   AVR with sufficient output ports and a USART (USI not supported)
@note
Tested:   ATTiny4313 with 1MHz internal clock. ATMega48 with 8MHz clock,
          ATTiny841 with 8MHz clock.

*/
/****************************************************************************
 *   Copyright (C) 2017 by Ken Sarkies (www.jiggerjuice.info)               *
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
#include "../../libs/serial.h"
#include "../../libs/xbee.h"
#include <util/delay.h>
#include "xbee-node-test-sleep.h"

/****************************************************************************/
/* Global variables */
uint8_t coordinatorAddress64[8];
uint8_t coordinatorAddress16[2];
uint8_t rxOptions;

/* Counter keep track of external transitions on the digital input */
static uint32_t counter;
static uint8_t wdtCounter;             /* Data Transmission Timer */
static bool transmitMessage;

/****************************************************************************/
/* Local Prototypes */

void hardwareInit(void);
void wdtInit(const uint8_t waketime);
void sendDataCommand(const uint8_t command, const uint32_t datum);
void sendMessage(const char* data);
void sleepXBee(void);
void wakeXBee(void);
void powerDown(void);
void powerUp(void);

/*---------------------------------------------------------------------------*/
/** @brief      Main Program

An interrupt service routine records a transition on the counter input. This
wakes the AVR which is sent back to sleep when the counts have settled.

The watchdog timer interrupt service routine provides a time tick of 8 seconds
(maximum). This wakes the AVR which counts off a number of ticks to extend the
sleep interval. Then the data transmission process begins within a loop.
*/

int main(void)
{

/*  Initialise hardware */
    hardwareInit();
/** Initialize the UART library, pass the baudrate and avr cpu clock 
(uses the macro UART_BAUD_SELECT()). Set the baudrate to a predefined value. */
    uartInit();
    wdtInit(WDT_TIME);              /* Set up watchdog timer */

/* Set the coordinator addresses. All zero 64 bit address with "unknown" 16 bit
address avoids knowing the actual address, but may cause an address discovery
event. */
    for  (uint8_t i=0; i < 8; i++) coordinatorAddress64[i] = 0x00;
    coordinatorAddress16[0] = 0xFE;
    coordinatorAddress16[1] = 0xFF;

/* Check for association indication from the XBee.
Don't start until it is associated. */
    uint8_t messageState;           /**< Progress in message reception */
    rxFrameType rxMessage;
    bool associated = false;
    while (! associated)
    {
#ifdef TEST_PORT_DIR
        cbi(TEST_PORT,TEST_PIN);    /* Set pin off */
#endif
        _delay_ms(200);
#ifdef TEST_PORT_DIR
        sbi(TEST_PORT,TEST_PIN);    /* Set pin on */
#endif
        _delay_ms(200);
        sendATFrame(2,"AI");

/* The frame type we are handling is 0x88 AT Command Response */
        uint16_t timeout = 0;
        messageState = 0;
        uint8_t messageError = XBEE_INCOMPLETE;
        while (messageError == XBEE_INCOMPLETE)
        {
            messageError = receiveMessage(&rxMessage, &messageState);
            if (timeout++ > 30000) break;
        }
/* If errors occur, or frame is the wrong type, just try again */
        associated = ((messageError == XBEE_COMPLETE) && \
                     (rxMessage.message.atResponse.data[0] == 0) && \
                     (rxMessage.frameType == AT_COMMAND_RESPONSE) && \
                     (rxMessage.message.atResponse.atCommand1 == 65) && \
                     (rxMessage.message.atResponse.atCommand2 == 73));
    }
#ifdef TEST_PORT_DIR
    cbi(TEST_PORT,TEST_PIN);            /* Set pin off to indicate success */
#endif

/*---------------------------------------------------------------------------*/
/* Main loop forever. */

/* Initialise watchdog timer count */
    wdtCounter = 0;

    counter = 0;

/* Start off with XBee asleep and wait for interrupt to wake it up */
    sleepXBee();

    transmitMessage = false;
    messageState = 0;
    for(;;)
    {
        sei();
        powerDown();            /* Turn off all peripherals for sleep */
/* Power down the AVR to deep sleep until an interrupt occurs */
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_mode();

/* Check for the timer interrupt to indicate it is time for a message to go out.
Wakeup the XBee and send putting it back to sleep afterwards. */
        if (transmitMessage)
        {
#ifdef TEST_PORT_DIR
            sbi(TEST_PORT,TEST_PIN);
#endif
/* Power up only essential peripherals for transmission of results. */
            powerUp();
            wakeXBee();
            uint8_t buffer[12];
            uint8_t i;
            char checksum = -(counter + (counter >> 8) + (counter >> 16) + (counter >> 24));
            uint32_t value = counter;
            for (i = 0; i < 10; i++)
            {
                if (i == 8) value = checksum;
                buffer[10-i] = "0123456789ABCDEF"[value & 0x0F];
                value >>= 4;
            }
            buffer[11] = 0;             /* String terminator */
            buffer[0] = 'D';            /* Data Command */
            sendTxRequestFrame(coordinatorAddress64, coordinatorAddress16,0,11,buffer);
            counter = 0;            /* Reset counter value */
            sleepXBee();
            transmitMessage = false;
#ifdef TEST_PORT_DIR
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
/** @brief Initialize the hardware for process measurement

Set unused ports to inputs and disable power to all unused peripherals.
Set the process counter interrupt to INT0.
*/
void hardwareInit(void)
{
/* Set input ports to pullups and disable digital input buffers on AIN inputs.
Refer to the defines files for the defined symbols. */

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

/* Set I/O ports to desired directions and initial settings */

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
    cbi(VBATCON_PORT,VBATCON_PIN);              /* Turn off to power down */
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
    sbi(TEST_PORT,TEST_PIN);                    /* Set pin on to start */
#endif

/* Counter: Use PCINT for the asynchronous pin change interrupt on the
count signal line. */
    sbi(PC_MSK,PC_INT);                         /* Mask */
    sbi(PC_IER,PC_IE);                          /* Enable */

    powerDown();                        /* Turns off all peripherals */
    powerUp();                          /* Turns on essential peripherals only */
}

/****************************************************************************/
/** @brief Power down all peripherals for Sleep

*/
void powerDown(void)
{
#ifdef ADC_ONR
    cbi(ADC_ONR,AD_EN); /* Disable the ADC first to ensure power down works */
#endif
    outb(PRR,0xFF);     /* power down all controllable peripherals */
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
}

/****************************************************************************/
/** @brief Initialize the hardware for process measurement

Set unused ports to inputs and disable power to all unused peripherals.
Set the process counter interrupt to INT0.
*/
void powerUp(void)
{
#ifdef PRR_USART0
    cbi(PRR,PRR_USART0);/* power up USART0 */
#endif
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
remains in interrupt mode. Note that the changing of these modes must follow
a strict protocol as outlined in the datasheet.

IMPORTANT: Disable the "WDT Always On" fuse.

@param[in] uint8_t waketime: a register setting, 9 or less (see datasheet).
*/
void wdtInit(const uint8_t waketime)
{
    uint8_t timeout = waketime;
    if (timeout > 9) timeout = 9;
    uint8_t wdtcsrSetting = (timeout & 0x07);
    if (timeout > 7) wdtcsrSetting |= _BV(WDP3);
    wdtcsrSetting |= _BV(WDIE); /* Set WDT interrupt enable */
    outb(MCUSR,0);              /* Clear the WDRF flag to allow WDE to reset */
#ifdef WDCE
/* This is the required change sequence to clear WDE and set enable and scale
as required for ATMega48 and most other devices. */
    outb(WDTCSR,_BV(WDCE) | _BV(WDE));  /* Set change enable */
#else
/* For later devices, notably ATTiny441 series, the CPU CCP register needs to be
written with a change enable key. Then setting WDIE will also clear WDE. */
    outb(CCP,0xD8);
#endif
    outb(WDTCSR,wdtcsrSetting); /* Set scaling factor and enable WDT interrupt */
    sei();
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
    while (inbit(ON_SLEEP_PORT,ON_SLEEP_PIN) == 0); /* Wait for wakeup */
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
    uint8_t countSignal = inbit(COUNT_PORT,COUNT_PIN);
    if (countSignal > 0)
    {
        _delay_us(100);
        countSignal = inbit(COUNT_PORT,COUNT_PIN);
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
    sbi(WDTCSR,WDIE);       /* Set interrupt enable again in case changed */
    if (wdtCounter > ACTION_COUNT)
    {
        transmitMessage = true;
        wdtCounter = 0;
    }
}

