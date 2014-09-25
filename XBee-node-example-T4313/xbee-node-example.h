/*        XBee AVR Node Example
       Ken Sarkies ksarkies@internode.on.net
            23 September 2014

version     0.0.0
Software    AVR-GCC 4.8.2
Target:     Any AVR with sufficient output ports and a timer
Tested:     ATMega48 at 8MHz internal clock.
            ATMega168 at 8MHz internal clock.
            ATTiny4313 at 1MHz clock.

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

/* 0.256ms clock from 1MHz clock
Timer clock scale value 3 gives scale of 64, (see timer.c)
This gives a 16ms overflow interrupt.*/
#define RTC_SCALE           3

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

/* Prototypes */

#endif /*_XBEE_NODE_EXAMPLE_H_ */
