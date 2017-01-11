/**
@mainpage XBee AVR Node Test
@version 0.0.0
@author Ken Sarkies (www.jiggerjuice.info)
@date 25 September 2014
@brief Code for an ATTiny841/ATMega48 AVR with an XBee in a Remote Low Power Node

A message containing counts from a digital input is transmitted every few
seconds.

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
Software: AVR-GCC 4.8.2
@note
Target:   Any AVR with sufficient output ports and a timer
@note
Tested:   ATMega48 series, ATTiny841 at 8MHz internal clock.
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

#include <inttypes.h>
#include <stdbool.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "../../libs/defines.h"
#include "../../libs/buffer.h"
#include "../../libs/serial.h"
#include "../../libs/timer.h"
#include "../../libs/xbee.h"
#include <util/delay.h>
#include "xbee-node-test.h"

/** Convenience macros (we don't use them all) */
#define TRUE 1
#define FALSE 0

#define  _BV(bit) (1 << (bit))
#define inb(sfr) _SFR_BYTE(sfr)
#define inw(sfr) _SFR_WORD(sfr)
#define outb(sfr, val) (_SFR_BYTE(sfr) = (val))
#define outw(sfr, val) (_SFR_WORD(sfr) = (val))
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#define high(x) ((uint8_t) (x >> 8) & 0xFF)
#define low(x) ((uint8_t) (x & 0xFF))

/*****************************************************************************/
/* Global Variables */
/** Real Time Clock Structure

The 32 bit time can also be accessed as four bytes. Time scale is defined in
the information block.
*/

#define BUFFER_SIZE 60

static volatile union timeUnion
{
  volatile uint32_t timeValue;
  volatile uint8_t  timeByte[4];
} realTime;

/* timeCount measures off timer interrupt ticks to provide an extended time
between transmissions */
static uint8_t timeCount;
/* Counter keep track of external transitions on the digital input */
static uint32_t counter;

/** @name UART variables */
/*@{*/
static volatile uint8_t checkSum;     /**< Checksum on message contents */
/*@}*/

static uint32_t timeValue;
static uint8_t coordinatorAddress64[8];
static uint8_t coordinatorAddress16[2];

/* Local Prototypes */

static void inline hardwareInit(void);

/*****************************************************************************/
/** @brief Main Program */

int main(void)
{
    timeCount = 0;
    counter = 0;
    wdt_disable();                  /* Stop watchdog timer */
    hardwareInit();                 /* Initialize the processor specific hardware */
    uartInit();
    initBuffers();                  /* Set up communications buffers if used */
    timer0Init(0,RTC_SCALE);        /* Configure the timer */
    timeValue = 0;                  /* Reset timer */
    uint8_t messageState;           /**< Progress in message reception */
    sei();                          /* Enable global interrupts */

/* Set the coordinator addresses. All zero 64 bit address with "unknown" 16 bit
address avoids knowing the actual address, but may cause an address discovery
event. */
    for  (uint8_t i=0; i < 8; i++) coordinatorAddress64[i] = 0x00;
    coordinatorAddress16[0] = 0xFE;
    coordinatorAddress16[1] = 0xFF;

/* Check for association indication from the XBee.
Don't start until it is associated. */
    rxFrameType rxMessage;
    bool associated = FALSE;
    while (! associated)
    {
#ifdef TEST_PORT_DIR
        cbi(TEST_PORT,TEST_PIN);            /* Set pin off to indicate success */
#endif
        _delay_ms(200);
#ifdef TEST_PORT_DIR
        sbi(TEST_PORT,TEST_PIN);            /* Set pin off to indicate success */
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
                     (rxMessage.message.atResponse.data == 0) && \
                     (rxMessage.frameType == AT_COMMAND_RESPONSE) && \
                     (rxMessage.message.atResponse.atCommand1 == 65) && \
                     (rxMessage.message.atResponse.atCommand2 == 73));
    }
#ifdef TEST_PORT_DIR
    cbi(TEST_PORT,TEST_PIN);            /* Set pin off to indicate success */
#endif

/* Main loop */
    messageState = 0;
    for(;;)
    {
        wdt_reset();

/* Check for incoming messages */
/* The Rx message variable is re-used and must be processed before the next */
        uint8_t messageError = receiveMessage(&rxMessage, &messageState);
/* The frame types we are handling are 0x90 Rx packet and 0x8B Tx status */
        if ((messageError == XBEE_COMPLETE) && (rxMessage.frameType == RX_REQUEST))
        {
/* Toggle test port */
#ifdef TEST_PORT_DIR
            if (rxMessage.message.rxRequest.data[0] == 'L') cbi(TEST_PORT,TEST_PIN);
            if (rxMessage.message.rxRequest.data[0] == 'O') sbi(TEST_PORT,TEST_PIN);
#endif
/* Echo */
            sendTxRequestFrame(rxMessage.message.rxRequest.sourceAddress64,
                               rxMessage.message.rxRequest.sourceAddress16,
                               0, rxMessage.length-12,
                               rxMessage.message.rxRequest.data);
        }
    }
}

/****************************************************************************/
/** @brief Initialize the hardware for process measurement and XBee control

*/
void hardwareInit(void)
{
/* PB3 is XBee sleep request output. */
#ifdef SLEEP_RQ_PIN
    sbi(SLEEP_RQ_PORT_DIR,SLEEP_RQ_PIN);/* XBee Sleep Request */
    cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Set to keep XBee on */
#endif
/* PB4 is XBee on/sleep input. High is XBee on, low is asleep. */
#ifdef ON_SLEEP_PIN
    cbi(ON_SLEEP_PORT_DIR,ON_SLEEP_PIN);/* XBee On/Sleep Status input pin */
#endif
/* PB5 is XBee reset output. Pulse low to reset. */
#ifdef XBEE_RESET_PIN
    sbi(XBEE_RESET_PORT_DIR,XBEE_RESET_PIN);/* XBee Reset */
    sbi(XBEE_RESET_PORT,XBEE_RESET_PIN);    /* Set to keep XBee on */
#endif
/* PC0 is the board analogue input. */
/* PC1 is the battery monitor analogue input. */
/* PC5 is the battery monitor control output. Hold low for lower power drain. */
#ifdef VBATCON_PIN
    sbi(VBATCON_PORT_DIR,VBAT_PIN);
    cbi(VBATCON_PORT,VBAT_PIN);            /* Unset pullup */
#endif
#ifdef VBAT_PIN
    cbi(VBAT_PORT_DIR,VBAT_PIN);
    cbi(VBAT_PORT,VBAT_PIN);            /* Unset pullup */
#endif
/* PD5 is the counter input. */
#ifdef COUNT_PIN
    cbi(COUNT_PORT_DIR,COUNT_PIN);      /* XBee counter input pin */
    sbi(COUNT_PORT,COUNT_PIN);          /* Set pullup */
#endif
/* General output for a LED to be activated by the mirocontroller as desired. */
#ifdef TEST_PORT_DIR
    sbi(TEST_PORT_DIR,TEST_PIN);
    sbi(TEST_PORT,TEST_PIN);            /* Set pin on */
#endif

/* Counter: Use PCINT for the asynchronous pin change interrupt on the
count signal line. */
    sbi(PC_MSK,PC_INT);                 /* Mask */
    sbi(PC_IER,PC_IE);                  /* Enable */
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
    realTime.timeValue++;
    timeCount++;
    if (timeCount == 0)
    {
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
#ifdef TEST_PORT_DIR
        sbi(TEST_PORT,TEST_PIN);
    }
    if (timeCount == 26)
        cbi(TEST_PORT,TEST_PIN);
#else
    }
#endif
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
