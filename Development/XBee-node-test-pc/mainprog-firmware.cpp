/*      Main program of code to be tested
                Firmware Test

** Omit the sleep sections for obvious reasons. The WDT is replaced by the
** simulated timer section.

Split the code to be tested into an initialization part and an operational
part that normally falls within an infinite loop. Place the initialization part
into the function called mainprogInit(), and the operational part into
mainprog(). Both these are called from the xbee-node-test.cpp emulator, with the
operational part having its loop emulated in the emulator code.

There should be no compilable processor specific statements. These can be
replaced by a call to a local function that should contain code to emulate the
statement. If this is not possible then the code is not suitable for testing in
this way.

Add all global variables. Add also any include files needed. These should not
contain references to any processor specific libraries or functions.

ISRs need to be changed to a function call and means to activate them devised.
A timer is emulated in the emulator code. Timer initialization is done by
calling an initialization function in the emulator code as shown. This function
sets the number of one millisecond tick counts at which the timer fires and
calls the ISR. If a timer ISR is not defined, a null ISR is substituted.

The serial.cpp file can be used to provide local function calls to POSIX (QT)
equivalents.

Other library files need to be copied over and the extension changed from c to
cpp. Update the .pro file to recognise all headers and source files for
compilation.
*/
/****************************************************************************
 *   Copyright (C) 2016 by Ken Sarkies ksarkies@internode.on.net            *
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
#include <unistd.h>
#include <QDebug>
#include <QString>

/*---------------------------------------------------------------------------*/
/* clib library functions to be bypassed */

static void wdt_disable() {}
static void wakeXBee() {}
extern void _timerTick();

/*---------------------------------------------------------------------------*/
/**** Test code starts here */


#include "../../libs/serial.h"
#include "../../libs/xbee.h"
#define F_CPU   1000000
#include "xbee-node-firmware.h"

#define RTC_SCALE   30

/** Convenience macros */
#define TRUE 1
#define FALSE 0
#define _BV(bit) (1 << (bit))
#define high(x) ((uint8_t) (x >> 8) & 0xFF)
#define low(x) ((uint8_t) (x & 0xFF))

/* Global Variables */
/** Real Time Clock Structure

The 32 bit time can also be accessed as four bytes. Time scale is defined in
the information block.
*/
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
static volatile uint16_t uartInput;   /**< Character and errorcode read from uart */
static volatile uint8_t lastError;    /**< Error code for transmission back */
static volatile uint8_t checkSum;     /**< Checksum on message contents */
/*@}*/

static uint32_t timeValue;
static uint8_t messageState;           /**< Progress in message reception */
static uint8_t messageReady;           /**< Indicate that a message is ready */
static uint8_t messageError;
static uint8_t coordinatorAddress64[8];
static uint8_t coordinatorAddress16[2];
uint8_t wdtCounter;             /* Data Transmission Timer */
bool stayAwake;

/* Local Prototypes */

//static void inline hardwareInit(void);
static void inline timer0Init(uint8_t mode,uint16_t timerClock);

/*****************************/
/* The initialization part is that which is run before the main loop. */
void mainprogInit()
{
    timeCount = 0;
    counter = 0;
    wdt_disable();                  /* Stop watchdog timer */
    hardwareInit();                 /* Initialize the processor specific hardware */
    uartInit();
    timer0Init(0,8000);             /* Configure the timer to 8 second ticks */
    timeValue = 0;                  /* reset timer */

/* Set the coordinator addresses. All zero 64 bit address with "unknown" 16 bit
address avoids knowing the actual address, but may cause an address discovery
event. */
    for  (uint8_t i=0; i < 8; i++) coordinatorAddress64[i] = 0x00;
    coordinatorAddress16[0] = 0xFE;
    coordinatorAddress16[1] = 0xFF;
    messageState = 0;
    messageReady = FALSE;
    messageError = 0;

/* Initialise watchdog timer count */
    wdtCounter = 0;
    stayAwake = false;              /* Keep awake in response to command */
}

/*****************************/
/** The Main Program is that part which runs inside an infinite loop.
The loop is emulated in the emulator program. */

void mainprog()
{

/* Main loop. Remove outer loop as it is simulated. */

//    for(;;)
//    {

//        if (! stayAwake) sleepXBee();
/* Power down the AVR to deep sleep until an interrupt occurs */
//        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
//        sleep_enable();
//        sleep_cpu();

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
                wdtCounter = 0;     /* Reset the WDT counter for next time */
/* Initiate a data transmission to the base station. This also serves as a
means of notifying the base station that the AVR is awake. */
                bool txDelivered = false;
                bool txStatusReceived = false;
                bool cycleComplete = false;
                uint8_t txCommand = 'C';
                bool transmit = true;
                uint8_t retry = 0;      /* Retries to get base response OK */
                while (! cycleComplete)
                {
QChar command = txCommand;
qDebug() << "New Cycle: Complete?" << cycleComplete;
if (transmit) qDebug() << "Transmit?" << QString::fromRawData(&command,1)
                       << ", Last Count" << lastCount;
                    if (transmit)
                    {
                        sendDataCommand(txCommand,lastCount);
                        txDelivered = false;    /* Allow check for Tx Status frame */
                        txStatusReceived = false;
                    }
                    transmit = false;   /* Prevent any more transmissions until told */

/* Deal with incoming message assembly for Tx Status and base station response
or command reception. Keep looping until we have a recognised packet or error. */
                    bool timeout = false;
                    rxFrameType rxMessage;      /* Received frame */
                    rxFrameType inMessage;      /* Buffered data frame */
                    uint8_t messageState = 0;
                    uint32_t timeResponse = 0;
                    bool packetError = false;
                    bool packetReady = false;
                    while (true)
                    {

/* Read in part of an incoming frame. */
                        uint8_t messageStatus = receiveMessage(&rxMessage, &messageState);
                        if (messageStatus != NO_DATA)
                        {
                            timeResponse = 0;
/* Got a frame complete without error. */
                            if (messageStatus == COMPLETE)
                            {
qDebug() << "Message" << QString("Type %1").arg(rxMessage.frameType,0,16)
         << ", Length" << rxMessage.length;
/* XBee Data frame. Copy to a buffer for later processing. */
                                if (rxMessage.frameType == 0x90)
                                {
                                    inMessage.length = rxMessage.length;
                                    inMessage.checksum = rxMessage.checksum;
                                    inMessage.frameType = rxMessage.frameType;
                                    for (uint8_t i=0; i<RF_PAYLOAD; i++)
                                        inMessage.message.rxRequest.data[i] =
                                            rxMessage.message.rxRequest.data[i];
QChar reply = rxMessage.message.rxRequest.data[0];
QString msg = QString("Reply: ")+QString::fromRawData(&reply,1)+" ";
for (uint8_t i=1; i<rxMessage.length; i++)
msg.append(QString("%1").arg(rxMessage.message.rxRequest.data[i]));
qDebug() << msg;
                                    packetReady = true;
                                }
/* XBee Status frame. Check if the transmitted message was delivered. Action to
repeat will happen ONLY if txDelivered is false and txStatusReceived is true. */
                                else if (rxMessage.frameType == 0x8B)
                                {
                                    txDelivered = (rxMessage.message.txStatus.deliveryStatus == 0);
                                    txStatusReceived = true;
qDebug() << "Status Frame: Status?" << rxMessage.message.txStatus.deliveryStatus
         << ", Delivered?" << txDelivered;
                                }
/* Unknown packet type. Discard as error and continue. */
                                else
                                {
qDebug() << "Packet error detected";
                                    packetError = true;
                                    txDelivered = false;
                                    txStatusReceived = false;
                                }
                                messageState = 0;   /* Reset packet counter */
                                break;
                            }
/* Zero message status means it is part way through so just continue on. */
/* If nonzero then this means other errors occurred (namely checksum or packet
length is wrong). */
                            else if (messageStatus > 0)
                            {
/* For any received errored Tx Status frame, we are unsure about its validity,
so treat it as if the delivery was made */
                                if (rxMessage.frameType == 0x8B)
                                {
qDebug() << "Faulty status frame - treat as delivered!";
                                    txStatusReceived = true;
                                    txDelivered = true;
                                }
                                else
                                {
/* With all other errors in the frame, clear the corrupted message, discard and
repeat. */
qDebug() << "Other error detected";
                                    rxMessage.frameType = 0;
                                    packetError = true;
                                    txDelivered = false;
                                    txStatusReceived = false;
                                }
                                messageState = 0;   /* Reset packet counter */
                                break;
                            }
                        }
/* Nothing received, check for timeout waiting for a base station response.
Otherwise continue waiting. */
                        else
                            if (timeResponse++ > RESPONSE_DELAY) 
                            {
qDebug() << "Timeout - Try number" << retry << ", Delivered?" << txDelivered
         << ", Received?" << txStatusReceived;
                                timeResponse = 0;
                                timeout = true;
                                txDelivered = false;
                                txStatusReceived = false;
                                break;
                            }
                    }

/* The transmitted data message was (supposedly) delivered. */
                    if (txDelivered)
                    {
/* A base station response to the data transmission arrived. */
qDebug() << "Delivered, start processing";
                        if (packetReady)
                        {
/* Respond to an XBee Data packet. */
/* The first character in the data field is an ACK or NAK. */
qDebug() << "Process Packet ready to check for ACK/NAK";
                            uint8_t rxCommand = inMessage.message.rxRequest.data[0];
/* Base station picked up an error and sent a NAK. Retry three times then give
up the entire cycle. */
                            if (rxCommand == 'N')
                            {
qDebug() << "NAK";
                                if (++retry >= 3) cycleComplete = true;
                                txCommand = 'N';
                                transmit = true;
                                packetReady = false;
                            }
/* Got an ACK: aaaah that feels good. */
                            else if (rxCommand == 'A')
                            {
/* We can now subtract the transmitted count from the current counter value
and go back to sleep. This will take us to the next outer loop so set lastCount
to cause it to drop out immediately if the counts had not changed. */
qDebug() << "ACK";
                                counter -= lastCount;
                                lastCount = 0;
                                cycleComplete = true;
                                packetReady = false;
                            }
/* If not an ACK/NAK, process below as an application data frame */
                        }
/* Errors found in received packet. Retry three times then give up the entire
cycle. */
                        else if (packetError)
                        {
qDebug() << "Process Packet Error";
                            if (++retry >= 3) cycleComplete = true;
                            txCommand = 'E';
                            transmit = true;
                        }
                    }
/* If the message was signalled as definitely not delivered, that is,
txStatusReceived but not txDelivered, repeat up to three times then give up the
entire cycle. */
                    else if (txStatusReceived)
                    {
qDebug() << "Process Not Delivered: Try number" << retry;
                        if (++retry >= 3) cycleComplete = true;
                        transmit = true;
                    }
/* If timeout, repeat, or for the initial transmission, send back a timeout
notification */
                    else if (timeout)
                    {
qDebug() << "Process Timeout: Try number" << retry;
                        if (++retry >= 3) cycleComplete = true;
                        if (txCommand == 'C') txCommand = 'T';
                        transmit = true;
                    }

/* If a command packet arrived outside the data transmission protocol then
this will catch it (i.e. independently of the Tx Status response).
This is intended for application commands. */
                    if (packetReady)
                    {
qDebug() << "Might be an Application packet?";
/* The first character in the data field is a command. Do not use A or N as
commands as they will be confused with late ACK/NAK */
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
/* If the repeats were exceeded, notify the base station of the abandonment of
this communication attempt. */
qDebug() << "Finished Cycle";
qDebug() << "-------------------------";
                if (retry >= 3) sendMessage(1,(uint8_t*)"X");
/* Otherwise notify acceptance */
                else sendMessage(1,(uint8_t*)"A");
            }
//            _delay_ms(1);
        }
        while (counter != lastCount);
//    }
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
    sendMessage(11,buffer);
}

/****************************************************************************/
/** @brief Send a string message

Wake the XBee and send a string message.

@param[in]  uint8_t* data: pointer to a string of data (ending in 0).
*/
void sendMessage(const uint8_t length,const uint8_t* data)
{
    wakeXBee();
    sendTxRequestFrame(coordinatorAddress64, coordinatorAddress16,0,
                       length,data);
}

/****************************************************************************/
/** @brief Timer 0 ISR.

*/

//ISR(TIMER0_OVF_vect)
void timerISR()
{
    realTime.timeValue++;
    timeCount++;
    wdtCounter = timeCount;
}

/****************************************************************************/
/* Timer initialization

The timer is initialised with the number of millisecond ticks before firing,
at which time the ISR is called. The initialization is provided in the emulator
code using the timerInit function.

In this implementation, the parameter timerTrigger is the number of milliseconds
between ticks.
*/

extern void timerInit(unsigned int timerTrigger);

void timer0Init(uint8_t mode,uint16_t timerTrigger)
{
    timerInit(timerTrigger);
}

/****************************************************************************/
/* Additional nulled hardware routines not needed for the test */

void hardwareInit(void) {}

/*---------------------------------------------------------------------------*/

