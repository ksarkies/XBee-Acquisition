/*  XBee API routines for the AVR UART

These provide the interface to the XBee via the serial port. Serial buffers
are defined and all communications are carried out through these buffers.
ISRs for transmit and receive of serial characters must be defined elsewhere
and call back to functions defined here to put and get characters to/from the
buffers.

The transmit buffer may not be needed and can be disabled to save space.
*/

/****************************************************************************
 *   Copyright (C) 2013 by Ken Sarkies (www.jiggerjuice.info)               *
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

#include "defines.h"
#include "xbee.h"
#include "serial.h"
#include "buffer.h"
#include <util/delay.h>

/* Global Variables */
/* Serial communications buffers. */
#ifdef USE_SEND_BUFFER
static uint8_t send_buffer[BUFFER_SIZE+3];
#endif
#ifdef USE_RECEIVE_BUFFER
static uint8_t receive_buffer[BUFFER_SIZE+3];
#endif

/****************************************************************************/
/** @brief Initialise the Communications Buffers

The receive and transmit buffers are confined to this module.

Globals: USE_SEND_BUFFER, receive_buffer
*/

void initBuffers(void)
{
#ifdef USE_SEND_BUFFER
    buffer_init(send_buffer,BUFFER_SIZE);
#endif
#ifdef USE_RECEIVE_BUFFER
    buffer_init(receive_buffer,BUFFER_SIZE);
#endif
}

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
#ifdef USE_SEND_BUFFER
    buffer_put(send_buffer, 0xFF-checksum);
#else
    sendch(0xFF-checksum);
#endif
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
#ifdef USE_RECEIVE_BUFFER
    uint16_t inputChar = buffer_get(receive_buffer);
#else
    uint16_t inputChar = getch();
#endif
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
                if (inputValue == 0x7E) state++;
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
is verified or until it times out. If a test port is defined it is flashed.

@returns bool: associated
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
                     (rxMessage.message.atResponse.status == 0) && \
                     (rxMessage.frameType == AT_COMMAND_RESPONSE) && \
                     (rxMessage.message.atResponse.atCommand1 == 'A') && \
                     (rxMessage.message.atResponse.atCommand2 == 'I'));
#ifdef TEST_PORT
        if (! associated)
        {
            _delay_ms(200);
            sbi(TEST_PORT,TEST_PIN);    /* Set pin on */
            _delay_ms(200);
            cbi(TEST_PORT,TEST_PIN);    /* Set pin off */
        }
#endif
        if (count++ > 10) break;
    }
    return associated;
}

/****************************************************************************/
/** @brief Software reset XBee.

This attempts a software reset of the XBee and expects a successful response.

@returns bool: reset completed OK
*/

bool resetXBeeSoft(void)
{
    uint8_t messageState = 0;   /* Progress in message reception */
    rxFrameType rxMessage;
    uint8_t count = 0;
    bool reset = false;
    uint8_t messageError;
    while (! reset)
    {
        sendATFrame(2,"FR");

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
        reset = ((messageError == XBEE_COMPLETE) && \
                 (rxMessage.message.atResponse.status == 0) && \
                 (rxMessage.frameType == AT_COMMAND_RESPONSE) && \
                 (rxMessage.message.atResponse.atCommand1 == 'F') && \
                 (rxMessage.message.atResponse.atCommand2 == 'R'));
#ifdef TEST_PORT
        if (! reset)
        {
            _delay_ms(200);
            sbi(TEST_PORT,TEST_PIN);    /* Set pin on */
            _delay_ms(200);
            cbi(TEST_PORT,TEST_PIN);    /* Set pin off */
        }
#endif
        if (count++ > 10) break;
    }
    return reset;
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

Extract the ADC data that was previously obtained. The XBee returns a variable
length data field depending on what ports have been enabled.

@param[in] uint8_t* data: data array from the XBee after a readXBeeIO call.
@param[in] uint8_t adcPort: 0-4. The XBee ADC port to be accessed.
@returns uint16_t AD2 value; XBee A/D converter is 10 bits.
*/

uint16_t getXBeeADC(uint8_t* data, uint8_t adcPort)
{
    uint16_t value = 0;
    uint8_t i = 4;
    if ((data[1] > 0) || (data[2] > 0)) i = 6;  /* Skip digital data present */
    uint8_t port = 0;                           /* Check through ADC data mask */
    for (port = 0; port < 5; port++)
    {
        if (port == adcPort)
        {
            value = (data[i] << 8) + data[i+1];
            break;
        }
        if ((data[3] & _BV(port)) > 0) i += 2;  /* Skip enabled ADCs */
    }
    return value;
}

/****************************************************************************/
/** @brief Get Byte from Transmit buffer

This is a callback from an ISR somewhere that transmits characters from a
serial port. The character is retrieved from the transmit buffer.

Globals: receive_buffer
*/

#ifdef USE_SEND_BUFFER
uint16_t getTransmitBuffer(void)
{
	return buffer_get(send_buffer);
}
#endif

/****************************************************************************/
/** @brief Put Byte to Receive buffer

This is a callback from an ISR somewhere that receives characters from a
serial port. The character is placed in the receive buffer.

Globals: receive_buffer
*/

#ifdef USE_RECEIVE_BUFFER
void put_receive_buffer(uint8_t data)
{
	buffer_put(receive_buffer,data);
}
#endif

/****************************************************************************/

