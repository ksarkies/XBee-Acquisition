/**
@mainpage AVR XBee Node Firmware
@version 2.1
@author Ken Sarkies (www.jiggerjuice.info)
@date 18 March 2016
@date 29 March 2016

@brief Code for an AVR with an XBee in a Remote Low Power Node

This is version two of the firmware. The rewrite is intended to expose the
logic flow more clearly and attempt to avoid subtle errors.

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
Tested:   ATTiny4313 with 1MHz internal clock. ATTiny841 with 8MHz clock.
          ATMega48 series with 8MHz clock,

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
static packet_error getIncomingMessage(uint16_t timeoutDelay, rxFrameType* inMessage);
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

/* Initialise process counter */
    counter = 0;

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

    resetXBee();
    wakeXBee();
/* Idle and blink the test pin for 12 seconds to give XBee time to associate. */
    uint8_t i = 0;
    for (i=0; i<60; i++)
    {
#ifdef TEST_PORT_DIR
        sbi(TEST_PORT,TEST_PIN);
#endif
        _delay_ms(100);
#ifdef TEST_PORT_DIR
        cbi(TEST_PORT,TEST_PIN);
#endif
        _delay_ms(100);
    }
/* Blink debug port to indicated ready */
#ifdef DEBUG_PORT_DIR
    sbi(DEBUG_PORT,DEBUG_PIN);          /* Blink debug LED */
    _delay_ms(100);
    cbi(DEBUG_PORT,DEBUG_PIN);
    _delay_ms(100);
    sbi(DEBUG_PORT,DEBUG_PIN); 
    _delay_ms(100);
    cbi(DEBUG_PORT,DEBUG_PIN);
#endif

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
                sbi(TEST_PORT,TEST_PIN);            /* Set test pin on */
#endif
                wdtCounter = 0;     /* Reset the WDT counter for next time */
/* Now ready to initiate a data transmission. */
/* Power up only essential peripherals for transmission of results. */
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
                while (! cycleComplete)
                {
/* Wait if not associated. After forty tries (12 seconds), sleep for a while. */
                    if (stage == associationCheck)
                    {
                        if (associated || (retryCount > 40))
                        {
                            retryCount = 0;
                            stage = batteryCheck;
                        }
                        else
                        {
                            timeoutDelay = 300;
                            if (retryEnable)
                            {
                                sendATFrame(2,"AI");
                                retryCount++;
                            }
                        }
                        retryEnable = true;
                    }
/* Read the battery voltage from the XBee. If not successful, just give up and go on. */
                    if (stage == batteryCheck)
                    {
                        if (batteryCheckOK || (retryCount > 3))
                        {
                            #ifdef VBATCON_PIN
/* Turn off battery measurement */
                            cbi(VBATCON_PORT,VBATCON_PIN);
                            #endif
                            retryCount = 0;
                            packetError = no_error;
                            nak = false;
                            ack = false;
                            stage = transmit;
                        }
                        else
                        {
                            timeoutDelay = 200;
                            if (retryEnable)
                            {
                                #ifdef VBATCON_PIN
/* Turn on battery measurement */
                                sbi(VBATCON_PORT,VBATCON_PIN);
                                #endif
                                sendATFrame(2,"IS");    /* Force Sample Read */
                                retryCount++;
                            }
                        }
                        retryEnable = true;
                    }
/* Transmit the message. An ACK from the coordinator is required. */
                    if (stage == transmit)
                    {
/* An ACK means that we can now complete the cycle gracefully. */
                        if (ack)
                        {
                            sendMessage("A");
/* Subtract the transmitted count from the current counter value. */
                            counter -= lastCount;
                            lastCount = 0;
                            retryCount = 0;
                            cycleComplete = true;
                        }
/* Too many retries means that we can now complete the cycle ungracefully. */
                        else if (retryCount > 3)
                        {
                            sendMessage("X");
                            _delay_ms(100);     /* Give time for message to go */
                            resetXBee();        /* Reset the XBee */
                            retryCount = 0;
                            cycleComplete = true;
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
/* Data filed has count 16 bits, voltage 10 bits, status 6 bits */
                                sendDataCommand(txCommand,
                                    lastCount+((batteryVoltage & 0x3FF)<<16)+
                                    ((parameter & 0x3F)<<26));
                                retryCount++;
                            }
                            retryEnable = true;
                            nak = false;
                        }
                    }

/* Wait for incoming messages. */
                    packetError = getIncomingMessage(timeoutDelay, &inMessage);
                    if (packetError == no_error)
                    {
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
                                }
                            }
                            break;
/* Irrelevant message types that can be ignored. */
                        case MODEM_STATUS:
                            packetError = modem_status;
                            retryEnable = false;
                            break;
                        case NODE_IDENT:
                            packetError = node_ident;
                            retryEnable = false;
                            break;
                        case IO_DATA_SAMPLE:
                            packetError = io_data_sample;
                            retryEnable = false;
                            break;
/* Status of previous transmission attempt. */
                        case TX_STATUS:
                            delivery = inMessage.message.txStatus.deliveryStatus;
                            retryEnable = false;
                            break;
/* Receive Packet. This can be of a variety of types. */
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
/* Got a special command plus possible parameters from the coordinator for action.
This must be allowed to occur at any stage. */
                                if (rxCommand == 'D')
                                {
                                    interpretCommand(&inMessage);
                                    packetError = no_error;
                                    retryEnable = false;
                                }
                            }
                            break;
                        }
                    }
                }
/* Cycle Complete */
            }
        }
        while (counter != lastCount);

        #ifdef TEST_PORT_DIR
        cbi(TEST_PORT,TEST_PIN);    /* Set test pin off */
        #endif
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
/** @brief Pull in a Received Data Message from the XBee.

Deal with incoming message assembly. A message is expected as a result of a
previous transmission, therefore this loops until a message is received, an
error or a timeout occurs.

@param[in] uint16_t timeoutDelay. Millisecond delay allowed for a message to come.
@param[out] rxFrameType* inMessage: The received frame.
@returns bool: packet_error. no_error, timeout, checksum_error, frame_error, unknown_error.
*/

packet_error getIncomingMessage(uint16_t timeoutDelay, rxFrameType* inMessage)
{
    uint16_t timeResponse = 0;
    packet_error packetError = no_error;
    uint8_t messageState = 0;
/* Loop until the message is received or an error occurs. */
    while (true)
    {

/* Read in part of an incoming frame. */
        uint8_t messageStatus = receiveMessage(inMessage, &messageState);
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
            _delay_ms(1);       /* one ms delay to provide precise timing */
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
/* Set input ports to pullups.
Refer to the defines files for the defined symbols. */

#ifdef PORTA
    outb(DDRA,0);           /* set as inputs */
#endif
#ifdef PORTA_PUP
    outb(PORTA,PORTA_PUP);  /* set pullups   */
#endif
#ifdef PORTB
    outb(DDRB,0);           /* set as inputs */
#endif
#ifdef PORTB_PUP
    outb(PORTB,PORTB_PUP);  /* set pullups   */
#endif
#ifdef PORTC
    outb(DDRC,0);           /* set as inputs */
#endif
#ifdef PORTC_PUP
    outb(PORTC,PORTC_PUP);  /* set pullups   */
#endif
#ifdef PORTD
    outb(DDRD,0);           /* set as inputs */
#endif
#ifdef PORTD_PUP
    outb(PORTD,PORTB_PUP);  /* set pullups   */
#endif

/* Disable digital input buffers on AIN inputs. */
#ifdef DIDR0_SET
    outb(DIDR0,DIDR0_SET);  /* set disable of digital input buffers   */
#endif
#ifdef DIDR1_SET
    outb(DIDR1,DIDR1_SET);  /* set disable of digital input buffers   */
#endif

/* Set I/O ports to desired directions and initial settings */

/* XBee Sleep Request ouput pin */
#ifdef SLEEP_RQ_PIN
    sbi(SLEEP_RQ_PORT_DIR,SLEEP_RQ_PIN);        /* XBee Sleep Request */
    cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);            /* Clear to keep XBee on */
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
    cbi(VBAT_PORT_DIR,VBAT_PIN);                /* Battery Measurement Enable */
    cbi(VBAT_PORT_PUP,VBAT_PIN);                /* Clear to disable pullup */
#endif
/* Counter input */
#ifdef COUNT_PIN
    cbi(COUNT_PORT_DIR,COUNT_PIN);
    sbi(COUNT_PORT_PUP,COUNT_PIN);              /* Set to pullup */
#endif
/* Test port to flash LED for microcontroller status */
#ifdef TEST_PIN
    sbi(TEST_PORT_DIR,TEST_PIN);
    sbi(TEST_PORT,TEST_PIN);                    /* Set pin on to start */
#endif
/* Debug port to flash LED for microcontroller signalling */
#ifdef DEBUG_PIN
    sbi(DEBUG_PORT_DIR,DEBUG_PIN);
    cbi(DEBUG_PORT,DEBUG_PIN);                  /* Set pin off to start */
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

Only essential peripherals are turned on, namely the UART and the A/D converter,
during the wake phase.
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

