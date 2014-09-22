/* Bootloader Header file */

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

#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include <avr/sfr_defs.h>
#if (MCU_TYPE==1)
#include "defines-M168.h"
#elif (MCU_TYPE==2)
#include "defines-T4313.h"
#else
#error "Processor not defined"
#endif

#define TRUE 1
#define FALSE 0

/* Xbee parameters */
#define RF_PAYLOAD          63
/* XBee Frame Types */
#define DATA_RX             0x90
#define DATA_TX             0x10
#define TRANSMIT_STATUS     0X8B
#define MODEM_STATUS        0x8A

/** Convenience macros (we don't use them all) */
#define TRUE 1
#define FALSE 0

/* The rxFrameType can be expressed as an Rx Request or AT Command Response frame */
typedef struct
{
    uint16_t length;
    uint8_t checksum;
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
            uint8_t data[RF_PAYLOAD];
        } atResponse;
        struct
        {
            uint8_t frameID;
            uint8_t sourceAddress64[8];
            uint8_t sourceAddress16[2];
            uint8_t atCommand1;
            uint8_t atCommand2;
            uint8_t status;
            uint8_t data[RF_PAYLOAD];
        } remoteResponse;
        struct
        {
            uint8_t frameID;
            uint8_t sourceAddress64[8];
            uint8_t sourceAddress16[2];
            uint8_t retryCount;
            uint8_t deliveryStatus;
            uint8_t discoveryStatus;
        } txStatus;
        struct
        {
            uint8_t sourceAddress64[8];
            uint8_t sourceAddress16[2];
            uint8_t options;
            uint8_t data[RF_PAYLOAD];
        } rxRequest;
    } message;
} rxFrameType;

/* The txFrameType can be expressed as a Tx Request or AT Command frame */
/* Note that only a single byte parameter is provided in the atCommand structure */
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

typedef enum {ready, inprogress, checksum, statemachine} messageError;
typedef enum {pageStart, pageFilling, pageErasing, pageWriting} pState;

/* Prototypes */
messageError parseMessage(uint8_t inputChar, uint8_t *messageState, rxFrameType *message);
void sendDataMessage(uint8_t *ch, uint8_t messageLength);

#endif

