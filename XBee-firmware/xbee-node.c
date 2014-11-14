/**
@mainpage AVR XBee Node Firmware
@version 0.0
@author Ken Sarkies (www.jiggerjuice.net)
@date 18 July 2014

@brief Code for an AVR with an XBee in a Remote Low Power Node

This code forms the interface between an XBee networking device using ZigBee
stack, and a counter signal providing count and battery voltage
measurements for communication to a base controller.

The code is written to make use of sleep modes and other techniques to
minimize power consumption. The AVR is woken by a count signal, updates the
running total, and sleeps.

The watchdog timer is used to wake the AVR at intervals to transmit the count
and battery voltage to the XBee. The XBee is then woken again after a short
interval to pass on any base station messages to the AVR.

@note
Fuses: Disable the "WDT Always On" fuse and disable the BOD fuse.
@note
Software: AVR-GCC 4.8.2
@note
Target:   Any AVR with sufficient output ports and a timer
@note
Tested:   ATTiny4313 at 1MHz internal clock.

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
#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#if (MCU_TYPE==168)
#include "../libs/defines-M168.h"
#elif (MCU_TYPE==4313)
#include "../libs/defines-T4313.h"
#elif (MCU_TYPE==841)
#include "../libs/defines-T841.h"
#else
#error "Processor not defined"
#endif

#include <util/delay.h>

#include "xbee-node.h"
#include "../libs/serial.h"

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
    uint8_t counter0;               /**< Counters for signal pulses */
    uint8_t counter1;
    uint8_t counter2;
    uint8_t counter3;
    uint8_t wdtCounter;             /**< Counter to extend WDT range */

/** @name UART variables */
/*@{*/
    volatile uint16_t uartInput;    /**< Character and errorcode read from uart */
    volatile uint8_t lastError;     /**< Error code for transmission back */
    volatile uint8_t checkSum;      /**< Checksum on message contents */
/*@}*/

    uint8_t messageState;           /**< Progress in message reception */
    uint8_t messageReady;           /**< Indicate that a message is ready */
    uint8_t coordinatorAddress64[8];
    uint8_t coordinatorAddress16[2];
    uint8_t rxOptions;
/*---------------------------------------------------------------------------*/
int main(void)
{

/*  Initialise hardware */
    hardwareInit();
    wdtInit();
/** Initialize the UART library, pass the baudrate and avr cpu clock 
(uses the macro UART_BAUD_SELECT()). Set the baudrate to a predefined value. */
    uartInit();

/* Set the coordinator addresses. All zero 64 bit address with "unknown" 16 bit
address avoids knowing the actual address, but may cause an address discovery
event. */
    for  (uint8_t i=0; i < 8; i++) coordinatorAddress64[i] = 0x00;
    coordinatorAddress16[0] = 0xFE;
    coordinatorAddress16[1] = 0xFF;
    messageState = 0;
    messageReady = FALSE;

/* initialise process counter */
    counter0 = 0;
    counter1 = 0;
    counter2 = 0;
    counter3 = 0;

/* Initialise WDT counter */
    wdtCounter = 0;
/*---------------------------------------------------------------------------*/
/* Main loop forever. */
    for(;;)
    {
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sleep_cpu();
        if (wdtCounter == 0)
        {
/* Wakeup the XBee and wait for a bit. Send a message and sleep it again
after a delay. */
            cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Wakeup XBee */
            _delay_ms(PIN_WAKE_PERIOD);
            uint8_t data[12] = "DNode End-3";
            sendTxRequestFrame(coordinatorAddress64, coordinatorAddress16,0,11,data);
            sbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Request XBee Sleep */

/* Toggle the test port to see if WDT interrupt working. */
//            if ((inb(TEST_PORT) & _BV(TEST_PIN)) > 0) cbi(TEST_PORT,TEST_PIN);
//            else sbi(TEST_PORT,TEST_PIN);
        }
//        handleReceiveMessage();
    }
}

/****************************************************************************/
/** @brief Check for incoming messages and respond.

An incoming message is assembled over multiple calls to this function, and
when complete messageReady is set. The message is then parsed and actions
taken as appropriate.

Note: The Rx message variable is re-used and must be processed before the next
arrives.

Globals: messageReady, messageState
*/
void handleReceiveMessage(void)
{
    rxFrameType rxMessage;
/* Wait for data to appear */
    uint16_t inputChar = getch();
    uint8_t messageError = high(inputChar);
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
/* Respond to received message */
/* The frame types we are handling are 0x90 Rx packet and 0x8B Tx status */
    if (messageReady)
    {
        messageReady = FALSE;
        if (messageError > 0) sendch(messageError);
        else if (rxMessage.frameType == RX_REQUEST)
        {
/* Simply toggle test port in response to simple commands */
            if (rxMessage.message.rxRequest.data[0] == 'L')
                cbi(TEST_PORT,TEST_PIN);
            if (rxMessage.message.rxRequest.data[0] == 'O')
                sbi(TEST_PORT,TEST_PIN);
/* TEST: Echo message back */
            sendTxRequestFrame(rxMessage.message.rxRequest.sourceAddress64,
                            rxMessage.message.rxRequest.sourceAddress16,
                            0, rxMessage.length-12,
                            rxMessage.message.rxRequest.data);
        }
    }
}

/****************************************************************************/
/** @brief Initialize the hardware for process measurement

Set unused ports to inputs and disable all unused peripherals.
Set the process counter interrupt to INT0.
*/
void hardwareInit(void)
{
/* Set PRR to disable all peripherals except USART.
Set input ports to pullups and disable digital input buffers on AIN inputs. */

    outb(PRR,0x0F);     /* power down timer/counters, USI */
    cbi(PRR,PRUSART);   /* power up USART */
    outb(DDRA,0);       /* set as inputs */
    outb(PORTA,0x07);   /* set pullups   */
    outb(DDRB,0);       /* set as inputs */
    outb(PORTB,0xFF);   /* set pullups   */
    outb(DDRD,0);       /* set as inputs */
    outb(PORTD,0x1F);   /* set pullups   */
    sbi(ACSR,ACD);      /* turn off Analogue Comparator */
    outb(DIDR,3);       /* turn off digital input buffers */

/* Set output ports to desired directions and initial settings */

    sbi(TEST_PORT_DIR,TEST_PIN);        /* Test port */
    sbi(VBAT_PORT_DIR,VBAT_PIN);
    cbi(VBAT_PORT,VBAT_PIN);            /* Battery Measure */
    sbi(SLEEP_RQ_PORT_DIR,SLEEP_RQ_PIN);
    sbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Default XBee Sleep */

/** Counter: Use Interrupt 0 with rising edge triggering in power down mode */
    sbi(GIMSK,INT0);                    /* Enable Interrupt 0 */
    outb(MCUCR,inb(MCUCR) | 0x03);      /* Rising edge trigger on interrupt 0 */
}

/****************************************************************************/
/** @brief Initialize the watchdog timer to interrupt on maximum delay

*/
void wdtInit(void)
{
/* Initialize the Watchdog timer to interrupt. */
/* IMPORTANT: Disable the "WDT Always On" fuse so that WDT can be turned off. */
    wdt_disable();     /* watchdog timer turn off ready for setup. */
    outb(WDTCR,0);
/* Set the WDT with WDE clear, interrupts enabled, interrupt mode set, and
maximum timeout 8 seconds to give continuous interrupt mode. */
    sei();
//    outb(WDTCR,_BV(WDIE)|_BV(WDP3)|_BV(WDP0));
    outb(WDTCR,_BV(WDIE));  /* For test only: 32 ms timeout */

}

/****************************************************************************/
/** @brief Build and transmit a basic frame

Send preamble, then message block, followed by computed checksum.

@param[in]  txFrameType txMessage
*/
void sendBaseFrame(txFrameType txMessage)
{
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
/** @brief Build and transmit a Tx Request frame

A data message for the XBee API is formed and transmitted.

@param[in]:   uint8_t sourceAddress64[]. Address of parent or 0 for coordinator.
@param[in]:   uint8_t sourceAddress16[].
@param[in]:   uint8_t radius. Broadcast radius or 0 for maximum network value.
@param[in]:   uint8_t dataLength. Length of data array.
@param[in]:   uint8_t data[]. Define array size to be greater than length.
*/
void sendTxRequestFrame(uint8_t sourceAddress64[], uint8_t sourceAddress16[],
                        uint8_t radius, uint8_t dataLength,
                        uint8_t data[])
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
    sendBaseFrame(txMessage);
}

/****************************************************************************/
/** @brief Interrupt on INT0.

Increment a counter 32 bits.

When a counter overflows the test is for zero value. The first test will only
trigger after 256 ISR calls, so the additional time of the following code
is negligible.

The code done this way looks clumsy but is much faster and gives the same
binary code size as incrementing a 32 bit integer, even using optimization
level 3 or fast.

This code requires 23 clock cycles. It is almost as fast as direct assembler
can make it with the exception that register R1 is pushed, cleared to zero and
a cpse instruction used for the test, needing 6 cycles. If a cpi and breq is
used, no register is involved and the time is reduced to 2 cycles, giving a
time saving of 17%.
*/
ISR(INT0_vect)
{
    counter0++;
    if (counter0 == 0)
    {
        counter1++;
        if (counter1 == 0)
        {
            counter2++;
            if (counter2 == 0)
            {
                counter3++;
            }
        }
    }
}
/****************************************************************************/
/** @brief Interrupt on WDT.

When the WDT timer overflows, 8 seconds will have elapsed which is too short to
do anything useful, so we go back to sleep again until 75 such events have
occurred (10 minutes). Here just reset the count. Check in the main program
if some action is needed.
*/
ISR(WDT_OVERFLOW_vect )
{
    if (wdtCounter++ > ACTION_COUNT)
    {
        wdtCounter = 0;
    }
}

