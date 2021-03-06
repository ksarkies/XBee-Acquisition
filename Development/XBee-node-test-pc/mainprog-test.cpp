/*      Main program of code to be tested

This provides a test emulated firmware that received data messages from the
coordinator and echoes back.

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
 *   Copyright (C) 2016 by Ken Sarkies (www.jiggerjuice.info)               *
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

/*---------------------------------------------------------------------------*/
/* clib library functions to be bypassed */

static void wdt_disable() {}
static void wdt_reset() {}

/*---------------------------------------------------------------------------*/
/**** Test code starts here */

#include "../../libs/serial.h"
#include "../../libs/xbee.h"

#define RTC_SCALE   30

/** Convenience macros */
#define TRUE 1
#define FALSE 0
#define  _BV(bit) (1 << (bit))
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

/** @name Variables for transmitted essage*/
/*@{*/
static volatile uint8_t checkSum;     /**< Checksum on sent message contents */
/*@}*/

/* This needs to be defined as global as the main loop is separated out */
static uint8_t messageState;

static uint32_t timeValue;
/* These only need to be global because the program is split in two */
static uint8_t coordinatorAddress64[8];
static uint8_t coordinatorAddress16[2];

/* Local Prototypes */

static void inline hardwareInit(void);
static void inline timer0Init(uint8_t mode,uint16_t timerClock);

void displayMessage(rxFrameType rxMessage)
{
    qDebug() << "Length" << rxMessage.length;
    qDebug() << "Checksum" << rxMessage.checksum;
    qDebug() << "Type" << rxMessage.frameType;
    qDebug() << "AT response received" << (char)rxMessage.message.atResponse.atCommand1 << \
                                         (char)rxMessage.message.atResponse.atCommand2;
    qDebug() << "Status" << rxMessage.message.atResponse.status;
    if (rxMessage.length > 5)
        qDebug() << "AT response data received" << rxMessage.message.atResponse.data;
}

/*****************************/
/* The initialization part is that which is run before the main loop. */
void mainprogInit()
{
    timeCount = 0;
    counter = 0;
    wdt_disable();                  /* Stop watchdog timer */
    hardwareInit();                 /* Initialize the processor specific hardware */
    uartInit();
    timer0Init(0,RTC_SCALE);        /* Configure the timer */
    timeValue = 0;                  /* reset timer */
    uint8_t messageState;           /**< Progress in message reception */

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
    uint8_t n = 0;

    while (! associated)
    {
        sendATFrame(2,"AI");
        qDebug() << "Sent AI request" << n++;
        sleep(1);

/* The frame type we are handling is 0x88 AT Command Response */
        uint16_t timeout = 0;
        messageState = 0;
        uint8_t messageError = XBEE_INCOMPLETE;
        while (messageError == XBEE_INCOMPLETE)
        {
            messageError = receiveMessage(&rxMessage, &messageState);
            if (timeout++ > 60000) break;
        }
        if (messageError != XBEE_COMPLETE)
        {
            qDebug() << "Message Error" << messageError;
        }
        else
        {   
            displayMessage(rxMessage);
        }
        associated = ((messageError == XBEE_COMPLETE) && \
                     (rxMessage.message.atResponse.data[0] == 0) && \
                     (rxMessage.frameType == AT_COMMAND_RESPONSE) && \
                     (rxMessage.message.atResponse.atCommand1 == 65) && \
                     (rxMessage.message.atResponse.atCommand2 == 73));
    }
    qDebug() << "Associated";
#ifdef TEST_PORT_DIR
    cbi(TEST_PORT,TEST_PIN);            /* Set pin off to indicate success */
#endif
}

/*****************************/
/** The Main Program is that part which runs inside an infinite loop.
The loop is emulated in the emulator program. */

void mainprog()
{
/* Main loop */
    wdt_reset();

/* Check for incoming messages */
/* The Rx message variable is re-used and must be processed before the next
arrives */
    messageState = 0;
    rxFrameType rxMessage;
    uint8_t messageError = receiveMessage(&rxMessage, &messageState);
/* The frame types we are handling are 0x90 Rx packet and 0x8B Tx status */
    if (messageError == XBEE_COMPLETE)
    {
        qDebug() << "Message Received" << rxMessage.message.rxPacket.data[0];
        if (rxMessage.frameType == RX_REQUEST)
        {
/* Toggle test port */
#ifdef TEST_PORT_DIR
            if (rxMessage.message.rxPacket.data[0] == 'L') cbi(TEST_PORT,TEST_PIN);
            if (rxMessage.message.rxPacket.data[0] == 'O') sbi(TEST_PORT,TEST_PIN);
#endif
/* Echo */
            sendTxRequestFrame(rxMessage.message.rxPacket.sourceAddress64,
                               rxMessage.message.rxPacket.sourceAddress16,
                               0, rxMessage.length-12,
                               rxMessage.message.rxPacket.data);
        }
    }
    else if (messageError != XBEE_INCOMPLETE)
    {
        displayMessage(rxMessage);
        qDebug() << "Message Error" << messageError;
    }
}

/****************************************************************************/
/** @brief Timer 0 ISR.

This ISR sends a count data record to the coordinator and toggles PC4
where there should be an LED.
*/

//ISR(TIMER0_OVF_vect)
void timerISR()
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
        counter = 0;                /* Reset counter value */
        qDebug() << "Sent Transmission";
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
/* Timer initialization

The timer is initialised with the number of millisecond ticks before firing,
at which time the ISR is called. The initialization is provided in the emulator
code using the timerInit function.

timerClock is the number of firings per second.
*/

extern void timerInit(unsigned int timerTrigger);

void timer0Init(uint8_t mode,uint16_t timerClock)
{
    timerInit(1000/timerClock);
}

/****************************************************************************/
/* Additional nulled hardware routines not needed for the test */

void hardwareInit(void) {}

/*---------------------------------------------------------------------------*/

