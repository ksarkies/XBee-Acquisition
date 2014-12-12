/**
@mainpage AVR XBee Node Firmware using NARTOS
@version 0.0
@author Ken Sarkies (www.jiggerjuice.net)
@date 11 December 2014

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
#include "../../NARTOS/nartos.h"
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
volatile uint8_t coordinatorAddress64[8];
volatile uint8_t coordinatorAddress16[2];
volatile uint8_t rxOptions;

volatile uint32_t counter;           /* External event counter */
volatile uint32_t lastCount;
volatile uint8_t wdtCounter;         /* Timer counter to extend sleep times */

volatile bool txStatusReceived;      /* The Tx Status message has been received */
volatile bool txOK;                  /* Tx Status message received correctly. */

volatile uint8_t retry;              /* Counter for retrying messages */
volatile bool stayAwake;             /* Keep awake in response to command */
volatile baseResponse_t baseResponse;/* Response from the base station received */
volatile bool txFinished;            /* Finish data part of transmission */
volatile uint8_t txCommand;          /* Command to send with data */
volatile uint8_t rxCommand;          /* Command received */
volatile bool rxTerminate;           /* Signal to the receive task to terminate */
volatile rxFrameType inMessage;      /* Data message received */

volatile uint8_t sendBuffer[12];     /* Transmission buffer */
volatile bool timeStatusResponse;    /* Timing of Tx Status response */
volatile uint8_t txStatusRetry;      /* Count of bad Tx status retries. */

volatile bool timeBaseResponse;      /* Timing of base station response */
volatile uint8_t messageState;       /* Progress of frame being received */
volatile rxFrameType rxMessage;      /* Buffered received message */
volatile messageError_t messageError;/* Error code for message received */

/* Task IDs */
uint8_t mainTaskID;
uint8_t receiveTaskID;

/*---------------------------------------------------------------------------*/
/** @brief Main program.

Initialise hardware and global variables, and start scheduler with tasks. */

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

/* Allow XBee to sleep */
    stayAwake = false;

/* Enable Interrupts */
    sei();

/** Initialise the OS stuff, setup the receive task as a first task, and jump
to the OS. */
  initNartos();
  mainTaskID = taskStart((uint16_t)*mainTask,FALSE);
  nartos();
}

/*---------------------------------------------------------------------------*/
/** @brief Main task

Set AVR and XBee to sleep. Wakes when a timer interrupt or a count interrupt
occurs.

Coordinate the data collection, transmission, reception and command
interpretation. */

void mainTask(void)
{
/* Main loop forever. */
    for(;;)
    {
        if (! stayAwake) sleepXBee();
/* Power down the AVR to deep sleep until an interrupt occurs */
        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
        sleep_enable();
        sleep_cpu();

/* On waking, note the count. After processing other work, wait a bit and check
if it has advanced. If not, return to sleep. Otherwise keep awake until the
counts have settled. This will avoid rapid wake/sleep cycles when counts are
changing. The counter is a global variable and is changed in the ISR. */
        do
        {
            lastCount = counter;

/* Any interrupt will wake the AVR. If it is a WDT timer overflow event,
the maximum time of 8 seconds will be too short to do anything useful, so go
back to sleep again until enough such interrupts have occurred. */
            if (wdtCounter > ACTION_COUNT)
            {
/* Now ready to initiate a data transmission. */
                wdtCounter = 0; /* Reset the WDT counter for next time */
                txFinished = false;
                retry = 0;
/* Start the receive task as messages will now start to come in */
                rxTerminate = false;
                receiveTaskID = taskStart((uint16_t)*receiveTask,FALSE);
/* Initiate a data transmission to the base station. This also serves as a
means of notifying the base station that the AVR is awake. */
                txCommand = 'C';
                while (! txFinished)
                {
/* Build string to send. */
                    char checksum = -(lastCount + (lastCount >> 8) +
                                    (lastCount >> 16) + (lastCount >> 24));
                    uint32_t value = lastCount;
                    for (uint8_t i = 0; i < 10; i++)
                    {
                        if (i == 8) value = checksum;
                        sendBuffer[10-i] = "0123456789ABCDEF"[value & 0x0F];
                        value >>= 4;
                    }
                    sendBuffer[11] = 0;             /* String terminator */
                    sendBuffer[0] = txCommand;
/* Send message and test for valid delivery */
                    txStatusRetry = 0;      /* Number of retries on bad status frames */
/* Repeat the message only if we know it was not delivered. Continue on if
Tx Status frame was corrupted or delivery was successful */
                    do
                    {
                        txStatusReceived = false;       /* Wait for a Tx Status frame */
                        txOK = true;                    /* Tx Status indicates delivery success */
                        timeStatusResponse = 0;         /* Timeout count waiting for status */
                        sendMessage(sendBuffer);
                        while ((! txStatusReceived) && (timeStatusResponse++ < TX_STATUS_DELAY))
                        {
                            taskRelinquish();       /* Wait for Tx Status */
                        }
                    }
                    while ((! txOK) && (txStatusRetry++ < 3));

/* Wait for an eventual response from the base station */
                    timeBaseResponse = 0;
                    while ((timeBaseResponse++ < RESPONSE_DELAY) &&
                          (baseResponse == waiting))
                        taskRelinquish();
/* If nothing received within the timeout period, resend up to 3 times.
Send the same command unless it is the initial attempt. In that case send a
timeout notification. */
                    if (baseResponse == waiting)
                    {
                        if (retry++ >= 3) txFinished = true;
                        if (txCommand == 'C') txCommand = 'T';
                    }
                    else
/* Errors found in received packet. */
                    if (baseResponse == packetError)
                    {
                        if (retry++ >= 3) txFinished = true;
                        txCommand = 'E';
                    }
                    else
/* Respond to an XBee Receive packet. */
                    if (inMessage.frameType == 0x90)
                    {
/* The first character in the data field is an ACK, NAK or command. */
                        uint8_t rxCommand = inMessage.message.rxRequest.data[0];
/* Base station picked up an error and sent a NAK. */
                        if (rxCommand == 'N')
                        {
                            if (retry++ >= 3) txFinished = true;
                            txCommand = 'N';
                        }
/* Got an ACK: aaaah that feels good. */
                        else if (rxCommand == 'A')
                        {
/* We can now subtract the transmitted count from the current counter value
and go back to sleep. This will take us to the next outer loop so set lastCount
to cause it to drop out immediately if the counts had not changed. */
                            counter -= lastCount;
                            lastCount = 0;
                            txFinished = true;
                        }
/* Interpret a 'Parameter Change' command. */
                        else if (rxCommand == 'P')
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
/* When ended, if the retrys were exceeded, notify the base station of the
abandonment of this communication attempt. */
                    if (txFinished)
                    {
                        if (retry >= 3) sendMessage("X");
/* Otherwise notify acceptance */
                        else  sendMessage("A");
                    }
                }
                rxTerminate = true;
            }
/* Idle for about 10ms to see if the counts change */
            _delay_ms(10);
        }
        while (counter != lastCount);
    }
}

/****************************************************************************/
/** @brief Check for incoming messages and respond.

An incoming message is read from the XBee. If nothing appears within a given
delay, a timeout is generated, otherwise the message is assembled. This is
checked for errors. If valid:
- A Tx Status message is interpreted for transmission failure.
- A data Rx message is buffered and signalled to the transmission task.
- Other messages are discarded.

The task is created before a round of transmissions are sent to the XBee
and terminates on instruction.
*/
void receiveTask(void)
{
/* Loop until terminated by external program */
    do
    {
/* Build a receive frame */
        baseResponse = waiting;
        messageState = 0;
        messageError = OK;
        txOK = false;
        do
        {
/* Wait for serial character to appear */
            uint8_t errorCode;
            uint16_t inputChar;
            do
            {
                taskRelinquish();
                inputChar = getchn();
                errorCode = high(inputChar);
            }
            while (errorCode == NO_DATA);
            if (errorCode != 0) messageError = usartError;
            else
            {
/* Look for message start */
/* Read in the length (16 bits) and frametype then the rest to a buffer */
                uint8_t inputValue = low(inputChar);
                switch(messageState)
                {
/* Skip until sync character received */
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
/* Frame type. Start building the checksum */
                    case 3:
                        rxMessage.frameType = inputValue;
                        rxMessage.checksum = inputValue;
                        messageState++;
                        break;
/* Rest of message, maybe including addresses, or just data */
                    default:
                        if (messageState > rxMessage.length + 3)
                            messageError = invalidLength;
                        else if (rxMessage.length + 3 > messageState)
                        {
                            rxMessage.message.array[messageState-4] = inputValue;
                            messageState++;
                            rxMessage.checksum += inputValue;
                        }
                        else
                        {
/* Message ended: reset state and compute checksum */
                            messageState = 0;
                            if (((rxMessage.checksum + inputValue + 1) & 0xFF) > 0)
                                messageError = checksum;
                            else messageError = OK;
                        }
                }
/* Message too long for buffer, probably bad message. */
                if (messageState > RF_PAYLOAD+17) messageError = invalidLength;
            }
        }
        while ((messageState != 0) && (messageError == OK));

        if (rxMessage.frameType == 0x90)    /* Received message */
/* Message failure. For the base station response notify a bad received frame. */
/* Copy the message to a buffer for use by the main program */
        {
            if (messageError != OK) baseResponse = packetError;
            else
            {
                inMessage.length = rxMessage.length;
                inMessage.checksum = rxMessage.checksum;
                inMessage.frameType = rxMessage.frameType;
                for (uint8_t i=0; i<messageState-4; i++)
                    inMessage.message.array[i] = rxMessage.message.array[i];
                baseResponse = packetReady;
            }
        }
/* Tx Status message */
        else if (rxMessage.frameType == 0x8B)    /* Tx Status message */
        {
/* Test the delivery status field. If not zero, sent packet was not delivered.
In the event of an errored frame, simply report it as OK to force the program
to continue. */
            if (messageError == OK)
                txOK = (rxMessage.message.rxRequest.data[1] == 0);
            else txOK = true;
            txStatusReceived = true;
        }
/* Other types of failed messages are discarded and timeouts take over */
    }
    while (! rxTerminate);
    taskExit();
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
/** @brief Build and Transmit a Tx Request frame

A data message for the XBee API is formed and transmitted.
The frameID is set to 0x02 to invoke a Tx Status response frame.

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

