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
Target:   AVR with sufficient output ports and a USART (USI not supported)
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
#include <string.h>
#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#if (MCU_TYPE==168)
#include "../libs/defines-M168.h"
#elif (MCU_TYPE==48)
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

/* Global variables used in ISRs */
volatile uint8_t counter0;      /**< Counters for signal pulses */
volatile uint8_t counter1;
volatile uint8_t counter2;
volatile uint8_t counter3;
volatile uint8_t wdtCounter;    /**< Counter to extend WDT range */
volatile uint8_t signalAction;

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
    wdtInit(WDT_TIME);
/** Initialize the UART library, pass the baudrate and avr cpu clock 
(uses the macro UART_BAUD_SELECT()). Set the baudrate to a predefined value. */
    uartInit();
    sei();

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
    signalAction = FALSE;
/*---------------------------------------------------------------------------*/
/* Main loop forever. */
    for(;;)
    {
/* Power down the AVR until an interrupt occurs */
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sleep_cpu();
//        cli();              /* Disable interrupts until ready to sleep again*/

/* When the watchdog timer has expired, see if there is anything to send */
        if (signalAction)
        {
            uint32_t count = (uint32_t)counter0+(uint32_t)(counter1<<8)
                            +(uint32_t)(counter2<<16)+(uint32_t)(counter3<<24);
//            if (count > 0)
            {
                uint8_t data[9];
                uint8_t i;
                for (i = 0; i < 8; i++)
                {
	                data[7-i] = "0123456789ABCDEF"[count & 0x0F];
	                count >>= 4;
                }
                data[8] = 0;
                sendMessage(data);
                counter0 = 0;
                counter1 = 0;
                counter2 = 0;
                counter3 = 0;
            }
            signalAction = FALSE;
        }
        handleReceiveMessage();
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
#ifdef TEST_PIN
/* Simply toggle test port in response to simple commands */
            if (rxMessage.message.rxRequest.data[0] == 'L')
                cbi(TEST_PORT,TEST_PIN);
            if (rxMessage.message.rxRequest.data[0] == 'O')
                sbi(TEST_PORT,TEST_PIN);
#endif
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

/** Counter: Use Interrupt 0 with low level triggering in power down mode.
Only low level triggering is possible in power down as all clocks are halted. */
//    sbi(IMSK,INT0);                     /* Enable Interrupt 0 */
//    outb(INT_CR,inb(INT_CR) & ~0x03);   /* Low level trigger on interrupt 0 */
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

@param[in] uint8_t timeout: a register setting, 9 or less (see datasheet).
*/
void wdtInit(uint8_t timeout)
{
    outb(WDT_CSR,0);     /* Clear the WDT register */
    if (timeout > 9) timeout = 9;
    uint8_t wdtcsrSetting = (timeout & 0x07);
    if (timeout > 7) wdtcsrSetting |= _BV(WDP3);
    outb(WDT_CSR,wdtcsrSetting | _BV(WDIE));
}

/****************************************************************************/
/** @brief Wake the XBee and send a message

Wakeup the XBee and wait for a bit. Send a string message and sleep it again.

@param[in]  uint8_t* data: pointer to a string of data (ending in 0).
*/
void sendMessage(uint8_t* data)
{
    cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Wakeup XBee */
    _delay_ms(PIN_WAKE_PERIOD);
    sendTxRequestFrame(coordinatorAddress64, coordinatorAddress16,0,strlen(data),data);
    sbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Request XBee Sleep */
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

/*--------------------------------------------------------------------------*/
/** @brief Convert an integer to a string in 16 bit ASCII hex form

@param[in] int32_t value: integer value to be printed.
*/

void intToHex(uint32_t value, uint8_t buffer[])
{
	uint8_t i;

	for (i = 7; i >= 0; i--)
	{
		buffer[i] = "0123456789ABCDEF"[value & 0x0F];
		value >>= 4;
	}
    buffer[8] = 0;
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
do anything useful, so we go back to sleep again until enough such events have
occurred. Here just reset the counter. Check in the main program if some action
is needed.

NOTE: sendMessage is used to signal a wake period.
*/
#if (MCU_TYPE==4313)
ISR(WDT_OVERFLOW_vect)
#else
ISR(WDT_vect)
#endif
{
    if (wdtCounter++ > ACTION_COUNT)
    {
        sendMessage("aA");
        wdtCounter = 0;
        signalAction = TRUE;
        _delay_ms(1);
    }
}

