/*        XBee AVR Node Firmware
       Ken Sarkies (www.jiggerjuice.info)
            21 July 2014

version     0.0
Software    AVR-GCC 4.8.2
Target:     Any AVR with sufficient output ports and a timer
Tested:     ATtint4313 at 1MHz internal clock.

*/
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

#ifndef _XBEE_NODE_H_
#define _XBEE_NODE_H_

/* WDT count to give desired time between activations of the AVR */
#define ACTION_MINUTES          10

/* Timeout setting for WDT to give 8 second ticks */
#define WDT_TIME                0x09

//#define ACTION_COUNT    (ACTION_MINUTES*60)/8
#define ACTION_COUNT            1

/* Xbee parameters */
#define RF_PAYLOAD              63
/* Time in ms XBee waits before sleeping */
#define PIN_WAKE_PERIOD         1
/* Time to wait for a response from the base station. Time units depend on
the code execution time needed to check for a received character, and F_CPU.
Aim at 200ms with an assumption that 10 clock cycles needed for the check. */
#define RESPONSE_DELAY          F_CPU/50
/* Response for a Tx Status frame should be smaller. Aim at 100ms */
#define TX_STATUS_DELAY         F_CPU/100

/**********************************************************/
/** @name Error Definitions.
From the UART:
@{*/
#define NO_DATA                 0x01
#define BUFFER_OVERFLOW         0x02
#define OVERRUN_ERROR           0x04
#define FRAME_ERROR             0x08

#define INVALID_LENGTH          0x10
#define CHECKSUM                0x11
#define ACK                     0x12
#define NAK                     0x13
#define COMPLETE                0x20
/*@}*/

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

typedef enum {waiting, packetError, packetReady} baseResponse_t;
typedef enum {OK, usartError, invalidLength, checksum} messageError_t;

/****************************************************************************/
/* Prototypes */

void hardwareInit(void);
void wdtInit(const uint8_t waketime);
void sendMessage(const uint8_t* data);
void sendDataCommand(const uint8_t command, const uint32_t datum);
void sleepXBee(void);

/* XBee related prototypes */

void sendBaseFrameconst (txFrameType txMessage);
void sendTxRequestFrame(const uint8_t sourceAddress64[], const uint8_t sourceAddress16[],
                        const uint8_t radius, const uint8_t length, const uint8_t data[]);

/* Prototypes for tasks */
void mainTask(void)         __attribute__ ((naked));
void receiveTask(void)      __attribute__ ((naked));

#endif /*_XBEE_NODE_H_ */
