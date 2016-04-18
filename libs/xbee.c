/*  XBee API routines for the AVR UART */

/****************************************************************************
 *   Copyright (C) 2013 by Ken Sarkies ksarkies@internode.on.net            *
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
#include "xbee.h"
#include "serial.h"

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
/** @brief Build and transmit a Tx Request frame

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

An incoming message is assembled over multiple calls to this function. A status
is returned indicating completion or error status of the message.

The message is built up as serial data is received, and therefore must not be
changed outside the function until the function returns COMPLETE.

@param[out] rxFrameType *rxMessage: Message received.
@param[out] uint8_t *messageState: Message build state, must be set to zero on
                                   the first call.
@returns uint8_t message completion/error state. Zero means character received
                                   OK but not yet finished.
*/
uint8_t receiveMessage(rxFrameType *rxMessage, uint8_t *messageState)
{
/* Wait for data to appear */
    uint16_t inputChar = getch();
    uint8_t messageError = high(inputChar);
    if (messageError != NO_DATA)
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
                    messageError = STATE_MACHINE;
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
                        messageError = CHECKSUM;
                    else messageError = COMPLETE;
                }
        }
        *messageState = state;
    }
    return messageError;
}

/****************************************************************************/

