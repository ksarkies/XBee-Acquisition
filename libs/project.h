/* Definitions Specific to the Project

These are independent of the hardware features of the processor.
*/

/****************************************************************************
 *   Copyright (C) 2013 by Ken Sarkies (www.jiggerjuice.info)               *
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

/* Choose whether to use hardware flow control for serial comms.
Needed for the bootloader as the upload is extensive. */
//#define USE_HARDWARE_FLOW

/* Choose whether to use buffering for serial communications. */
#define USE_RECEIVE_BUFFER
#define USE_TRANSMIT_BUFFER

/* Interrupts will normally be needed with serial buffering */
#ifdef USE_TRANSMIT_BUFFER
#define TRANSMIT_BUFFER_SIZE 32
#define USE_TRANSMIT_INTERRUPTS
#endif
#ifdef USE_RECEIVE_BUFFER
#define RECEIVE_BUFFER_SIZE 32
#define USE_RECEIVE_INTERRUPTS
#endif

/* Use the defined output pin to force the XBee to stay awake while in the
bootloader. This is valid for the XBee sleep mode 1 only. The application
should move it to other modes if necessary. Note that using this may fail
because the output pins may be forced to an undesired level during programming. */
#define XBEE_STAY_AWAKE         false

/* These defines control how the bootloader interacts with hardware */
/* Use the defined input pin to decide if the application will be entered
automatically */
#define AUTO_ENTER_APP          1

