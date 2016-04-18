/* Header file for AVR XBee */

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

#ifndef XBEE_H
#define XBEE_H

/** @name Error Definitions.
@{*/
/* From the XBee receiver process: */
#define STATE_MACHINE           0x10
#define CHECKSUM                0x11
#define ACK                     0x12
#define NAK                     0x13
#define COMPLETE                0x20
/*@}*/

/* Xbee parameters */
#define RF_PAYLOAD              63

#define RX_REQUEST              0x90
#define TX_REQUEST              0x10

/* The rxFrameType can be expressed as an Rx Request or AT Command Response frame */
typedef struct
{
    uint16_t length;
    uint8_t checksum;
    uint8_t frameType;
    uint8_t frameId;
    union
    {
        uint8_t array[RF_PAYLOAD+13];
        struct
        {
            uint8_t sourceAddress64[8];
            uint8_t sourceAddress16[2];
            uint8_t options;
            uint8_t data[RF_PAYLOAD];
        } rxRequest;
        struct
        {
            uint8_t frameId;
            uint8_t sourceAddress16[2];
            uint8_t retryCount;
            uint8_t deliveryStatus;
            uint8_t discoveryStatus;
        } txStatus;
    } message;
} rxFrameType;

/* The txFrameType can be expressed as a Tx Request or AT Command frame */
typedef struct
{
    uint16_t length;
    uint8_t checksum;
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
} txFrameType;

/*----------------------------------------------------------------------*/
/* Prototypes */

void sendTxRequestFrame(const uint8_t sourceAddress64[],
                        const uint8_t sourceAddress16[],
                        const uint8_t radius, const uint8_t dataLength,
                        const uint8_t data[]);
void sendBaseFrame(const txFrameType txMessage);

#endif

