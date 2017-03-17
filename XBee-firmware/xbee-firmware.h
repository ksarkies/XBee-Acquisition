/*        XBee AVR Node Firmware
       Ken Sarkies (www.jiggerjuice.info)
            21 July 2014

version     1.0
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
#define ACTION_MINUTES          2

/* Timeout setting for WDT to give 8 second ticks */
#define WDT_TIME                0x09

//#define ACTION_COUNT    (ACTION_MINUTES*60)/8
#define ACTION_COUNT            1       /* Temporary test value */

/* Time in ms XBee waits before sleeping */
#define PIN_WAKE_PERIOD         1

/* Number of program loop cycles to wait for a response from the base station
following a data transmission. This depends on the code execution time needed
to check for a received character, and F_CPU. Aim at 200ms with an assumption
that 10 clock periods are needed for each cycle. F_CPU is in Hz. */
#define RESPONSE_DELAY          (F_CPU/1000)/10*200

/* Response for a Tx Status frame should be smaller. Aim at 100ms */
#define TX_STATUS_DELAY         (F_CPU/1000)/10*100

/* Time to mute counter update following a transmission 10ms. */
#define MUTE_TIME               (F_CPU/1000)/10*10

#endif /*_XBEE_NODE_H_ */
