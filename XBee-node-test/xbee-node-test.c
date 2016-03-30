/**
@mainpage XBee AVR Node Test
@version 0.0.0
@author Ken Sarkies (www.jiggerjuice.info)
@date 25 September 2014
@brief Code for an ATMega48 AVR with an XBee in a Remote Low Power Node 

This code forms the core of an interface between an XBee networking device
using ZigBee stack, and a data acquisition unit making a variety of
measurements for communication to a base controller.

This application works with AVRs having a bootloader block or for situations
where a bootloader is not required. It has no code referring to a bootloader.

The board targetted is the test board developed for the project using the
ATMega48 series microcontrollers. See the hardwareInit() function for
documented I/O ports.

Count interrupt via PCINT2 is handled.

@note
Software: AVR-GCC 4.8.2
@note
Target:   Any AVR with sufficient output ports and a timer
@note
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

#include <inttypes.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "../libs/defines.h"
#include <util/delay.h>
#include "../libs/serial.h"
#include "../libs/timer.h"
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
/** Real Time Clock Global Variable

The 32 bit time can also be accessed as four bytes. Time scale is defined in
the information block.
*/
volatile union timeUnion
{
  volatile uint32_t timeValue;
  volatile uint8_t  timeByte[4];
} time;

/* timeCount measures off timer interrupt ticks to provide an extended time
between transmissions */
uint8_t timeCount;
/* Counter keep track of external transitions on the digital input */
uint32_t counter;

/*****************************************************************************/
/* Global Variables */

/** @name UART variables */
/*@{*/
    volatile uint16_t uartInput;   /**< Character and errorcode read from uart */
    volatile uint8_t lastError;    /**< Error code for transmission back */
    volatile uint8_t checkSum;     /**< Checksum on message contents */
/*@}*/

    uint32_t timeValue;
    uint8_t messageState;           /**< Progress in message reception */
    uint8_t messageReady;           /**< Indicate that a message is ready */
    uint8_t messageError;
    uint8_t coordinatorAddress64[8];
    uint8_t coordinatorAddress16[2];
    uint8_t rxOptions;

/*****************************************************************************/
/* Local Prototypes */

static void inline hardwareInit(void);
static void inline timer0Init(uint8_t mode,uint16_t timerClock);
static uint16_t timer0Read(void);
void sendTxRequestFrame(uint8_t sourceAddress64[], uint8_t sourceAddress16[],
                        uint8_t radius, uint8_t length, uint8_t data[]);

/*****************************************************************************/
/** @brief Main Program */

int main(void)
{
    timeCount = 0;
    counter = 0;
    wdt_disable();                  /* Stop watchdog timer */
    hardwareInit();                 /* Initialize the processor specific hardware */
    timer0Init(0,RTC_SCALE);        /* Configure the timer */
    timeValue = 0;                  /* reset timer */
    uartInit();

/* Set the coordinator addresses. All zero 64 bit address with "unknown" 16 bit
address avoids knowing the actual address, but may cause an address discovery
event. */
    for  (uint8_t i=0; i < 8; i++) coordinatorAddress64[i] = 0x00;
    coordinatorAddress16[0] = 0xFE;
    coordinatorAddress16[1] = 0xFF;
    messageState = 0;
    messageReady = FALSE;
    messageError = 0;
/* Main loop */
    for(;;)
    {
        wdt_reset();

/* Check for incoming messages */
/* The Rx message variable is re-used and must be processed before the next */
        rxFrameType rxMessage;
/* Wait for data to appear */
        uint16_t inputChar = getch();
        messageError = high(inputChar);
        if (messageError != NO_DATA)
        {
/* Pull in the next character and look for message start */
/* Read in the length (16 bits) and frametype then the rest to a buffer */
            uint8_t inputValue = low(inputChar);
            switch(messageState)
            {
/* Sync character */
                case 0:
                    if (inputChar == 0x7E) messageState++;
                    break;
/* Two byte length */
                case 1:
                    rxMessage.length = (inputChar << 8);
                    messageState++;
                    break;
                case 2:
                    rxMessage.length += inputValue;
                    messageState++;
                    break;
/* Frame type */
                case 3:
                    rxMessage.frameType = inputValue;
                    rxMessage.checksum = inputValue;
                    messageState++;
                    break;
/* Rest of message, maybe include addresses or just data */
                default:
                    if (messageState > rxMessage.length + 3)
                        messageError = STATE_MACHINE;
                    else if (rxMessage.length + 3 > messageState)
                    {
                        rxMessage.message.array[messageState-4] = inputValue;
                        messageState++;
                        rxMessage.checksum += inputValue;
                    }
                    else
                    {
                        messageReady = TRUE;
                        messageState = 0;
                        if (((rxMessage.checksum + inputValue + 1) & 0xFF) > 0)
                            messageError = CHECKSUM;
                    }
            }
        }
/* The frame types we are handling are 0x90 Rx packet and 0x8B Tx status */
        if (messageReady)
        {
            messageReady = FALSE;
            if (messageError > 0) sendch(messageError);
            else if (rxMessage.frameType == RX_REQUEST)
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
}

/****************************************************************************/
/** @brief Build and transmit a Tx Request frame

A data message for the XBee API is formed and transmitted.

@param[in]:   uint8_t sourceAddress64[]. Address of parent or 0 for coordinator.
@param[in]:   uint8_t sourceAddress16[].
@param[in]:   uint8_t radius. Broadcast radius or 0 for maximum network value.
@param[in]:   uint8_t dataLength. Length of data array.
@param[in]:   uint8_t data[]. Define array size to be greater than length.
*/
void sendTxRequestFrame(uint8_t sourceAddress64[], uint8_t sourceAddress16[],
                         uint8_t radius, uint8_t dataLength, uint8_t data[])
{
    txFrameType txMessage;
    txMessage.frameType = TX_REQUEST;
    txMessage.message.txRequest.frameID = 0x02;
    txMessage.length = dataLength+14;
    for (uint8_t i=0; i < 8; i++)
    {
        txMessage.message.txRequest.sourceAddress64[i] = sourceAddress64[i];
    }
    for (uint8_t i=0; i < 2; i++)
    {
        txMessage.message.txRequest.sourceAddress16[i] = sourceAddress16[i];
    }
    txMessage.message.txRequest.radius = radius;
    txMessage.message.txRequest.options = 0;
    for (uint8_t i=0; i < dataLength; i++)
    {
        txMessage.message.txRequest.data[i] = data[i];
    }
/* Build and transmit a basic frame.
Send preamble, then data block, followed by computed checksum */
    sendch(0x7E);
    sendch(high(txMessage.length));
    sendch(low(txMessage.length));
    sendch(txMessage.frameType);
    txMessage.checksum = txMessage.frameType;
    for (uint8_t i=0; i < txMessage.length-1; i++)
    {
        uint8_t txData = txMessage.message.array[i];
        sendch(txData);
        txMessage.checksum += txData;
    }
    sendch(0xFF-txMessage.checksum);
}

/****************************************************************************/
/** @brief Initialize the hardware for process measurement and XBee control

*/
void hardwareInit(void)
{
/* General output for a LED to be activated by the mirocontroller as desired. */
#ifdef TEST_PORT_DIR
    sbi(TEST_PORT_DIR,TEST_PIN);
    sbi(TEST_PORT,TEST_PIN);            /* Set pullup */
#endif
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
    sbi(XBEE_RESET_PORT_DIR,XBEE_RESET_PIN);/* XBee Sleep Request */
    sbi(XBEE_RESET_PORT,XBEE_RESET_PIN);    /* Set to keep XBee on */
#endif
/* PC0 is the board analogue input. */
/* PC1 is the battery monitor analogue input. */
/* PC5 is the battery monitor control output. Hold low for lower power drain. */
    cbi(VBAT_PORT_DIR,VBAT_PIN);
    cbi(VBAT_PORT,VBAT_PIN);            /* Unset pullup */
/* PD5 is the counter input. */
#ifdef COUNT_PIN
    cbi(COUNT_PORT_DIR,COUNT_PIN);      /* XBee counter input pin */
    sbi(COUNT_PORT,COUNT_PIN);          /* Set pullup */
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
*/

ISR(TIMER0_OVF_vect)
{
    time.timeValue++;
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
/** @brief   Initialise Timer 0

This function will initialise the timer with the mode of operation and the
clock rate to be used. An error will be returned if the timer is busy.

Because ATMega64, ATMega128, ATMega103 offer different scale factors, there
needs to be a conversion provided between the specification here and the scale
setting. The additional clock settings provided for those MCUs are not used
here, nor are the external clock settings of the remaining MCUs.

This code is also valid for the ATTiny4313.

Timer 0 is typically an 8-bit timer and has very basic functionality. Some MCUs
offer PWM capability while most do not.

    @param  mode
            Ignored for simple timers.
    @param  timerClock
            00  Stopped
            01  F_CLK
            02  F_CLK/8
            03  F_CLK/64
            04  F_CLK/256
            05  F_CLK/1024

The timer continues to run until it is stopped by calling this function with
timerClock=0. At the moment, mode does nothing.
*/

void timer0Init(uint8_t mode,uint16_t timerClock)
{
  if (timerClock > 5) timerClock = 5;
#if defined(__AVR_ATMega64__) || \
    defined(__AVR_ATMega128__) || \
    defined(__AVR_ATMega103)
/* Rescale clock values to match those of the above MCUs */
  if (timerClock > 2) ++timerClock;
  if (timerClock > 4) ++timerClock;
#endif
  outb(TIMER_CONT_REG0,((inb(TIMER_CONT_REG0) & 0xF8)|(timerClock & 0x07)));
#if defined (TCNT0L)
  outb(TCNT0L,0);                    /* 16 bit - clear both registers */
  outb(TCNT0H,0);
#else
  outb(TCNT0,0);                    /* Clear the register */
#endif
#if (TIMER_INTERRUPT_MODE == 1)
  sbi(TIMER_FLAG_REG0, TOV0);       /* Force clear the interrupt flag */
  sbi(TIMER_MASK_REG0, TOIE0);      /* Enable the overflow interrupt */
  sei();
#endif
}

/****************************************************************************/
/** @brief   Read Timer 0

This function will return the current timer value as a 16 bit unsigned integer
even if the timer is only 8 bit. This allows for a possibility of a 16 bit
timer being at timer 0 (so far this is not the case in any MCU).

In the event of a 16 bit register, the hardware registers must be accessed
high byte first. The avr-gcc compiler does this automatically.

    @return Timer Value.
*/

uint16_t timer0Read()
{
#if defined (TCNT0L)
  return (int16_t)inb(TCNT0L)+((int16_t)inb(TCNT0H)<<8);
#else
  return (int16_t) inb(TCNT0);
#endif
}

/****************************************************************************/
