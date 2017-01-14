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

/* Time in ms XBee waits before sleeping */
#define PIN_WAKE_PERIOD         1

/* Choose whether to use hardware flow control for serial comms.
Needed for the bootloader as the upload is extensive. */
#define USE_HARDWARE_FLOW

/* Choose whether to use buffering for serial communications. */
//#define USE_RECEIVE_BUFFER
//#define USE_SEND_BUFFER

/* Interrupts will normally be needed with serial buffering */
#if defined USE_RECEIVE_BUFFER || defined USE_SEND_BUFFER
#define USE_INTERRUPTS
#endif

/* Use the defined output pin to force the XBee to stay awake while in the
bootloader. This is valid for the XBee sleep mode 1 only. The application
should move it to other modes if necessary. Note that using this may fail
because the output pins may be forced to an undesired level during programming. */
#define XBEE_STAY_AWAKE         1

/* These defines control how the bootloader interacts with hardware */
/* Use the defined input pin to decide if the application will be entered
automatically */
#define AUTO_ENTER_APP          1

/****************************************************************************/
/* Prototypes */

void inline hardwareInit(void);
void sendDataCommand(const uint8_t command, const uint32_t datum);
void sendMessage(const char* data);
void sleepXBee(void);
void wakeXBee(void);

#endif /*_XBEE_NODE_TEST_H_ */
