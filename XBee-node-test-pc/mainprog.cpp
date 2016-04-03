/*      Main program of code to be tested

Place the code to be tested into the function called mainprog() which is called
from the xbee-node-test.cpp wrapper. There should be no processor specific
statements. These can be replaced by a call to a local function that should
contain code to emulate the statement. If this is not possible then the code is
not suitable for testing in this way.

Add all global variables. Add also any include files needed. These should not
contain references to any processor specific libraries or functions.

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
#include "../libs/serial.h"
#include "../libs/xbee.h"

/* Library functions to be bypassed */
static void wdt_disable() {}

/*---------------------------------------------------------------------------*/
/* Dummy define to satisfy code compilation for this example */
#define RTC_SCALE   0

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
} time;

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

/* Local Prototypes */

static void inline hardwareInit(void);
static void inline timer0Init(uint8_t mode,uint16_t timerClock);

/** Main Program */

void mainprog()
{
    timeCount = 0;
    counter = 0;
    wdt_disable();                  /* Stop watchdog timer */
    hardwareInit();                 /* Initialize the processor specific hardware */
    uartInit();
    timer0Init(0,RTC_SCALE);        /* Configure the timer */
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
/* Main loop */
    for(;;)
    {
//            wdt_reset();

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
/* Additional nulled hardware routines not needed for the test */

void hardwareInit(void) {}
void timer0Init(uint8_t mode,uint16_t timerClock) {}

/*---------------------------------------------------------------------------*/

