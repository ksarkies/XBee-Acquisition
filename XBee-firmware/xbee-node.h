/*        XBee AVR Node Firmware
       Ken Sarkies ksarkies@internode.on.net
            21 July 2014

version     0.0
Software    AVR-GCC 4.8.2
Target:     Any AVR with sufficient output ports and a timer
Tested:     ATtint4313 at 1MHz internal clock.

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

#ifndef _XBEE_NODE_H_
#define _XBEE_NODE_H_

/* WDT count to give desired time between activations of the AVR */
#define ACTION_MINUTES          2

/* Timeout setting for WDT to give 8 second ticks */
#define WDT_TIME                0x09

//#define ACTION_COUNT    (ACTION_MINUTES*60)/8
#define ACTION_COUNT            4

/* Xbee parameters */
#define RF_PAYLOAD              63

/* Time in ms XBee waits before sleeping */
#define PIN_WAKE_PERIOD         1

/* Time to wait for a response from the base station. Time units depend on
the code execution time needed to check for a received character, and F_CPU.
Aim at 200ms with an assumption that 10 clock cycles needed for the check. */
#define RESPONSE_DELAY          F_CPU/100

/* Response for a Tx Status frame should be smaller. Aim at 100ms */
#define TX_STATUS_DELAY         F_CPU/100

/* Time to mute counter update following a transmission */
#define MUTE_TIME               F_CPU/1000

/* Choose whether to use hardware flow control for serial comms.
Needed for the bootloader as the upload is extensive. */
//#define USE_HARDWARE_FLOW

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

void hardwareInit(void);
void wdtInit(const uint8_t waketime);
void sendDataCommand(const uint8_t command, const uint32_t datum);
void sendMessage(const char* data);
void sleepXBee(void);
void wakeXBee(void);

#endif /*_XBEE_NODE_H_ */
