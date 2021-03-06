/*      Main program of code to be tested
                Firmware

@mainpage AVR XBee Node Firmware
@version 2.1
@author Ken Sarkies (www.jiggerjuice.info)
@date 18 March 2016
@date 29 March 2017

This provides an emulated firmware with full protocol implementation for the
watermeter system. It matches the firmware provided in XBee-firmware.

This is version two of the firmware. The rewrite is intended to expose the
logic flow more clearly and attempt to avoid subtle errors.

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
#include <time.h>

#define RTC_SCALE   30

extern FILE *fp;

/*---------------------------------------------------------------------------*/
/* Debug print prototypes */

#include "xbee-firmware.h"

void printData(uint8_t item, uint16_t data);
void dumpPacket(rxFrameType* inMessage);

/*---------------------------------------------------------------------------*/
/**** Test code starts here */

#include "xbee-firmware.h"

/****************************************************************************/
/* Global Variables */

uint8_t coordinatorAddress64[8];
uint8_t coordinatorAddress16[2];
uint8_t rxOptions;

static uint32_t counter;            /* Event Counter */
static uint8_t wdtCounter;          /* Data Transmission Timer */
static bool transmitMessage;        /* Permission to send a message */
static bool stayAwake;              /* Keep XBee awake until further notice */
static uint16_t wakeInterval;       /* number of ticks between wakeups */
static uint8_t wdtTick;             /* timer tick setting (see manual) */

/** @name UART variables */
/*@{*/
static volatile uint16_t uartInput;   /**< Character and errorcode read from uart */
static volatile uint8_t lastError;    /**< Error code for transmission back */
static volatile uint8_t checkSum;     /**< Checksum on message contents */
/*@}*/

/****************************************************************************/
/* Local Prototypes */

static packet_error interpretMessage(uint16_t timeoutDelay, bool wait, rxFrameType* inMessage);
static void interpretCommand(rxFrameType* inMessage);
static packet_error getIncomingMessage(uint16_t timeoutDelay, bool wait, rxFrameType* inMessage);
static uint16_t stringToHex(const uint8_t length, const uint8_t* string);
static void hexToString(const uint32_t value, char* string, const uint8_t length);

/*---------------------------------------------------------------------------*/
/* The initialization part is that which is run before the main loop. */

void mainprogInit()
{
/* Initialise process counter */
    counter = 0;
printData(0,0);
    hardwareInit();             /* Initialize the processor specific hardware */
/** Initialize the UART library, pass the baudrate and avr cpu clock
(uses the macro UART_BAUD_SELECT()). Set the baudrate to a predefined value. */
    uartInit();
    wdtInit(WDT_TIME, true);    /* Set up watchdog timer */

/* Set the coordinator addresses. All zero 64 bit address with "unknown" 16 bit
address avoids knowing the actual address, but may cause an address discovery
event. */
    uint8_t i=0;
    for (i=0; i < 8; i++) coordinatorAddress64[i] = 0x00;
    coordinatorAddress16[0] = 0xFE;
    coordinatorAddress16[1] = 0xFF;

    resetXBee();
    wakeXBee();
/* Startup delay 12 seconds to give time to associate. */
    
    for (i=0; i < 12; i++)
    {
        printf("Delay: ");
        if (checkAssociated()) printf("Associated\n");
        sleep(1);
    }

/* Initialise watchdog timer count */
    wdtCounter = 0;
    stayAwake = false;              /* Keep asleep to start */
/* When the WDT activates, it sets transmitMessage to activate the
transmission cycle. */
    transmitMessage = false;
}

/*****************************/
/** The Main Program is that part which runs inside an infinite loop.
The loop is emulated in the emulator program. */

int mainprog()
{

/* Main loop. Remove outer loop as it is simulated. */

//    for(;;)
//    {

//        if (! stayAwake) sleepXBee();

/* On waking, note the count, wait a bit, and check if it has advanced. If
not, return to sleep. Otherwise keep awake until the counts have settled.
This will avoid rapid wake/sleep cycles when counts are changing.
Counter is a global and is changed in the ISR. */
        uint32_t lastCount;
        uint8_t sleepDelay = 0;
RES:    do
        {
            lastCount = counter;
            if (sleepDelay > 10) sleepDelay = 0;

/* Check for and deal with any extra messages that have arrived while the XBee
was awake or between transmissions. */
            if (! transmitMessage)
            {
                rxFrameType inMessage;              /* Received data frame */
                interpretMessage(500, false, &inMessage);
            }
/* Any interrupt will wake the AVR. If it is a WDT timer overflow event,
8 seconds will be too short to do anything useful, so go back to sleep again
until enough such events have occurred. The WDT ISR will set transmitMessage
when the conditions are satisfied. */
            else
            {
                transmitMessage = false;
                wdtCounter = 0;     /* Reset the WDT counter for next time */
/* Now ready to initiate a data transmission. */
                powerUp();
                wakeXBee();

/* A cycle passes through a number of stages depending on error conditions
until the transmission has been completed or abandoned. */
                uint8_t retryCount = 0;
                bool retryEnable = true;            /* Allows a retry to occur */
                bool associated = false;
                bool batteryCheckOK = false;        /* Got back valid response to IS command */
                bool ack = false;                   /* received ACK from coordinator */
                bool nak = false;                   /* received NAK from coordinator */
                bool delivery = false;              /* Signalled as not delivered */
                uint16_t timeoutDelay = 0;
                uint32_t batteryVoltage = 0;
                bool cycleComplete = false;
                rxFrameType inMessage;              /* Received data frame */
                txStage stage = associationCheck;
                packet_error packetError = unknown_error;
printData(1,0);
                while (! cycleComplete)
                {
/* Wait if not associated. After forty tries (12 seconds), sleep for a while. */
                    if (stage == associationCheck)
                    {
                        if (associated || (retryCount > 40))
                        {
                            retryCount = 0;
                            stage = batteryCheck;
printData(2,0);
                        }
                        else
                        {
                            timeoutDelay = 300;
                            if (retryEnable)
                            {
                                sendATFrame(2,"AI");
                                retryCount++;
                            }
printData(3,retryCount);
                        }
                        retryEnable = true;
                    }
/* Set up to read the battery voltage via the XBee. If not successful, just
give up and go on. */
                    if (stage == batteryCheck)
                    {
                        if (batteryCheckOK || (retryCount > 3))
                        {
/* Turn off battery measurement */
//                            #ifdef VBATCON_PIN
//                            cbi(VBATCON_PORT,VBATCON_PIN);
//                            #endif
                            retryCount = 0;
                            packetError = no_error;
                            nak = false;
                            ack = false;
                            stage = transmit;
printData(4,0);
                        }
                        else
                        {
                            timeoutDelay = 200;
                            if (retryEnable)
                            {
//                                #ifdef VBATCON_PIN
/* Turn on battery measurement */
//                                sbi(VBATCON_PORT_DIR,VBATCON_PIN);
//                                #endif
                                sendATFrame(2,"IS");    /* Force Sample Read */
                                retryCount++;
                            }
printData(5,retryCount);
                        retryEnable = true;
                        }
                    }
/* Transmit the message. An ACK from the coordinator is required. */
                    if (stage == transmit)
                    {
/* An ACK means that we can now complete the cycle gracefully. */
                        if (ack)
                        {
                            sendMessage("A");
/* We can now subtract the transmitted count from the current counter value. */
                            counter -= lastCount;
                            lastCount = 0;
                            retryCount = 0;
                            cycleComplete = true;
printData(6,0);
                        }
/* Too many retries means that we can now complete the cycle ungracefully. */
                        else if (retryCount > 3)
                        {
printData(7,0);
                            sendMessage("X");
                            usleep(100000);
                            mainprogInit();
                            goto RES;           /* Soft reset the Node */
                        }
/* Otherwise decide how to respond with a retransmission. */
                        else
                        {
                            timeoutDelay = 2000;
                            if (retryEnable)
                            {
                                uint32_t parameter = retryCount;
                                uint8_t txCommand = 'C';
                                if (packetError == timeout) txCommand = 'T';
/* Last read of XBee gave an error */
                                else if (packetError != no_error)
                                {
                                    parameter = packetError;
                                    txCommand = 'E';
                                }
                                if (nak) txCommand = 'N';
/* Last attempt was a failed delivery */
                                if (delivery > 0)
                                {
                                    parameter = delivery;
                                    txCommand = 'S';
                                }
/* Data field has count 16 bits, voltage 10 bits, status 6 bits */
                                sendDataCommand(txCommand, 0,
                                    lastCount+((batteryVoltage & 0x3FF)<<16)+
                                    ((parameter & 0x3F)<<26));
                                retryCount++;
printData(8,txCommand);
                            }
                            retryEnable = true;
                            nak = false;
                        }
                    }

/* Wait for incoming messages. */
printData(10,timeoutDelay);
                    packetError = interpretMessage(timeoutDelay, true, &inMessage);
printData(9,packetError);
                    if (packetError == no_error)
                    {
dumpPacket(&inMessage);
/* Interpret messages and gather information returned */
                        switch (inMessage.frameType)
                        {
                        case AT_COMMAND_RESPONSE:
                            if (inMessage.message.atResponse.status != 0)
                                packetError = frame_error;
                            else
                            {
/* Association Indication */
                                if ((inMessage.message.atResponse.atCommand1 == 'A') && \
                                    (inMessage.message.atResponse.atCommand2 == 'I'))
                                {
                                    if (stage != associationCheck) retryEnable = false;
                                    associated = (inMessage.message.atResponse.data[0] == 0);
                                }
/* Battery Voltage Measurement */
                                if ((inMessage.message.atResponse.atCommand1 == 'I') && \
                                    (inMessage.message.atResponse.atCommand2 == 'S'))
                                {
                                    if (stage != batteryCheck) retryEnable = false;
                                    batteryCheckOK = true;
                                    if (inMessage.length > 5)
                                        batteryVoltage = 
                                            getXBeeADC(inMessage.message.atResponse.data,1);
printData(16,batteryVoltage);
                                }
                            }
                            break;
/* Status of previous transmission attempt. */
                        case TX_STATUS:
                            delivery = inMessage.message.txStatus.deliveryStatus;
                            retryEnable = false;
                            break;
/* Receive Packet. This can be of a variety of types but only A and N are
accepted here and only during the transmit stage. */
                        case RX_PACKET:
                            {
                                uint8_t rxCommand = inMessage.message.rxPacket.data[0];
                                if (stage == transmit)
                                {
/* Check if the first character in the data field is an ACK or NAK.
These messages are ONLY valid in the transmit stage and could confuse the
protocol in other stages. */
/* Base station picked up an error in the previous response and sent a NAK. */
                                    if (rxCommand == 'N') nak = true;
/* Got an ACK: aaaah that feels good. */
                                    else if (rxCommand == 'A') ack = true;
/* Otherwise report an error in the command field. */
                                    else packetError = command_error;
                                }
                                else  retryEnable = false;
                            }
                            break;
                        }
                    }
else printData(9,packetError);
                }
/* Cycle Complete */
            }
/* Delay here to prevent sleep mode from occurring until counts have settled.
Count period is typically less than 10ms. */
//            _delay_ms(1);
        }
        while (/*(sleepDelay++ < 10) || */ (counter != lastCount));

//        #ifdef TEST_PORT_DIR
//        cbi(TEST_PORT,TEST_PIN);    /* Set test pin off */
//        #endif
/* Power down the AVR to deep sleep until an interrupt occurs */
//        sei();
//        if (! stayAwake) sleepXBee();
//        set_sleep_mode(SLEEP_MODE_PWR_DOWN);
//        sleep_enable();
//        sleep_cpu();

//    }
    return true;
}

/****************************************************************************/
/** @brief Interpret a Received Message.

Checks for a message received and interprets it to deal with certain cases
needing action independently of the transmit cycle, or ignoring.

@param[in] rxFrameType* inMessage: The received frame with the message.
@param[in] bool wait: if true, wait for a character to arrive, otherwise return.
@returns packet_error: no_error, timeout, checksum_error, frame_error,
                       unknown_error, no_character.
*/

packet_error interpretMessage(uint16_t timeoutDelay, bool wait, rxFrameType* inMessage)
{
    packet_error packetError = getIncomingMessage(timeoutDelay, wait, inMessage);
    if (packetError == no_error)
    {
        switch (inMessage->frameType)
        {
/* Irrelevant message types that can be ignored. */
        case MODEM_STATUS:
            packetError = modem_status;
            break;
        case NODE_IDENT:
            packetError = node_ident;
            break;
        case IO_DATA_SAMPLE:
            packetError = io_data_sample;
            break;
/* Receive Packet. This can be of a variety of types.
We deal only with the commands to the node. */
        case RX_PACKET:
            {
/* Got a special command plus possible parameters from the coordinator for
action. */
                uint8_t rxCommand = inMessage->message.rxPacket.data[0];
                if (rxCommand == 'D')
                {
                    interpretCommand(inMessage);
                    packetError = no_error;
                }
            }
        }
    }
    return packetError;
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
    char response[13];
/* The first character in the data field is a command. Do not use A or N as
commands as they will be confused with late ACK/NAK messages. */
    uint8_t nodeCommand = inMessage->message.rxPacket.data[1];
printData(11,nodeCommand);
    uint8_t parameter = inMessage->message.rxPacket.data[2];
printData(11,parameter);
/* Interpret a 'Parameter Read/Change' command. */
    if (nodeCommand == 'P')
    {
/* Wakeup time interval. Send back value and/or change.  */
        if (parameter == 'W')
        {
            uint16_t newWakeInterval = stringToHex(inMessage->length-15,
                                            inMessage->message.rxPacket.data+3);
            if (newWakeInterval > 0)
            {
                wakeInterval = newWakeInterval;
            }
            sendDataCommand(nodeCommand, parameter, wakeInterval);
        }
    }
/* Keep XBee awake until further notice for possible reconfiguration. */
    else if (nodeCommand == 'A')
    {
        if (parameter == 'W')
        {
            stayAwake = true;
        }
/* Send XBee to sleep. */
        else if (parameter == 'S')
        {
            stayAwake = false;
        }
/* Return a response to indicate reception */
        response[0] = 'A';
        response[1] = 0;
        sendMessage(response);
    }
}

/****************************************************************************/
/** @brief Pull in a Received Data Message from the XBee.

Deal with incoming message assembly. A message is expected as a result of a
previous transmission, therefore this loops until a message is received, an
error or a timeout occurs.

@param[in] uint16_t timeoutDelay. Millisecond delay allowed for a message to come.
@param[in] bool wait: if true, wait for a character to arrive, otherwise return.
@param[out] rxFrameType* inMessage: The received frame.
@returns packet_error: no_error, timeout, checksum_error, frame_error,
                       unknown_error, no_character.
*/

packet_error getIncomingMessage(uint16_t timeoutDelay, bool wait, rxFrameType* inMessage)
{
    uint16_t timeResponse = 0;
    packet_error packetError = no_error;
    uint8_t messageState = 0;
/* Wait for first character */
    uint8_t messageStatus = receiveMessage(inMessage, &messageState);
    if (messageStatus == XBEE_NO_CHARACTER)
    {
        if (! wait) return no_character;
    }
/* Loop until the rest of the message is received or an error occurs. */
    while (true)
    {

/* Read in part of an incoming frame. */
        uint8_t messageStatus = receiveMessage(inMessage, &messageState);
        if (messageStatus == XBEE_NO_CHARACTER) messageStatus = XBEE_INCOMPLETE;
        if (messageStatus != XBEE_INCOMPLETE)
        {
/* If message status is not a frame complete without error, then this means
other errors occurred (namely checksum or packet length is wrong). */
            if (messageStatus != XBEE_COMPLETE)
            {
                if (messageStatus == XBEE_CHECKSUM) packetError = checksum_error;
                else if (messageStatus == XBEE_STATE_MACHINE) packetError = frame_error;
                else packetError = unknown_error;
            }
            break;                  /* Drop out of the loop */
        }
/* Nothing received or message still in progress, check for timeout waiting for
the base station response. Otherwise continue waiting. */
        else
        {
//            _delay_ms(1);
            usleep(1000);           /* one ms delay to provide precise timing */
            if (timeResponse++ > timeoutDelay)
            {
                packetError = timeout;
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

void sendDataCommand(const uint8_t command, const uint8_t parameter, const uint32_t datum)
{
    char buffer[13];
    buffer[0] = command;
    buffer[1] = parameter;
    uint8_t offset = 1;
    if (parameter == 0) offset = 0;
    hexToString(datum, buffer+offset, 10);
    buffer[11+offset] = 0;             /* String terminator */
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
/** @brief Reset the XBee

This sets the xbee-reset pin on the MCU low then high again to force a hardware
reset.
*/

inline void resetXBee(void)
{
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

/****************************************************************************/
/* Debug print of messages and data to declutter code */

void printData(uint8_t item, uint16_t data)
{
    char timeString[20];
    time_t rawtime;
    struct tm * timeinfo;
    rawtime = time(NULL);
    timeinfo = localtime(&rawtime);
    strftime(timeString, sizeof(timeString),"%FT%H:%M:%S",timeinfo);
    switch (item)
    {
    case 0:
        printf("XBee Firmware Emulator version %s\n",VERSION);
        fprintf(fp,"XBee Firmware Emulator version %s\n",VERSION);
    case 1:
        printf("--------------------------\nNEW CYCLE ");
        fprintf(fp,"--------------------------\nNEW CYCLE ");
        printf("%s\n", timeString);
        fprintf(fp,"%s\n", timeString);
        break;
    case 2:
        printf("Associated\n");
        fprintf(fp,"Associated\n");
        break;
    case 3:
        printf("Not Associated, retry: %d\n", data);
        fprintf(fp,"Not Associated, retry: %d\n", data);
        break;
    case 4:
        printf("Battery Voltage Success\n");
        fprintf(fp,"Battery Voltage Success\n");
        break;
    case 5:
        printf("Battery Voltage Fail, retry: %d\n", data);
        fprintf(fp,"Battery Voltage Fail, retry: %d\n", data);
        break;
    case 6:
        printf("ACK sent\n");
        fprintf(fp,"ACK sent\n");
        break;
    case 7:
        printf("NAK sent\n");
        fprintf(fp,"NAK sent\n");
        break;
    case 8:
        printf("Transmit Command %c sent\n", data);
        fprintf(fp,"Transmit Command %c sent\n", data);
        break;
    case 9:
        {
            if (data < 10)
            {
                char const *packetErrorString[] = {"No Error", "Timeout",
                "Unknown Type", "Checksum", "Frame Error", "Modem Status",
                "Node Ident", "I/O Data", "Protocol Error", "Command Error",
                "Unknown Error"};
                printf("Packet Error %s\n", packetErrorString[data]);
                fprintf(fp,"Packet Error %s\n", packetErrorString[data]);
            }
            else
            {
                printf("Packet Error Out of Range\n");
                fprintf(fp,"Packet Error Out of Range\n");
            }
        break;
        }
    case 10:
        printf("Timeout Delay %d time %s\n", data, timeString);
        fprintf(fp,"Timeout Delay %d time %s\n", data, timeString);
        break;
    case 11:
        printf("Received data command %d time %s\n", data, timeString);
        fprintf(fp,"Received data command %d time %s\n", data, timeString);
        break;
    case 12:
        printf("Received message %d time %s\n", data, timeString);
        fprintf(fp,"Received message %d time %s\n", data, timeString);
        break;
    case 13:
        printf("Received character %d time %s\n", data, timeString);
        fprintf(fp,"Received character %d time %s\n", data, timeString);
    case 14:
        printf("Checked Receiver %d time %s\n", data, timeString);
        fprintf(fp,"Checked Receiver %d time %s\n", data, timeString);
        break;
    case 15:
        printf("Received Nothing %d time %s\n", data, timeString);
        fprintf(fp,"Received Nothing %d time %s\n", data, timeString);
        break;
    case 16:
        printf("Battery Voltage %d\n", data);
        fprintf(fp,"Battery Voltage %d\n", data);
        break;
    }
}

/****************************************************************************/
/* Debug print of packet contents to declutter code */

void dumpPacket(rxFrameType* inMessage)
{
    char timeString[20];
    time_t rawtime;
    struct tm * timeinfo;
    rawtime = time(NULL);
    timeinfo = localtime(&rawtime);
    strftime(timeString, sizeof(timeString),"%FT%H:%M:%S",timeinfo);
    uint8_t i = 0;
    bool txDelivered = (inMessage->message.txStatus.deliveryStatus == 0);
    switch (inMessage->frameType)
    {
    case AT_COMMAND_RESPONSE:
        printf("AT Command Response ");
        printf("Status: %d ",inMessage->message.atResponse.status);
        printf("Command: %c%c ",inMessage->message.atResponse.atCommand1,
                inMessage->message.atResponse.atCommand2);
        fprintf(fp,"AT Command Response ");
        fprintf(fp,"Status: %d ",inMessage->message.atResponse.status);
        fprintf(fp,"Command: %c%c ",inMessage->message.atResponse.atCommand1,
                inMessage->message.atResponse.atCommand2);
        for (i=0; i<inMessage->length-5; i++)
        {
            printf(" %X", inMessage->message.atResponse.data[i]);
            fprintf(fp," %X", inMessage->message.atResponse.data[i]);
        }
        break;
    case MODEM_STATUS:
        printf("Modem Status ");
        printf("Status: %d ",inMessage->message.modemStatus.status);
        fprintf(fp,"Modem Status ");
        fprintf(fp,"Status: %d ",inMessage->message.modemStatus.status);
        break;
    case NODE_IDENT:
        printf("Identify Packet: Length %d  data contents", inMessage->length);
        for (i=0; i<inMessage->length-1; i++) printf(" %X", inMessage->message.array[i]);
        fprintf(fp,"Identify Packet: Length %d  data contents", inMessage->length);
        for (i=0; i<inMessage->length-1; i++) fprintf(fp," %X", inMessage->message.array[i]);
        break;
    case IO_DATA_SAMPLE:
        printf("I/O Data Sample: Length %d  data contents", inMessage->length);
        for (i=0; i<inMessage->length-1; i++) printf(" %X", inMessage->message.array[i]);
        fprintf(fp,"I/O Data Sample: Length %d  data contents", inMessage->length);
        for (i=0; i<inMessage->length-1; i++) fprintf(fp," %X", inMessage->message.array[i]);
        break;
/* Status of previous transmission attempt. */
    case TX_STATUS:
        printf("Transmit Status: data contents");
        for (i=0; i<inMessage->length-1; i++) printf(" %x", inMessage->message.array[i]);
        printf(" Delivery Status? %d", txDelivered);
        if (! txDelivered) printf(" not");
        printf(" delivered");
        fprintf(fp,"Transmit Status: data contents");
        for (i=0; i<inMessage->length-1; i++) fprintf(fp," %x", inMessage->message.array[i]);
        fprintf(fp," Delivery Status? %d", txDelivered);
        if (! txDelivered) fprintf(fp," not");
        fprintf(fp," delivered");
        break;
/* Receive Packet. This can be of a variety of types. */
    case RX_PACKET:
        printf("Data Packet: reply %c", inMessage->message.rxPacket.data[0]);
        printf(", data contents");
        for (i=1; i<inMessage->length; i++) printf(" %X", inMessage->message.rxPacket.data[i]);
        fprintf(fp,"Data Packet: reply %c", inMessage->message.rxPacket.data[0]);
        fprintf(fp,", data contents");
        for (i=1; i<inMessage->length; i++) fprintf(fp," %X", inMessage->message.rxPacket.data[i]);
        break;
    default:
        printf("Unknown Frame: Type %X, Length %d ", inMessage->frameType, inMessage->length);
        printf(" data contents");
        for (i=0; i<inMessage->length-1; i++) printf(" %X", inMessage->message.array[i]);
        fprintf(fp,"Unknown Frame: Type %X, Length %d ", inMessage->frameType, inMessage->length);
        fprintf(fp," data contents");
        for (i=0; i<inMessage->length-1; i++) fprintf(fp," %X", inMessage->message.array[i]);
    }
    printf(" time %s\n",timeString);
    fprintf(fp," time %s\n",timeString);
}

/****************************************************************************/
/** @brief Convert Hex ASCII String to Integer.

The string is interpreted as hexadecimal characters until an invalid character,
zero terminator or the end of string is encountered. If an invalid character
is detected or the string length is not positive definite, a zero is returned.

@param[in] uint8_t length: length of string.
@param[in] uint8_t* string.
@returns uint16_t: hex digit.
*/

uint16_t stringToHex(const uint8_t length, const uint8_t* string)
{
    uint16_t count = 0;
    if (length > 0)
    {
        int i=0;
        for (i=0; i<length; i++)
        {
            unsigned int digit=0;
            char hex = string[i];
            if (hex == 0) break;
            if ((hex >= '0') && (hex <= '9'))
                digit = hex - '0';
            else
            {
                if ((hex >= 'A') && (hex <= 'F')) digit = hex + 10 - 'A';
                else
                {
                    count = 0;
                    break;
                }
            }
            count = (count << 4) + digit;
        }
    }
    return count;
}

/****************************************************************************/
/** @brief Convert Integer to Hex ASCII String.

@param[in] uint32_t num.
@param[out] uint8_t* string.
@param[in] uint8_t length: length of string to produce. Longer strings will be
                           truncated.
*/

void hexToString(const uint32_t num, char* string, const uint8_t length)
{
    uint32_t value = num;
    char checksum = -(value + (value >> 8) + (value >> 16) + (value >> 24));
    uint8_t i;
    for (i = 0; i < length; i++)
    {
        if (i == length-2) value = checksum;
        string[length-i] = "0123456789ABCDEF"[value & 0x0F];
        value >>= 4;
    }
}

