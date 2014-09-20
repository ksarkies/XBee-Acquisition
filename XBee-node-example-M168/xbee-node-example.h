/*        XBee AVR Node Example
       Ken Sarkies ksarkies@internode.on.net
            4 January 2013

version     0.0.0
Software    AVR-GCC 4.8.2
Target:     Any AVR with sufficient output ports and a timer
Tested:     ATMega48 at 8MHz internal clock.

*/
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

#ifndef _XBEE_NODE_EXAMPLE_H_
#define _XBEE_NODE_EXAMPLE_H_

#include <inttypes.h>

/* 0.128ms clock for 8MHz clock and value 5 (gives scale 1024, see timer.c)
This gives a 32ms overflow interrupt.*/
#define RTC_SCALE           5

/* Baudrate settings */
#define UART_BAUD_RATE      38400

#define BAUDRATE UART_BAUD_SELECT(UART_BAUD_RATE,F_CPU)

/* Xbee parameters */
#define RF_PAYLOAD  63

/**********************************************************/
/** @name Error Definitions.
From the UART:
@{*/
#define NO_DATA                 0x01
#define BUFFER_OVERFLOW         0x02
#define OVERRUN_ERROR           0x04
#define FRAME_ERROR             0x08

#define STATE_MACHINE           0x10
#define CHECKSUM                0x11
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

/* Prototypes */

void hardwareInit(void);
void timerInit(void);
void resetTimer(void);

/* XBee related prototypes */

void sendBaseFrame(txFrameType txMessage);
void sendTxRequestFrame(uint8_t sourceAddress64[], uint8_t sourceAddress16[],
                        uint8_t radius, uint8_t length, uint8_t data[]);

#endif /*_XBEE_NODE_EXAMPLE_H_ */
