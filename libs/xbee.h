/* Header file for AVR XBee */

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

#ifndef XBEE_H
#define XBEE_H

/** @name Error Definitions.
@{*/
/* From the XBee receiver process: */
#define XBEE_STATE_MACHINE      0x10    /* Trying to read beyond the message length */
#define XBEE_CHECKSUM           0x11    /* Message checksum error */
#define XBEE_ACK                0x12
#define XBEE_NAK                0x13
#define XBEE_INCOMPLETE         0x14    /* More characters to come */
#define XBEE_COMPLETE           0x15    /* Message completed OK */
#define XBEE_NO_CHARACTER       0x16    /* No character present */
/*@}*/

/* Xbee parameters */
#define RF_PAYLOAD              32

/* Frame Types */
#define AT_COMMAND              0x08
#define TX_REQUEST              0x10
#define AT_COMMAND_RESPONSE     0x88
#define MODEM_STATUS            0x8A
#define TX_STATUS               0x8B
#define RX_PACKET               0x90
#define IO_DATA_SAMPLE          0x92
#define NODE_IDENT              0x95

/* Serial buffer size */
#define BUFFER_SIZE 60

/* The rxFrameType can be expressed as an Rx Request or AT Command Response frame */
typedef struct
{
    uint16_t length;
    uint8_t frameType;
    union
    {
        uint8_t array[RF_PAYLOAD+13];
        struct
        {
            uint8_t frameID;
            uint8_t atCommand1;
            uint8_t atCommand2;
            uint8_t status;
            uint8_t data[RF_PAYLOAD+9];
        } atResponse;
        struct
        {
            uint8_t sourceAddress64[8];
            uint8_t sourceAddress16[2];
            uint8_t options;
            uint8_t data[RF_PAYLOAD];
        } rxPacket;
        struct
        {
            uint8_t frameID;
            uint8_t sourceAddress16[2];
            uint8_t retryCount;
            uint8_t deliveryStatus;
            uint8_t discoveryStatus;
        } txStatus;
        struct
        {
            uint8_t status;
        } modemStatus;
    } message;
    uint8_t checksum;
} rxFrameType;

/* The txFrameType can be expressed as a Tx Request or AT Command frame */
typedef struct
{
    uint16_t length;
    uint8_t frameType;
    union
    {
        uint8_t array[RF_PAYLOAD+15];
        struct
        {
            uint8_t frameID;
            uint8_t atCommand1;
            uint8_t atCommand2;
            uint8_t parameter;
        } atCommand;
        struct
        {
            uint8_t frameID;
            uint8_t sourceAddress64[8];
            uint8_t sourceAddress16[2];
            uint8_t radius;
            uint8_t options;
            uint8_t data[RF_PAYLOAD];
        } txRequest;
    } message;
    uint8_t checksum;
} txFrameType;

typedef enum {associationCheck, batteryCheck, transmit} txStage;
typedef enum {no_error, timeout, unknown_frame_type, checksum_error, frame_error,
              modem_status, node_ident, io_data_sample,
              protocol_error, command_error, unknown_error, no_character} packet_error;

/*----------------------------------------------------------------------*/
/* Prototypes */

void sendTxRequestFrame(const uint8_t sourceAddress64[],
                        const uint8_t sourceAddress16[],
                        const uint8_t radius, const uint8_t dataLength,
                        const uint8_t data[]);
void sendATFrame(const uint8_t dataLength, const char data[]);
void sendBaseFrame(const txFrameType txMessage);
uint8_t receiveMessage(rxFrameType *rxMessage, uint8_t *messageState);
bool checkAssociated(void);
bool resetXBeeSoft(void);
int8_t readXBeeIO(uint8_t* data);
uint16_t getXBeeADC(uint8_t* data, uint8_t adcPort);

#endif

