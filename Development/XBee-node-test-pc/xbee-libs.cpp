/*  XBee API library routines for the AVR UART interface */

/****************************************************************************
 *   Copyright (C) 2013 by Ken Sarkies (www.jiggerjuice.info)               *
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
#include <stdbool.h>
#include "../../libs/xbee.h"
#include "../../libs/serial.h"

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

/****************************************************************************/
/** @brief Build and transmit a Tx Request frame to a remote unit

A data message for the XBee API is formed and transmitted.

@param[in]:   uint8_t sourceAddress64[]. Address of parent or 0 for coordinator.
@param[in]:   uint8_t sourceAddress16[].
@param[in]:   uint8_t radius. Broadcast radius or 0 for maximum network value.
@param[in]:   uint8_t dataLength. Length of data array.
@param[in]:   uint8_t data[]. Define array size to be greater than length.
*/
void sendTxRequestFrame(const uint8_t sourceAddress64[],
                        const uint8_t sourceAddress16[],
                        const uint8_t radius, const uint8_t dataLength,
                        const uint8_t data[])
{
    uint8_t i;
    txFrameType txMessage;
    txMessage.frameType = TX_REQUEST;
    txMessage.message.txRequest.frameID = 0x02;
    txMessage.length = dataLength+14;
    for (i=0; i < 8; i++)
    {
        txMessage.message.txRequest.sourceAddress64[i] = sourceAddress64[i];
    }
    for (i=0; i < 2; i++)
    {
        txMessage.message.txRequest.sourceAddress16[i] = sourceAddress16[i];
    }
    txMessage.message.txRequest.radius = radius;
    txMessage.message.txRequest.options = 0;
    for (i=0; i < dataLength; i++)
    {
        txMessage.message.txRequest.data[i] = data[i];
    }
    sendBaseFrame(txMessage);
}

/****************************************************************************/
/** @brief Build and transmit an AT Query frame to the local XBee

A local AT frame requesting information is formed and transmitted. The AT
command consists of two ASCII characters followed by binary parameters.

At this stage only a single parameter at most is accepted.

@param[in]:   uint8_t dataLength: the number of elements in the data array.
@param[in]:   uint8_t data[]: the AT command followed by parameters.
*/
void sendATFrame(const uint8_t dataLength, const char data[])
{
    txFrameType atFrame;
    atFrame.frameType = AT_COMMAND;
    atFrame.message.atCommand.frameID = 0x03;
    atFrame.length = 4;
    atFrame.message.atCommand.atCommand1 = data[0];
    atFrame.message.atCommand.atCommand2 = data[1];
    if (dataLength > 2)
    {
        atFrame.message.atCommand.parameter = data[2];
        atFrame.length++;
    }
    sendBaseFrame(atFrame);
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
    uint8_t i;
    for (i=0; i < txMessage.length-1; i++)
    {
        uint8_t txData = txMessage.message.array[i];
        sendch(txData);
        checksum += txData;
    }
    sendch(0xFF-checksum);
}

/****************************************************************************/
/** @brief Check for incoming messages and respond.

The USART input is tested for an incoming receiver frame. If a byte is received
an INCOMPLETE status is returned. When the message has been fully received
a COMPLETE status is returned. All other results are errors.

The message is built up as serial data is received, and therefore must not be
changed outside the function until the function returns COMPLETE.

@param[out] rxFrameType *rxMessage: Message received.
@param[out] uint8_t *messageState: Message build state, must be set to zero on
                                   the first call.
@returns uint8_t message completion/error state.
*/
uint8_t receiveMessage(rxFrameType *rxMessage, uint8_t *messageState)
{
/* Wait for data to appear */
    uint16_t inputChar = getch();
    uint16_t messageError = XBEE_INCOMPLETE;
    if (high(inputChar) != NO_DATA)
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
                    messageError = XBEE_STATE_MACHINE;
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
                        messageError = XBEE_CHECKSUM;
                    else messageError = XBEE_COMPLETE;
                }
        }
        *messageState = state;
    }
    return messageError;
}

/****************************************************************************/
/** @brief Check for association indication from the local XBee.

The XBee is queried for association indication. This loops until association
is verified. If a test port is defined the test port is flashed.

*/

bool checkAssociated(void)
{
    uint8_t messageState = 0;   /* Progress in message reception */
    rxFrameType rxMessage;
    uint8_t count = 0;
    bool associated = false;
    while (! associated)
    {
        sendATFrame(2,"AI");

/* The frame type we are handling is 0x88 AT Command Response */
        uint16_t timeout = 0;
        messageState = 0;
        uint8_t messageError = XBEE_INCOMPLETE;
/* Wait for response. If it doesn't come, try sending again. */
        while (messageError == XBEE_INCOMPLETE)
        {
            messageError = receiveMessage(&rxMessage, &messageState);
            if (timeout++ > 30000) break;
        }
/* If errors occur, or frame is the wrong type, just try again */
        associated = ((messageError == XBEE_COMPLETE) && \
                     (rxMessage.message.atResponse.data[0] == 0) && \
                     (rxMessage.frameType == AT_COMMAND_RESPONSE) && \
                     (rxMessage.message.atResponse.atCommand1 == 65) && \
                     (rxMessage.message.atResponse.atCommand2 == 73));
        if (count++ > 10) break;
    }
    return associated;
}

/****************************************************************************/
/** @brief Read the I/O ports on the local XBee.

Data frame from a force read of the XBee ports.
Byte 0 always 1
Byte 1,2 mask of all enabled of the 12 digital I/O ports.
Byte 3 mask of all enabled of the 4 analogue I/O ports and supply voltage.
Byte 4,5 value of the enabled I/O ports, or missing if the mask is zero.
Following this (byte 6 on or byte 4 on if no digital data is present) is blocks
of two bytes giving the values of the A/D ports.

@param[out] uint8_t* data; data field from the command response.
@returns int8_t length of data field. Returns -1 if read fails after 3 attempts.
*/

int8_t readXBeeIO(uint8_t* data)
{
    int8_t length = -1;
    uint8_t count = 0;
    while (count++ < 3)
    {
        sendATFrame(2,"IS");                /* Force Sample Read */

/* The frame type we are handling is 0x88 AT Command Response */
        uint16_t timeout = 0;
        uint8_t messageState = 0;
        rxFrameType rxMessage;
        uint8_t messageError = XBEE_INCOMPLETE;
        while (messageError == XBEE_INCOMPLETE)
        {
            messageError = receiveMessage(&rxMessage, &messageState);
            if (timeout++ > 30000) break;
        }
        bool ok = ((messageError == XBEE_COMPLETE) && \
              (rxMessage.message.atResponse.status == 0) && \
              (rxMessage.frameType == AT_COMMAND_RESPONSE) && \
              (rxMessage.message.atResponse.atCommand1 == 73) && \
              (rxMessage.message.atResponse.atCommand2 == 83));
        if (ok)
        {
            length = rxMessage.length-5;    /* length of data field */
            uint8_t i = 0;
            for (i = 0; i < length; i++)
                data[i] = rxMessage.message.atResponse.data[i];
            break;
        }
    }
    return length;
}

/****************************************************************************/
/** @brief Access ADC data from the XBee.

The XBee returns a variable length data field
depending on what ports have been enabled.

@param[in] uint8_t* data: data field from the XBee after a readXBeeIO.
@returns uint16_t AD2 value; XBee A/D converter is 10 bits.
*/

uint16_t getXBeeADC(uint8_t* data, uint8_t adcPort)
{
    uint16_t value = 0;
    uint8_t i = 4;
    if ((data[1] > 0) || (data[2] > 0)) i = 6;  /* Skip digital data if present */
    uint8_t port = 0;                           /* Check through ADC data mask */
    for (port = 0; port < 5; port++)
    {
        if (port == adcPort)
        {
            value = (data[i] << 8) + data[i+1];
            break;
        }
        if ((data[3] & _BV(port)) > 0) i += 2;  /* Skip unwanted enabled ADCs */
    }
    return value;
}

/****************************************************************************/

