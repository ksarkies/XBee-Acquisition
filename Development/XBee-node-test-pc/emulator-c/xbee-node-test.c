/**
@mainpage XBee AVR Node Test
@version 0.0.0
@author Ken Sarkies (www.jiggerjuice.info)
@date 2 April 2016
@brief Code for a POSIX system with an XBee on a serial port 

This code forms the core of an interface between an XBee networking device
using ZigBee stack, and a data acquisition unit making a variety of
measurements for communication to a base controller.

This is designed to run on a POSIX system with an XBee on a serial port. Delays
are implemented directly in the code.

The purpose of this is to provide a testbed for code debug.

The program run from the command line. A debug mode is available for extended
printout.

xbee-node-test -d -P /dev/ttyUSB0 -b 38400

In command line mode, the program can only be stopped by ctl-C or process kill.
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

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h> 
#include <termios.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "xbee-node-test.h"
#include "../xbee-node-firmware.h"

/*---------------------------------------------------------------------------*/
/* Globals */

bool debug;
int port;

/*---------------------------------------------------------------------------*/
/* Local prototypes */

bool set_serial_attribs(int port, int baudrate, int parity);
void set_blocking(int port, bool block);

/* Timer ISR to be simulated */
void _timerTick();
static unsigned int timerTickMs;
static unsigned int timerTickCount;

// The timer ISR is defined in mainprog.cpp, or is substituted with a null call
__attribute__((weak)) void timerISR() {}

//-----------------------------------------------------------------------------
/** @brief Main Program

Open a serial port and launch the running code.
*/

int main(int argc,char ** argv)
{
    char serialPort[20] = SERIAL_PORT;
    int baudrate = BAUDRATE;

/* Pull in command line options */
    int c;
    int baudParm;
    debug = false;
    opterr = 0;
    while ((c = getopt (argc, argv, "P:db:")) != -1)
    {
        switch (c)
        {
        case 'P':
            strcpy(optarg,serialPort);
            break;
        case 'd':
            debug = true;
            break;
        case 'b':
            baudParm = atoi(optarg);
            switch (baudParm)
            {
            case 1200: baudrate=B1200;break;
            case 2400: baudrate=B2400;break;
            case 4800: baudrate=B4800;break;
            case 9600: baudrate=B9600;break;
            case 19200: baudrate=B19200;break;
            case 38400: baudrate=B38400;break;
            case 57600: baudrate=B57600;break;
            case 115200: baudrate=B115200;break;
            default:
                fprintf (stderr, "Invalid Baudrate %i.\n", baudParm);
                return false;
            }
            break;
        case '?':
            if ((optopt == 'P') || (optopt == 'b'))
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option -%c.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
            default: return false;
        }
    }

/* Check that the serial port is available and initialise it.
uartInit() in the emulated code should be set to a null function. */
    struct stat fileStatus;
    if(stat(serialPort,&fileStatus) == 0)   /* Check if serial port exists */
    {
        port = open(serialPort, O_RDWR | O_NOCTTY | O_SYNC);
        if (port < 0)
        {
                printf("Unable to open serial port %s: %s\n",
                        serialPort, strerror(errno));
                return;
        }
        bool ok = set_serial_attribs(port, baudrate, 0);
        if (ok)
        {
                printf("Unable to change serial attributes %s: %s\n",
                        serialPort, strerror(errno));
                return;
        }
        set_blocking(port, false);          /* set no blocking */

/* Run emulated code. The test code is split to an initialisation part and an
operational part which falls within an (almost) infinite loop emulated here.
After initialisation of the timer the tick function is called to update the
counter. The ISR is called in that function when the limit is reached. */
        mainprogInit();                 // Initialization section
        while (true)
        {
/* Ensure time ticks over for calls to an emulated ISR, and call the ISR */
            _timerTick();
/* Place test code to be run here. */
            mainprog();
        }
    }
    else
        printf("Serial port not present %s\n",serialPort);

/* Set additional command line only actions, if any. */
    return true;
}

/****************************************************************************/
/* @brief Timer initialization

This function is only called in the main program if needed for timer
initialization.

@param[in] timerTrigger. Count at which a defined ISR is triggered.
*/

void timerInit(unsigned int timerTrigger)
{
    timerTickMs = 0;
    timerTickCount = timerTrigger;
}

/****************************************************************************/
/* brief Timer tick

This counts off a number of milliseconds until the selected timer count
has been completed, then calls the ISR as would happen with a hardware timer.

The number of milliseconds delay must be a multiple of 55 as the POSIX clock has
this granularity.

This is called in the outer loop of the test code in codeRun() above. However if
test code has inner loops it must be called there as well to ensure that the
timer continues to tick over.
*/

void _timerTick()
{
    usleep(55000);
    if (timerTickMs++ > timerTickCount/55)
    {
        timerTickMs = 0;
        timerISR();
    }
}

/*****************************************************************************/
/* brief Setup Serial Interface

Refer to POSIX struct termios and tcgetattr for definitions.

@param[in] int port: file descriptor.
@param[in] int baudrate.
@param[in] int parity. Setting for c_cflag. This can be any valid setting but is
                intended to be zero or a combination of PARENB and PARODD.
@returns true if successful.
*/

bool set_serial_attribs(int port, int baudrate, int parity)
{
    struct termios tty;
    memset (&tty, 0, sizeof tty);
    if (tcgetattr (port, &tty) != 0)
    {
        printf("Unable to get serial parameters: %d\n", errno);
        return false;
    }

    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
/* disable IGNBRK for mismatched speed tests; otherwise receive break
as \000 chars */
    tty.c_iflag &= ~IGNBRK;         // disable break processing
    tty.c_lflag = 0;                // no signaling chars, no echo,
                                    // no canonical processing
    tty.c_oflag = 0;                // no remapping, no delays
    tty.c_cc[VMIN]  = 0;            // read doesn't block
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY); // shut off xon/xoff ctrl

    tty.c_cflag |= (CLOCAL | CREAD);// ignore modem controls,
                                    // enable reading
    tty.c_cflag &= ~(PARENB | PARODD);      // shut off parity
    tty.c_cflag |= parity;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(port, TCSANOW, &tty) != 0)
    {
        printf("Unable to set serial parameters: %d\n", errno);
        return false;
    }
    return true;
}

/*****************************************************************************/
/* brief Setup Serial Interface

Refer to POSIX struct termios and tcgetattr for definitions.

@param[in] int port: file descriptor.
@param[in] bool block.
*/

void set_blocking(int port, bool block)
{
    struct termios tty;
    memset(&tty, 0, sizeof tty);
    if (tcgetattr(port, &tty) != 0)
    {
        printf("Unable to get serial parameters: %d\n", errno);
        return;
    }
    tty.c_cc[VMIN]  = 0;
    if (block) tty.c_cc[VMIN]  = 1;
    tty.c_cc[VTIME] = 5;            // 0.5 seconds read timeout

    if (tcsetattr(port, TCSANOW, &tty) != 0)
        printf("Unable to set serial parameters: %d\n", errno);
}

