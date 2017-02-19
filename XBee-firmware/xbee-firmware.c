/**
@mainpage AVR XBee Node Firmware
@version 0.0
@author Ken Sarkies (www.jiggerjuice.info)
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

An error correction protocol is used to ensure that count data is delivered
correctly to the base station before new counts are started. Refer to the
detailed documentation for more information.

@note
CTS must be set in the XBee and USE_HARDWARE_FLOW also enabled.
@note
Fuses: Disable the "WDT Always On" fuse and disable the BOD fuse.
@note
Software: AVR-GCC 4.8.2
@note
Target:   AVR with sufficient output ports and a USART (USI not supported)
@note
Tested:   ATTiny4313 with 1MHz internal clock. ATMega48 with 8MHz clock,
          ATTiny841 with 8MHz clock.

*/
/****************************************************************************
 *   Copyright (C) 2014 by Ken Sarkies (www.jiggerjuice.info)               *
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

#include <string.h>
#include <avr/sfr_defs.h>
#include <avr/wdt.h>
#include <avr/sleep.h>

#include "../libs/defines.h"
#include "../libs/serial.h"
#include "../libs/xbee.h"
#include <util/delay.h>
#include "xbee-firmware.h"

/****************************************************************************/
/* Global variables */
uint8_t coordinatorAddress64[8];
uint8_t coordinatorAddress16[2];
uint8_t rxOptions;

static uint32_t counter;            /* Event Counter */
static uint8_t wdtCounter;          /* Data Transmission Timer */
static bool transmitMessage;        /* Permission to send a message */
static bool stayAwake;              /* Keep XBee awake until further notice */

/****************************************************************************/
/* Local Prototypes */

static void interpretCommand(rxFrameType* inMessage);
static packet_error readIncomingMessages(bool* packetReady, bool* txStatusReceived,
                                    bool* txDelivered, rxFrameType* inMessage);
static void hardwareInit(void);
static void wdtInit(const uint8_t waketime, bool wdeSet);
static void sendDataCommand(const uint8_t command, const uint32_t datum);
static void sendMessage(const char* data);
static void resetXBee(void);
static void sleepXBee(void);
static void wakeXBee(void);
static void powerDown(void);
static void powerUp(void);

/****************************************************************************/
/** @brief      Main Program

An interrupt service routine records a transition on the counter input. This
wakes the AVR which is sent back to sleep when the counts have settled.

The watchdog timer interrupt service routine provides a time tick of 8 seconds
(maximum). This wakes the AVR which counts off a number of ticks to extend the
sleep interval. Then the data transmission process begins within a loop.
The XBee should return a delivery status frame and the base system should
return a data frame indicating successful delivery or error.

An inner loop checks the XBee for serial data and selects out the base station
transmission and the delivery (Tx Status) frame.
- If a valid frame is received and it is a base station response, packetReady
  is set to signal the next stage of the protocol.
- If an invalid frame is received but is verified as a data frame, packetError
  is set.
- If a valid frame is received and it is a Tx Status is received the txDelivered
  is set from the delivery status field and txReceived is set.
- If the frame is invalid but is verified as a Tx Status, the process continues
  as if it were valid and delivered. the protocol is left to sort out the mess.
- If a timeout occurs and a Tx Status frame was not delivered, again continues
  as if it were valid and delivered.
- If a second timeout occurs after this, timeout is set.

Each of these sets of booleans are evaluated. In the event of errors, the
original message is repeated up to 3 times. Then the communication is abandoned.
- If txDelivered and packetReady, then we have a valid response. If it has the
  N command then the base station got an error. Repeat with N response.
- If it has the A command then all is well. Clear the count and sleep.
- If not packetReady and packetError was set, repeat with E response.
- If not txDelivered but txReceived then it is known certainly that a non
  delivery occurred, and the message is repeated. To avoid duplicates this is
  the only time that the original message is repeated.
- If timeout was set then nothing came back from the XBee or base station and
  the original message is repeated with T response.
- Finally packetReady is checked again regardless of any Tx Status frames, as
  an autonomous message may have come from the base station. This is interpreted
  according to a list of predetemined base station commands.

At the end, if the base station had responded positively and the protocol ended
successfully, a Y response is sent back without any further checks. If all
retries have passed without a positive response, an X response is returned.
*/

int main(void)
{

/*  Initialise hardware */
    hardwareInit();
/** Initialize the UART library, pass the baudrate and avr cpu clock 
(uses the macro UART_BAUD_SELECT()). Set the baudrate to a predefined value. */
    uartInit();
    wdtInit(WDT_TIME, true);            /* Set up watchdog timer */

/* Set the coordinator addresses. All zero 64 bit address with "unknown" 16 bit
address avoids knowing the actual address, but may cause an address discovery
event. */
    for  (uint8_t i=0; i < 8; i++) coordinatorAddress64[i] = 0x00;
    coordinatorAddress16[0] = 0xFE;
    coordinatorAddress16[1] = 0xFF;

/* Check for association indication from the XBee.
Don't start until it is associated. */
    while (! checkAssociated());

/*---------------------------------------------------------------------------*/
/* Main loop forever. */


/* Initialise watchdog timer count */
    wdtCounter = 0;
/* Initialise process counter */
    counter = 0;
    stayAwake = XBEE_STAY_AWAKE;
    transmitMessage = false;

/* Turn off until some counts start to arrive */
    if (!stayAwake) sleepXBee();

    for(;;)
    {
/* The WDIE bit must be set each time an interrupt occurs in case the WDT
reset-after-interrupt was enabled. This will prevent a reset from occurring
unless the MCU has lost its way. */
        sbi(WDTCSR,WDIE);
        sei();
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
#ifdef TEST_PORT_DIR
                sbi(TEST_PORT,TEST_PIN);            /* Set pin on */
#endif
                wdtCounter = 0;     /* Reset the WDT counter for next time */
/* Now ready to initiate a data transmission. */
/* Power up only essential peripherals for transmission of results. */
                powerUp();
                wakeXBee();

/* First check for association indication from the XBee in case this was lost.
Don't proceed until it is associated. If this times out, everything sleeps
until the next cycle. */
                if (checkAssociated())
                {

/* Read the battery voltage from the XBee. */
#ifdef VBATCON_PIN
                    sbi(VBATCON_PORT,VBATCON_PIN);  /* Turn on battery measurement */
#endif
                    uint8_t data[12];
                    int8_t dataLength = readXBeeIO(data);
                    uint16_t batteryVoltage = 0;
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
                        if (transmit)
                        {
                            sendDataCommand(txCommand,
                                lastCount+((uint32_t)batteryVoltage<<16)+((uint32_t)retry<<30));
                            txDelivered = false;    /* Allow check for Tx Status frame */
                            txStatusReceived = false;
                        }
                        transmit = false;   /* Prevent any more transmissions until told */

/* Wait for incoming messages. */
                        rxFrameType inMessage;      /* Buffered data frame */
                        bool packetReady = false;
                        packet_error packetError =
                            readIncomingMessages(&packetReady, &txStatusReceived,
                                                 &txDelivered, &inMessage);
/* Drop out if there are too many errors */
                        if (packetError != no_error) errorCount++;
                        if (errorCount > 10) break;

/* ============ Interpret messages and decide on actions to take */

/* The transmitted data message was (supposedly) delivered. */
                        if (txDelivered)
                        {
/* Respond to an XBee Data packet. This could be an ACK/NAK response part of
the overall protocol, indicating the final status of the protocol, or a
higher level command. */
                            if (packetReady)
                            {
/* Check if the first character in the data field is an ACK or NAK. */
                                uint8_t rxCommand = inMessage.message.rxPacket.data[0];
/* Base station picked up an error in the previous response and sent a NAK.
Retry three times with an N command then give up the entire cycle with an X
command. */
                                if (rxCommand == 'N')
                                {
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
                            txCommand = 'S';
                            transmit = true;
                        }
/* If timeout, repeat, or for the initial transmission, send back a timeout
notification */
                        else if (packetError == timeout)
                        {
                            if (++retry >= 3) cycleComplete = true;
                            txCommand = 'T';
                            transmit = true;
                        }

/* ============ Command Packets */

/* If a command packet arrived outside the data transmission protocol then
this will catch it (i.e. independently of the Base station status response).
This is intended for application commands. */
                        if (packetReady) interpretCommand(&inMessage);
                    }
/* Notify acceptance. No response is expected. */
                    if (retry < 3) sendMessage("A");
/* Otherwise if the repeats were exceeded, notify the base station of the
abandonment of this communication attempt. No response is expected.
Reset the XBee in case this caused the problem. */
                    else
                    {
                        sendMessage("X");
                        resetXBee();            /* Reset the XBee */
                    }

#ifdef TEST_PORT_DIR
                    cbi(TEST_PORT,TEST_PIN);    /* Set pin off */
#endif
                }
            }

            _delay_ms(1);
        }
        while (counter != lastCount);

/* Power down the AVR to deep sleep until an interrupt occurs */
        sei();
        if (! stayAwake)
        {
            sleepXBee();
            powerDown();            /* Turn off all peripherals for sleep */
#ifdef VBATCON_PIN
            cbi(VBATCON_PORT,VBATCON_PIN);   /* Turn off battery measurement */
#endif
/* Power down the AVR to deep sleep until an interrupt occurs */
            set_sleep_mode(SLEEP_MODE_PWR_DOWN);
            sleep_mode();
        }

    }
}

/****************************************************************************/
/** @brief Interpret a Command Message from the Base Station.

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

packet_error readIncomingMessages(bool* packetReady, bool* txStatusReceived,
                             bool* txDelivered, rxFrameType* inMessage)
{
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
/* 0x90 is a Zigbee Receive Packet frame that will contain command and data
from the base station system. Copy to a buffer for later processing. */
                if (rxMessage.frameType == 0x90)
                {
                    inMessage->length = rxMessage.length;
                    inMessage->checksum = rxMessage.checksum;
                    inMessage->frameType = rxMessage.frameType;
                    for (uint8_t i=0; i<RF_PAYLOAD; i++)
                        inMessage->message.rxPacket.data[i] =
                            rxMessage.message.rxPacket.data[i];
                    *packetReady = true;
                }
/* 0x8B is a Zigbee Transmit Status frame. Check if it is telling us the
transmitted message was delivered. Action to repeat will happen ONLY if
txDelivered is false and txStatusReceived is true. */
                else if (rxMessage.frameType == 0x8B)
                {
                    *txDelivered = (rxMessage.message.txStatus.deliveryStatus == 0);
                    *txStatusReceived = true;
                }
/* Unknown packet type. Discard as error and continue. */
                else
                {
                    packetError = unknown_type;
                    *txDelivered = false;
                    *txStatusReceived = false;
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
                    *txStatusReceived = true;
                else
                {
/* With all other errors in the frame, discard everything and repeat. */
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
for transmission. This is sent at the beginning of the string. Allowance is made
for 9 characters: one for the command and 8 for the 32 bit integer.

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
/* Set input ports to pullups and disable digital input buffers on AIN inputs.
Refer to the defines files for the defined symbols. */

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

/* Set I/O ports to desired directions and initial settings */

/* XBee Sleep Request ouput pin */
#ifdef SLEEP_RQ_PIN
    sbi(SLEEP_RQ_PORT_DIR,SLEEP_RQ_PIN);        /* XBee Sleep Request */
    cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);            /* Set to keep XBee on */
#endif
/* XBee On/Sleep Status input pin */
#ifdef ON_SLEEP_PIN
    cbi(ON_SLEEP_PORT_DIR,ON_SLEEP_PIN);
#endif
/* XBee reset output. Pulse low to reset. */
#ifdef XBEE_RESET_PIN
    sbi(XBEE_RESET_PORT_DIR,XBEE_RESET_PIN);    /* XBee Reset output pin */
    sbi(XBEE_RESET_PORT,XBEE_RESET_PIN);        /* Set to keep XBee on */
#endif
/* Battery Measurement Enable output pin */
#ifdef VBATCON_PIN
    sbi(VBATCON_PORT_DIR,VBATCON_PIN);          /* Battery Measure Enable */
    cbi(VBATCON_PORT,VBATCON_PIN);              /* Turn off to power down */
#endif
/* Battery Measurement Input */
#ifdef VBAT_PIN
    cbi(VBAT_PORT_DIR,VBAT_PIN);
    cbi(VBAT_PORT,VBAT_PIN);
#endif
/* Counter input */
#ifdef COUNT_PIN
    cbi(COUNT_PORT_DIR,COUNT_PIN);
    sbi(COUNT_PORT,COUNT_PIN);                  /* Set to pullup */
#endif
/* Test port to flash LED for microcontroller status */
#ifdef TEST_PIN
    sbi(TEST_PORT_DIR,TEST_PIN);
    cbi(TEST_PORT,TEST_PIN);                    /* Set pin on to start */
#endif

/* Counter: Use PCINT for the asynchronous pin change interrupt on the
count signal line. */
    sbi(PC_MSK,PC_INT);                         /* Mask */
    sbi(PC_IER,PC_IE);                          /* Enable */

    powerDown();                        /* Turns off all peripherals */
    powerUp();                          /* Turns on essential peripherals only */
}

/****************************************************************************/
/** @brief Power down all peripherals for Sleep

*/

void powerDown(void)
{
#ifdef ADC_ONR
    cbi(ADC_ONR,AD_EN); /* Disable the ADC first to ensure power down works */
#endif
    outb(PRR,0xFF);     /* power down all controllable peripherals */
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
}

/****************************************************************************/
/** @brief Power up the hardware

Only essential peripherals are turned on, namely the UART and the A/D converter.
*/

void powerUp(void)
{
#ifdef PRR_USART0
    cbi(PRR,PRR_USART0);/* power up USART0 */
#endif
#ifdef ADC_ONR
    sbi(ADC_ONR,AD_EN); /* Enable the ADC */
#endif
}

/****************************************************************************/
/** @brief Initialize the watchdog timer to interrupt on maximum delay

The watchdog timer is set to interrupt rather than reset so that it can wakeup
the AVR. Keeping WDTON disabled (default) will allow the settings to be
changed without restriction. The watchdog timer is initially disabled after
reset.

The timeout settings give the same time interval for each AVR, regardless of the
clock frequency used by the WDT.

The interrupt enable mode is set. If the WDE bit is zero the WDT remains in
interrupt mode. Otherwise a reset will occur following the next interrupt if the
WDT is not reset.

Note that the changing of these modes must follow a strict protocol as outlined
in the datasheet.

IMPORTANT: Disable the "WDT Always On" fuse.

@param[in] uint8_t waketime: a register setting, 9 or less (see datasheet).
@param[in] bool wdeSet: set the WDE bit to enable reset and interrupt to occur
*/

void wdtInit(const uint8_t waketime, bool wdeSet)
{
    uint8_t timeout = waketime;
    if (timeout > 9) timeout = 9;
    uint8_t wdtcsrSetting = (timeout & 0x07);
    if (timeout > 7) wdtcsrSetting |= _BV(WDP3);
    wdtcsrSetting |= _BV(WDIE);             /* Set WDT interrupt enable */
    if (wdeSet) wdtcsrSetting |= _BV(WDE);  /* Set reset-after-interrupt */
    outb(MCUSR,0);              /* Clear the WDRF flag to allow WDE to reset */
#ifdef WDCE
/* This is the required change sequence to clear WDE and set enable and scale
as required for ATMega48 and most other devices. */
    outb(WDTCSR,_BV(WDCE) | _BV(WDE));  /* Set change enable */
#else
/* For later devices, notably ATTiny441 series, the CPU CCP register needs to be
written with a change enable key if the MCU is set to safety level 2 (WDT
always on). */
    outb(CCP,0xD8);
#endif
    outb(WDTCSR,wdtcsrSetting); /* Set scaling factor and enable WDT interrupt */
    sei();
}

/****************************************************************************/
/** @brief Reset the XBee

This sets the xbee-reset pin on the MCU low then high again to force a hardware
reset.
*/

inline void resetXBee(void)
{
#ifdef XBEE_RESET_PIN
    cbi(XBEE_RESET_PORT,XBEE_RESET_PIN);    /* Activate XBee Reset */
    _delay_us(200);
    sbi(XBEE_RESET_PORT,XBEE_RESET_PIN);    /* Release XBee Reset */
    _delay_us(100);
#endif
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
    sbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Request XBee Sleep */
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
    cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Request or set XBee Wake */
    while (inbit(ON_SLEEP_PORT,ON_SLEEP_PIN) == 0); /* Wait for wakeup */
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
    uint8_t countSignal = inbit(COUNT_PORT,COUNT_PIN);
    if (countSignal > 0)
    {
        _delay_us(100);
        countSignal = inbit(COUNT_PORT,COUNT_PIN);
        if (countSignal > 0) counter++;
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
    if (wdtCounter > ACTION_COUNT)
    {
        transmitMessage = true;
        wdtCounter = 0;
    }
}

