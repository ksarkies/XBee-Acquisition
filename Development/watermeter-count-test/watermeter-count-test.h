/*        AVR Watermeter Node Firmware
       Ken Sarkies (www.jiggerjuice.info)               *
            21 July 2014

version     0.0
Software    AVR-GCC 4.8.2
Target:     Any AVR with sufficient output ports and a timer
Tested:     ATtiny4313 at 1MHz internal clock.

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

#ifndef _XBEE_NODE_H_
#define _XBEE_NODE_H_

/* WDT count to give desired time between activations of the AVR */
#define ACTION_MINUTES          10

/* Timeout setting for WDT to give 1 second ticks */
#define WDT_TIME                0x05
//#define WDT_TIME                0x09

//#define ACTION_COUNT    (ACTION_MINUTES*60)/8
#define ACTION_COUNT            0

/* Time to wait for a response from the base station. Time units depend on
the code execution time needed to check for a received character, and F_CPU.
Aim at 200ms with an assumption that 10 clock cycles needed for the check. */
#define RESPONSE_DELAY          F_CPU/50

/* Response for a Tx Status frame should be smaller. Aim at 100ms */
#define TX_STATUS_DELAY         F_CPU/100

/* Time to mute counter update following a transmission */
#define MUTE_TIME               F_CPU/1000

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
#define ACK                     0x12
#define NAK                     0x13
#define COMPLETE                0x20
/*@}*/

#define RX_REQUEST              0x90
#define TX_REQUEST              0x10

/****************************************************************************/
/* Prototypes */

void hardwareInit(void);
void wdtInit(const uint8_t waketime);
void sendDataCommand(const uint8_t command, const uint32_t datum);

#endif /*_XBEE_NODE_H_ */
