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

@note
CTS must be set in the XBee.

*/
/****************************************************************************
 *   Copyright (C) 2016 Ken Sarkies (www.jiggerjuice.info)               *
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
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define RTC_SCALE   30

/*---------------------------------------------------------------------------*/
/* clib library functions to be bypassed */

/*---------------------------------------------------------------------------*/
/**** Test code starts here */

#include "xbee-node-firmware.h"

/****************************************************************************/
/* Global Variables */

uint8_t coordinatorAddress64[8];
uint8_t coordinatorAddress16[2];
uint8_t rxOptions;

static uint32_t counter;            /* Event Counter */
static uint8_t wdtCounter;          /* Data Transmission Timer */
static bool transmitMessage;        /* Permission to send a message */
static bool stayAwake;              /* Keep XBee awake until further notice */

/** @name UART variables */
/*@{*/
static volatile uint16_t uartInput;   /**< Character and errorcode read from uart */
static volatile uint8_t lastError;    /**< Error code for transmission back */
static volatile uint8_t checkSum;     /**< Checksum on message contents */
/*@}*/

/****************************************************************************/
/* Local Prototypes */

static void interpretCommand(rxFrameType* inMessage);
static packet_error readIncomingMessage(bool* packetReady, bool* txStatusReceived,
                                    bool* txDelivered, rxFrameType* inMessage);

/*---------------------------------------------------------------------------*/
/* The initialization part is that which is run before the main loop. */

void mainprogInit()
{
    hardwareInit();                 /* Initialize the processor specific hardware */
    uartInit();
    wdtInit(WDT_TIME, true);            /* Set up watchdog timer */

/* Set the coordinator addresses. All zero 64 bit address with "unknown" 16 bit
address avoids knowing the actual address, but may cause an address discovery
event. */
    uint8_t i;
    for (i=0; i < 8; i++) coordinatorAddress64[i] = 0x00;
    coordinatorAddress16[0] = 0xFE;
    coordinatorAddress16[1] = 0xFF;

/* Initialise watchdog timer count */
    wdtCounter = 0;
/* Initialise process counter */
    counter = 0;
    stayAwake = false;              /* Keep asleep to start */
    transmitMessage = false;
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
            if (transmitMessage)
            {
                transmitMessage = false;
                wdtCounter = 0;     /* Reset the WDT counter for next time */
/* Now ready to initiate a data transmission. */
                powerUp();
                wakeXBee();
/* First check for association indication from the XBee.
Don't proceed until it is associated. */
                if (checkAssociated())
                {

/* Read the battery voltage from the XBee. */
#ifdef VBATCON_PIN
                    sbi(VBATCON_PORT_DIR,VBATCON_PIN);  /* Turn on battery measurement */
#endif
                    uint8_t data[12];
                    int8_t dataLength = readXBeeIO(data);
                    uint32_t batteryVoltage = 0;
                    if (dataLength > 0) batteryVoltage = getXBeeADC(data,1);

/* Initiate a data transmission to the base station. This also serves as a
means of notifying the base station that the AVR is awake. */
                    bool txDelivered = false;
                    bool txStatusReceived = false;
                    bool cycleComplete = false;
                    uint8_t txCommand = 'C';
                    bool transmit = true;
                    uint8_t errorCount = 0;
                    uint8_t retry = 0;      /* Retries to get base response OK */
                    while (! cycleComplete)
                    {
if (cycleComplete) printf("Cycle Completed\n");
else printf("New Cycle\n");
                        if (transmit)
                        {
printf("Transmit Command %c , Last Count %d\n", txCommand, lastCount);
                            sendDataCommand(txCommand,lastCount+(batteryVoltage << 16));
                            txDelivered = false;    /* Allow check for Tx Status frame */
                            txStatusReceived = false;
                        }
                        transmit = false;   /* Prevent any more transmissions until told */

/* Wait for incoming messages. */
                        rxFrameType inMessage;      /* Buffered data frame */
                        bool packetReady = false;
                        packet_error packetError =
                            readIncomingMessage(&packetReady, &txStatusReceived,
                                                 &txDelivered, &inMessage);
/* Break out of cycle if there are too many errors */
                        if (packetError != no_error)
                        {
printf("Receive error %d\n", packetError);
                            errorCount++;
                        }
                        if (errorCount > 10)
                        {
printf("Too many receive errors\n");
                            break;
                        }

/* ============ Interpret messages and decide on actions to take */

/* The transmitted data message was (supposedly) delivered. */
                        if (txDelivered)
                        {
printf("Delivered, start processing\n");
if (!packetReady) printf("Normal Zigbee Transmit Status Frame from local XBee\n");
/* Respond to an XBee Data packet. This could be an ACK/NAK response part of
the overall protocol, indicating the final status of the protocol, or a
higher level command. */
                            if (packetReady)
                            {
printf("Process Packet ready to check for ACK/NAK\n");
/* Check if the first character in the data field is an ACK or NAK. */
                                uint8_t rxCommand = inMessage.message.rxPacket.data[0];
/* Base station picked up an error in the previous response and sent a NAK.
Retry three times with an N command then give up the entire cycle with an X
command. */
                                if (rxCommand == 'N')
                                {
printf("NAK received.\n");
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
to zero to cause it to drop out immediately if the counts had not changed. */
printf("ACK received.\n");
                                    counter -= lastCount;
                                    lastCount = 0;
                                    cycleComplete = true;
                                    packetReady = false;
                                }
/* If not an ACK/NAK, process below as an application data/command frame */
                            }
/* Errors found in received packet. Retry three times then give up the entire
cycle. Send an E data packet to signal to the base station. */
                            else if (packetError != no_error)
                            {
                                if (++retry >= 3) cycleComplete = true;
printf("Process Packet Error %d, Try number %d\n", packetError, retry);
                                txCommand = 'E';
                                transmit = true;
                            }
                        }
/* If the message was signalled as definitely not delivered (or was errored),
that is, txStatusReceived but not txDelivered, repeat up to three times then
give up the entire cycle. Send an S packet to signal to the base station which
should avoid any unlikely duplication (even though previous transmissions were
not received). */
                        else if (txStatusReceived)
                        {
                            if (++retry >= 3) cycleComplete = true;
printf("Process Not Delivered: Try number %d\n", retry);
                            txCommand = 'S';
                            transmit = true;
                        }
/* If timeout, repeat, or for the initial transmission, send back a timeout
notification */
                        else if (packetError == timeout)
                        {
                            if (++retry >= 3) cycleComplete = true;
printf("Process Timeout: Try number %d\n", retry);
                            txCommand = 'T';
                            transmit = true;
                        }

/* ============ Command Packets */

/* If a command packet arrived outside the data transmission protocol then
this will catch it (i.e. independently of the Tx Status response).
This is intended for application commands. */
                        if (packetReady) interpretCommand(&inMessage);
                    }
printf("Finished Cycle\n");
if (retry >= 3) printf("Abandoned, X sent.\n"); else printf("Accepted, A sent.\n");
printf("-------------------------\n");
/* Notify acceptance. No response is expected. */
                    if (retry < 3) sendMessage("A");
/* Otherwise if the repeats were exceeded, notify the base station of the
abandonment of this communication attempt. No response is expected. */
                    else sendMessage("X");
                }
else printf("Not associated.\n");
            }
//            _delay_ms(1);
        }
        while (counter != lastCount);
//    }
}

/****************************************************************************/
/** @brief Interpret a Command Message from the Coordinator.

Deal with commands, parameters and data intended for the attached processor
system. Anything not recognised is ignored.

Globals: all changeable parameters: stayAwake.

@param[in] rxFrameType* inMessage: The received frame with the message.
*/

void interpretCommand(rxFrameType* inMessage)
{
/* The first character in the data field is a command. Do not use A or N as
commands as they will be confused with late ACK/NAK messages. */
    uint8_t rxCommand = inMessage->message.rxPacket.data[0];
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

/****************************************************************************/
/** @brief Read a Received Data Message from the XBee.

Deal with incoming message assembly for Tx Status and base station response
or command reception. A message is expected as a result of a previous
transmission, therefore this loops until a message is received, an error or a
timeout occurs.

@param[out] bool* packetReady
@param[out] bool* txStatusReceived
@param[out] bool* txDelivered
@param[out] rxFrameType* inMessage: The received frame.
@returns bool: error status.
*/

packet_error readIncomingMessage(bool* packetReady, bool* txStatusReceived,
                             bool* txDelivered, rxFrameType* inMessage)
{
    uint8_t i;
    rxFrameType rxMessage;              /* Received frame */
    uint32_t timeResponse = 0;
    packet_error packetError = no_error;
    uint8_t messageState = 0;
/* Loop until the message is received or an error occurs. */
    while (true)
    {

/* Read in part of an incoming frame. */
        uint8_t messageStatus = receiveMessage(&rxMessage, &messageState);
        if (messageStatus != XBEE_INCOMPLETE)
        {
            timeResponse = 0;               /* reset timeout counter */
/* Got a frame complete without error. */
            if (messageStatus == XBEE_COMPLETE)
            {
printf("Message Type %x, Length %d\n", rxMessage.frameType,rxMessage.length);
/* 0x90 is a Zigbee Receive Packet frame that will contain command and data
from the coordinator system. Copy to a buffer for later processing. */
                if (rxMessage.frameType == 0x90)
                {
                    inMessage->length = rxMessage.length;
                    inMessage->checksum = rxMessage.checksum;
                    inMessage->frameType = rxMessage.frameType;
                    for (i=0; i<RF_PAYLOAD; i++)
                        inMessage->message.rxPacket.data[i] =
                            rxMessage.message.rxPacket.data[i];
                    *packetReady = true;
printf("Got a data frame: reply %c", rxMessage.message.rxPacket.data[0]);
printf(", data contents");
for (i=1; i<rxMessage.length; i++) printf(" %x", rxMessage.message.rxPacket.data[i]);
printf("\n");
                }
/* 0x8B is a Zigbee Transmit Status frame. Check if it is telling us the
transmitted message was delivered. Action to repeat will happen ONLY if
txDelivered is false and txStatusReceived is true. */
                else if (rxMessage.frameType == 0x8B)
                {
                    *txDelivered = (rxMessage.message.txStatus.deliveryStatus == 0);
                    *txStatusReceived = true;
printf("Zigbee Transmit Status Frame: data contents");
for (i=0; i<rxMessage.length-1; i++) printf(" %x", rxMessage.message.array[i]);
printf("\n");
printf("Delivery Status? %d", rxMessage.message.txStatus.deliveryStatus);
if (! *txDelivered) printf(" not");
printf(" delivered\n");
                }
/* Unknown packet type. Discard as error and continue. */
                else
                {
                    packetError = unknown_type;
                    *txDelivered = false;
                    *txStatusReceived = false;
printf("Packet error detected: Type %x, Length %d\n", rxMessage.frameType, rxMessage.length);
printf("Unknown Frame: data contents");
for (i=0; i<rxMessage.length-1; i++) printf(" %x", rxMessage.message.array[i]);
printf("\n");
                }
                messageState = 0;   /* Reset packet counter */
                break;
            }
/* If message status is anything else, then this means other errors occurred
(namely checksum or packet length is wrong). */
            else
            {
/* For any received errored Tx Status frame, we are unsure about its validity,
so treat it as if the delivery did not occur. This may result in data being
duplicated if the original message was actually received correctly. */
                *txDelivered = false;
                if (rxMessage.frameType == 0x8B)
                {
                    *txStatusReceived = true;
printf("Faulty status frame - treat as errored!\n");
                }
                else
                {
/* With all other errors in the frame, discard everything and repeat. */
printf("Other error detected %x\n", rxMessage.frameType);
                    rxMessage.frameType = 0;
                    packetError = unknown_error;
                    *txStatusReceived = false;
                }
                messageState = 0;   /* Reset packet counter */
                break;
            }
        }
/* Nothing received, check for timeout waiting for a base station response.
Otherwise continue waiting. */
        else
        {
            if (timeResponse++ > RESPONSE_DELAY)
            {
printf("Timeout waiting for base station: ");
if (! *txDelivered) printf("not ");
printf("delivered ");
if (! *txStatusReceived) printf("not ");
printf("received\n");
                timeResponse = 0;
                packetError = timeout;
                *txDelivered = false;
                *txStatusReceived = false;
                break;
            }
        }
    }
    return packetError;
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
}

/****************************************************************************/
/** @brief Power down all peripherals for Sleep

*/
void powerDown(void)
{
}

/****************************************************************************/
/** @brief Initialize the hardware for process measurement

Set unused ports to inputs and disable power to all unused peripherals.
Set the process counter interrupt to INT0.
*/
void powerUp(void)
{
}

/****************************************************************************/
/** @brief Initialize the watchdog timer to interrupt on maximum delay

The timer is initialised with the number of millisecond ticks before firing,
at which time the ISR is called. The initialization is provided in the emulator
code using the timerInit function.

The watchdog timer is set to interrupt rather than reset so that it can wakeup
the AVR. Keeping WDTON disabled (default) will allow the settings to be
changed without restriction. The watchdog timer is initially disabled after
reset.

The timeout settings give the same time interval for each AVR, regardless of the
clock frequency used by the WDT.

The interrupt enable mode is set. If the WDE bit is zero the WDT remains in
interrupt mode. Otherwise a reset will occur following the next interrupt if the
WDT is not reset.

Note that the changing of these modes must follow
a strict protocol as outlined in the datasheet.

IMPORTANT: Disable the "WDT Always On" fuse.

@param[in] uint8_t waketime: a register setting, 9 or less (see datasheet).
@param[in] bool wdeSet: set the WDE bit to enable reset and interrupt to occur
*/

extern void timerInit(unsigned int timerTrigger);

void wdtInit(const uint8_t waketime, bool wdeSet)
{
    timerInit(1000/RTC_SCALE);
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

/****************************************************************************/
/** @brief Interrupt on Watchdog Timer.

Increment the counter to signal state of WDT.
*/
#if (MCU_TYPE==4313)
//ISR(WDT_OVERFLOW_vect)
#else
//ISR(WDT_vect)
#endif
void timerISR()
{
    wdtCounter++;
    if (wdtCounter == 0)
    {
        transmitMessage = true;
        wdtCounter = 0;
    }
}

