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
#include <stdbool.h>
#include <avr/sfr_defs.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>

#include "../libs/defines.h"
#include "../libs/serial.h"
#include <util/delay.h>
#include "xbee-node.h"

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
uint8_t coordinatorAddress64[8];
uint8_t coordinatorAddress16[2];
uint8_t rxOptions;

uint32_t counter;
uint8_t wdtCounter;

/*---------------------------------------------------------------------------*/
int main(void)
{

/*  Initialise hardware */
    hardwareInit();
    wdtInit(WDT_TIME);
/** Initialize the UART library, pass the baudrate and avr cpu clock 
(uses the macro UART_BAUD_SELECT()). Set the baudrate to a predefined value. */
    uartInit();

/* Set the coordinator addresses. All zero 64 bit address with "unknown" 16 bit
address avoids knowing the actual address, but may cause an address discovery
event. */
    for  (uint8_t i=0; i < 8; i++) coordinatorAddress64[i] = 0x00;
    coordinatorAddress16[0] = 0xFE;
    coordinatorAddress16[1] = 0xFF;

/* Initialise process counter */
    counter = 0;

/* Initialise watchdog timer count */
    wdtCounter = 0;
    bool stayAwake = false;         /* Keep awake in response to command */

    sei();
/*---------------------------------------------------------------------------*/
/* Main loop forever. */
    for(;;)
    {
        if (! stayAwake) sleepXBee();
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
8 seconds will be too short to do anything useful, so go back to sleep again
until enough such events have occurred. */
            if (wdtCounter > ACTION_COUNT)
            {
/* Now ready to initiate a data transmission. */
                wdtCounter = 0; /* Reset the WDT counter for next time */
/* Initiate a data transmission to the base station. This also serves as a
means of notifying the base station that the AVR is awake. */
                bool txDelivered = false;
                bool cycleComplete = false;
                uint8_t txCommand = 'C';
                bool transmit = true;
                uint8_t retry = 0;      /* Retries to get base response OK */
                while (! cycleComplete)
                {
                    if (transmit)
                    {
                        sendDataCommand(txCommand,lastCount);
                        txDelivered = false;
                    }
                    transmit = false;   /* Prevent transmissions until told */

/* Deal with incoming message assembly for Tx Status and base station response
or command reception. */
                    bool timeout = false;
                    bool txStatusReceived = false;
                    rxFrameType rxMessage;      /* Received frame */
                    rxFrameType inMessage;      /* Buffered data frame */
                    uint8_t messageState = 0;
                    uint16_t timeResponse = 0;
                    bool packetError = false;
                    bool packetReady = false;
                    while (true)
                    {

/* Read in part of an incoming frame. */
                        uint8_t messageStatus = receiveMessage(&rxMessage, &messageState);
                        if (messageStatus == NO_DATA) timeResponse++;
                        else
                        {
                            timeResponse = 0;
/* Got a frame complete without error. */
                            if (messageStatus == COMPLETE)
                            {
/* XBee Receive frame. Copy to a buffer for later processing. */
                                if (rxMessage.frameType == 0x90)
                                {
                                    inMessage.length = rxMessage.length;
                                    inMessage.checksum = rxMessage.checksum;
                                    inMessage.frameType = rxMessage.frameType;
                                    for (uint8_t i=0; i<RF_PAYLOAD; i++)
                                        inMessage.message.rxRequest.data[i] =
                                            rxMessage.message.rxRequest.data[i];
                                    packetReady = true;
                                }
/* Tx Status frame. Check if the message was delivered. Action to repeat will
happen ONLY if txDelivered is false and txStatusReceived is true. */
                                else if (rxMessage.frameType == 0x8B)
                                {
                                    txDelivered = (rxMessage.message.txStatus.deliveryStatus == 0);
                                    txStatusReceived = true;
                                }
                                messageState = 0;   /* Reset in case of a repeat */
                                break;
                            }
/* Zero message status means it is part way through so just continue on. */
/* If nonzero then this means other errors occurred (namely checksum or packet
length is wrong). */
                            else if (messageStatus > 0)
                            {
                                if (rxMessage.frameType == 0x90)
                                {
/* With errors in the data frame, clear the corrupted message. */
                                    messageState = 0;
                                    rxMessage.frameType = 0;
                                    packetError = true;
                                }
/* For any received errored Tx Status frame, we are unsure about its validity,
so treat it as if the delivery was made */
                                else if (rxMessage.frameType == 0x8B)
                                {
                                    txStatusReceived = true;
                                    txDelivered = true;
                                }
                                break;
                            }
                        }
/* Check for a timeout waiting for a status frame. If timeout, continue on by
assuming delivered and let higher processes determine if the message was
really delivered. */
                        if ((! txStatusReceived) && (timeResponse > TX_STATUS_DELAY))
                        {
                            txDelivered = true;
                            txStatusReceived = true;
                            break;
                        }
/* Check for a timeout waiting for a base station response. */
                        if (timeResponse > RESPONSE_DELAY)
                        {
                            timeout = true;
                            break;
                        }
                    }

/* The transmitted data message was (supposedly) delivered. */
                    if (txDelivered)
                    {
/* A base station response to the data transmission arrived. */
                        if (packetReady)
                        {
/* Respond to an XBee Receive packet. */
                            if (inMessage.frameType == 0x90)
                            {
/* The first character in the data field is an ACK or NAK. */
                                uint8_t rxCommand = inMessage.message.rxRequest.data[0];
/* Base station picked up an error and sent a NAK. Retry three times then give
up the entire cycle. */
                                if (rxCommand == 'N')
                                {
                                    if (++retry >= 3) cycleComplete = true;
                                    txCommand = 'N';
                                    transmit = true;
                                }
/* Got an ACK: aaaah that feels good. */
                                else if (rxCommand == 'A')
                                {
/* We can now subtract the transmitted count from the current counter value
and go back to sleep. This will take us to the next outer loop so set lastCount
to cause it to drop out immediately if the counts had not changed. */
                                    counter -= lastCount;
                                    lastCount = 0;
                                    cycleComplete = true;
                                }
                            }
                        }
/* Errors found in received packet. Retry three times then give up the entire
cycle. */
                        else if (packetError)
                        {
                            if (++retry >= 3) cycleComplete = true;
                            txCommand = 'E';
                            transmit = true;
                        }
                    }
/* If the message was signalled as definitely not delivered, repeat up to three
times then give up the entire cycle. */
                    else if (txStatusReceived)
                    {
                        if (++retry >= 3) cycleComplete = true;
                        transmit = true;
                    }
/* If timeout, repeat, or for the initial transmission, send back a timeout
notification */
                    else if (timeout)
                    {
                        if (++retry >= 3) cycleComplete = true;
                        if (txCommand == 'C') txCommand = 'T';
                        transmit = true;
                    }

/* If a command packet arrived outside the data transmission protocol then
this will catch it (i.e. independently of the Tx Status response). */
                    if (packetReady)
                    {
                        if (inMessage.frameType == 0x90)
                        {
/* The first character in the data field is a command. */
                            uint8_t rxCommand = inMessage.message.rxRequest.data[0];
/* Interpret a 'Parameter Change' command. */
                            if (rxCommand == 'P')
                            {
                            }
/* Keep XBee awake until further notice for possible reconfiguration. */
                            else if (rxCommand == 'W')
                            {
                                stayAwake = true;
                            }
/* Send XBee to sleep. */
                            else if (rxCommand == 'S')
                            {
                                stayAwake = false;
                            }
                        }
                    }
                }
/* If the repeats were exceeded, notify the base station of the abandonment of
this communication attempt. */
                if (retry >= 3) sendMessage("X");
/* Otherwise notify acceptance */
                else sendMessage("A");
            }

            _delay_ms(1);
        }
        while (counter != lastCount);
    }
}

/****************************************************************************/
/** @brief Check for incoming messages and respond.

An incoming message is assembled over multiple calls to this function. A status
is returned indicating completion or error status of the message.

The message is built up as serial data is received, and therefore must not be
changed outside the function until the function returns COMPLETE.

@param[out] rxFrameType *rxMessage: Message received.
@param[out] uint8_t *messageState: Message build state, must be set to zero on
                                   the first call.
@returns uint8_t message completion/error state. Zero means character received
                                   OK but not yet finished.
*/
uint8_t receiveMessage(rxFrameType *rxMessage, uint8_t *messageState)
{
/* Wait for data to appear */
    uint16_t inputChar = getchn();
    uint8_t messageError = high(inputChar);
    if (messageError != NO_DATA)
    {
        uint8_t state = *messageState;
/* Pull in the received character and look for message start */
/* Read in the length (16 bits) and frametype then the rest to a buffer */
        uint8_t inputValue = low(inputChar);
        switch(state)
        {
/* Sync character */
            case 0:
                if (inputChar == 0x7E) state++;
                break;
/* Two byte length */
            case 1:
                rxMessage->length = (inputChar << 8);
                state++;
                break;
            case 2:
                rxMessage->length += inputValue;
                state++;
                break;
/* Frame type */
            case 3:
                rxMessage->frameType = inputValue;
                rxMessage->checksum = inputValue;
                state++;
                break;
/* Rest of message, maybe include addresses or just data */
            default:
                if (state > rxMessage->length + 3)
                    messageError = STATE_MACHINE;
                else if (rxMessage->length + 3 > state)
                {
                    rxMessage->message.array[state-4] = inputValue;
                    state++;
                    rxMessage->checksum += inputValue;
                }
                else
                {
                    state = 0;
                    if (((rxMessage->checksum + inputValue + 1) & 0xFF) > 0)
                        messageError = CHECKSUM;
                    else messageError = COMPLETE;
                }
        }
        *messageState = state;
    }
    return messageError;
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

@param[in] uint8_t timeout: a register setting, 9 or less (see datasheet).
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

/****************************************************************************/
/** @brief Build and transmit a basic frame

Send preamble, then message block, followed by computed checksum.

@param[in]  txFrameType txMessage
*/
void sendBaseFrame(const txFrameType txMessage)
{
    sendch(0x7E);
    sendch(high(txMessage.length));
    sendch(low(txMessage.length));
    sendch(txMessage.frameType);
    uint8_t checksum = txMessage.frameType;
    for (uint8_t i=0; i < txMessage.length-1; i++)
    {
        uint8_t txData = txMessage.message.array[i];
        sendch(txData);
        checksum += txData;
    }
    sendch(0xFF-checksum);
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
void sendTxRequestFrame(const uint8_t sourceAddress64[], const uint8_t sourceAddress16[],
                        const uint8_t radius, const uint8_t dataLength,
                        const uint8_t data[])
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
/** @brief Convert a 32 bit value to ASCII hex form and send with a command.

Also compute an 8-bit modular sum checksum from the data and convert to hex
for transmission. This is sent at the beginning of the string.

@param[in] int8_t command: ASCII command character to prepend to message.
@param[in] int32_t datum: integer value to be sent.
*/

void sendDataCommand(const uint8_t command, const uint32_t datum)
{
    uint8_t buffer[12];
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
/** @brief Wake the XBee and send a string message

If XBee is not awake, wake it and wait for a bit. Send a string message.
The Sleep_Rq pin is active low.

@param[in]  uint8_t* data: pointer to a string of data (ending in 0).
*/
void sendMessage(const uint8_t* data)
{
    if ((inb(SLEEP_RQ_PORT) & SLEEP_RQ_PIN) > 0)
    {
        cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Wakeup XBee */
        _delay_ms(PIN_WAKE_PERIOD);
    }
    sendTxRequestFrame(coordinatorAddress64, coordinatorAddress16,0,strlen(data),data);
}

/****************************************************************************/
/** @brief Sleep the XBee

*/
inline void sleepXBee(void)
{
    sbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Request XBee Sleep */
}

/****************************************************************************/
/** @brief Interrupt on Count Signal.

Determine if a change in the count signal level has occurred. A downward
change will require a count to be registered. An upward change is ignored.

Sample twice to ensure that this isn't a false alarm.
*/
ISR(PCINT1_vect)
{
    uint8_t countSignal = (inb(COUNT_PORT) & _BV(COUNT_PIN));
    if (countSignal == 0)
    {
        countSignal = (inb(COUNT_PORT) & _BV(COUNT_PIN));
        if (countSignal == 0) counter++;
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

