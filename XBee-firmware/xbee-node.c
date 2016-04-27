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

An error correction protocol is used to ensure that count data is delivered
correctly to the base station before new counts are started. Refer to the
detailed documentation for more information.

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
#include "../libs/xbee.h"
#include "xbee-node.h"
#include <util/delay.h>

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
#define  inbit(sfr, bit) (_SFR_BYTE(sfr) & _BV(bit))
#define  high(x) ((uint8_t) (x >> 8) & 0xFF)
#define  low(x) ((uint8_t) (x & 0xFF))

/* Global variables */
uint8_t coordinatorAddress64[8];
uint8_t coordinatorAddress16[2];
uint8_t rxOptions;

uint32_t counter;               /* Event Counter */
uint8_t wdtCounter;             /* Data Transmission Timer */

/*---------------------------------------------------------------------------*/
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

    wdtInit(WDT_TIME);              /* Set up watchdog timer */
/*---------------------------------------------------------------------------*/
/* Main loop forever. */
    for(;;)
    {
        sei();
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
/* XBee Data frame. Copy to a buffer for later processing. */
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
/* XBee Status frame. Check if the transmitted message was delivered. Action to
repeat will happen ONLY if txDelivered is false and txStatusReceived is true. */
                                else if (rxMessage.frameType == 0x8B)
                                {
                                    txDelivered = (rxMessage.message.txStatus.deliveryStatus == 0);
                                    txStatusReceived = true;
                                }
/* Unknown packet type. Discard as error and continue. */
                                else
                                {
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
so treat it as if the delivery was faulty */
                                if (rxMessage.frameType == 0x8B)
                                {
                                    txDelivered = false;
                                    txStatusReceived = true;
                                }
                                else
                                {
/* With all other errors in the frame, discard everything and repeat. */
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
                        if (packetReady)
                        {
/* Respond to an XBee Data packet. */
/* The first character in the data field is an ACK or NAK. */
                            uint8_t rxCommand = inMessage.message.rxRequest.data[0];
/* Base station picked up an error and sent a NAK. Retry three times then give
up the entire cycle. */
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
to cause it to drop out immediately if the counts had not changed. */
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
this will catch it (i.e. independently of the Tx Status response).
This is intended for application commands. */
                    if (packetReady)
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

Wake the XBee and send a string message.

@param[in]  uint8_t* data: pointer to a string of data (ending in 0).
*/
void sendMessage(const char* data)
{
    wakeXBee();
    sendTxRequestFrame(coordinatorAddress64, coordinatorAddress16,0,
                       strlen(data),data);
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
remains in interrupt mode. Note that the changing of these modes must follow
a strict protocol as outlined in the datasheet.

IMPORTANT: Disable the "WDT Always On" fuse.

@param[in] uint8_t waketime: a register setting, 9 or less (see datasheet).
*/
void wdtInit(const uint8_t waketime)
{
    uint8_t timeout = waketime;
    if (timeout > 9) timeout = 9;
    uint8_t wdtcsrSetting = (timeout & 0x07);
    if (timeout > 7) wdtcsrSetting |= _BV(WDP3);
    wdtcsrSetting |= _BV(WDIE); /* Set WDT interrupt enable */
    outb(MCUSR,0);              /* Clear the WDRF flag to allow WDE to reset */
#ifdef WDCE
/* This is the required change sequence to clear WDE and set enable and scale
as required for ATMega48 and most other devices. */
    outb(WDTCSR,_BV(WDCE) | _BV(WDE));  /* Set change enable */
#else
/* For later devices, notably ATTiny441 series, the CPU CCP register needs to be
written with a change enable key. Then setting WDIE will also clear WDE. */
    outb(CCP,0xD8);
#endif
    outb(WDTCSR,wdtcsrSetting); /* Set scaling factor and enable WDT interrupt */
    sei();
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

The XBee should wake in a very short time.
*/
inline void wakeXBee(void)
{
/* If the XBee is asleep: */
    if (inbit(ON_SLEEP_PORT,ON_SLEEP_PIN) == 0)
    {
/* Set Sleep_RQ high in case the XBee is in cyclic/pin wake mode. */
        sbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);
        _delay_ms(PIN_WAKE_PERIOD);
    }
    cbi(SLEEP_RQ_PORT,SLEEP_RQ_PIN);    /* Request or set XBee Wake */
    _delay_ms(PIN_WAKE_PERIOD);
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
    sbi(WDTCSR,WDIE);       /* Set interrupt enable again in case changed */
}

