/*        XBee AVR Node Test

       Ken Sarkies ksarkies@internode.on.net
            25 September 2014

version     0.0.0
Software    AVR-GCC 4.8.2
Target:     Any AVR with sufficient output ports and a timer
Tested:     ATMega48 at 8MHz internal clock, and ATTiny841 at 1MHz.

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

#ifndef _XBEE_NODE_TEST_H_
#define _XBEE_NODE_TEST_H_

#include <inttypes.h>

/* Adapt operation to sleep the XBee and allow battery measurement */
#define SLEEP_XBEE
#define BATTERY_MEASURE

/* Time in ms XBee waits before sleeping */
#define PIN_WAKE_PERIOD         1

#endif /*_XBEE_NODE_TEST_H_ */
