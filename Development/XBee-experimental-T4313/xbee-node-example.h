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

/* 0.256ms clock from 8MHz clock
Timer clock scale value 5 gives scale of 1024, (see timer.c)
This gives a 16ms overflow interrupt.*/
#define RTC_SCALE           5

/* Xbee parameters */
#define RF_PAYLOAD  63

#define RX_REQUEST              0x90
#define TX_REQUEST              0x10

/* Prototypes */

#endif /*_XBEE_NODE_EXAMPLE_H_ */
