/*        XBee AVR Node Firmware
       Ken Sarkies (www.jiggerjuice.info)
            3 February 2017

version     0.0
Software    AVR-GCC 4.8.2
Target:     Any AVR with sufficient output ports and a timer
Tested:     ATTiny4313 at 1MHz internal clock.

*/
/****************************************************************************
 *   Copyright (C) 2017 by Ken Sarkies (www.jiggerjuice.info)               *
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

#ifndef XBEE_NODE_TEST_H_C
#define XBEE_NODE_TEST_H_C

#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>

#define DEBUG   1

#define LOG_FILE            "../xbee-node-test.dat"

/* Port default */
#define SERIAL_PORT         "/dev/ttyUSB0"
#define BAUDRATE            B38400

/* Choose whether to use hardware flow control for serial comms. */
//#define USE_HARDWARE_FLOW

/* Serial Port Parameters */

enum DataBitsType
{
    DATA_5 = 5,
    DATA_6 = 6,
    DATA_7 = 7,
    DATA_8 = 8
};

enum ParityType
{
    PAR_NONE,
    PAR_ODD,
    PAR_EVEN,
    PAR_SPACE
};

enum StopBitsType
{
    STOP_1,
    STOP_2
};

enum FlowType
{
    FLOW_OFF,
    FLOW_HARDWARE,
    FLOW_XONXOFF
};

extern int port;            /* Serial port file descriptor */
extern bool debug;

#endif /*_XBEE_NODE_TEST_H_C */
