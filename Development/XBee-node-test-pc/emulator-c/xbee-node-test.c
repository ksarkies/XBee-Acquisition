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
#include <sys/file.h>
#include "xbee-node-test.h"
#include "../xbee-firmware.h"

/*---------------------------------------------------------------------------*/
/* Globals */

bool debug;
int port;
FILE *fp;                   /* File for results */

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
    char logFileName[40];
    strcpy(logFileName,LOG_FILE);
    char serialPort[20] = SERIAL_PORT;
    int baudrate = BAUDRATE;

/* Pull in command line options */
    int c;
    int baudParm;
    debug = false;
    opterr = 0;
    while ((c = getopt (argc, argv, "P:dnb:L:")) != -1)
    {
        switch (c)
        {
        case 'P':
            strcpy(serialPort,optarg);
            break;
        case 'd':
            debug = true;
            break;
        case 'L':
            strcpy(logFileName,optarg);
            break;
        case 'n':
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
/* Open as R/W and no controlling terminal */
        port = open(serialPort, O_RDWR | O_NOCTTY | O_SYNC);
        if (port < 0)
        {
            printf("Unable to open serial port %s: %s\n",
                    serialPort, strerror(errno));
            return false;
        }
        if (flock(port, LOCK_EX | LOCK_NB) < 0)
        {
            printf("XBee is in use on port %s\n", serialPort);
            return false;
        }
        bool ok = set_serial_attribs(port, baudrate, 0);
        if (! ok)
        {
            printf("Unable to change serial attributes %s: %s\n",
                    serialPort, strerror(errno));
            return false;
        }

printf("Running, baudrate %d port %s\n", baudrate, serialPort);
/* Attempt to open a log file */
    fp = fopen(logFileName, "a");
    if (fp == NULL)
    {
        printf("Cannot open new results file");
    }
/* Run emulated code. The test code is split to an initialisation part and an
operational part which falls within an (almost) infinite loop emulated here.
After initialisation of the timer the tick function is called to update the
counter. The ISR is called in that function when the limit is reached.
If the main program returns a false condition, restart everything. */
        while (true)
        {
            mainprogInit();                     /* Initialization section */
            while (true)
            {
/* Ensure time ticks over for calls to an emulated ISR, and call the ISR */
                _timerTick();
/* Place test code to be run here. */
                if (! mainprog()) break;
            }
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
    memset(&tty, 0, sizeof tty);    // Fill with zeros
    if (tcgetattr(port, &tty) != 0)
    {
        printf("Unable to get serial parameters: %d\n", errno);
        return false;
    }

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
/* disable IGNBRK for mismatched speed tests; otherwise receive break
as \000 chars */
    tty.c_iflag &= ~IGNBRK;             /* disable break processing */
    tty.c_iflag &= ~(IGNPAR | PARMRK);  /* disable parity checks */
    tty.c_iflag &= ~INPCK;              /* disable parity checking */
    tty.c_iflag &= ~ISTRIP;             /* disable stripping 8th bit */
    tty.c_iflag &= ~(INLCR | ICRNL);    /* disable translating NL <-> CR */
    tty.c_iflag &= ~IGNCR;              /* disable ignoring CR */
    tty.c_iflag &= ~(IXON | IXOFF);     /* disable XON/XOFF flow control */
    tty.c_oflag &= ~ OPOST;             /* disable output processing */
    tty.c_oflag &= ~(ONLCR | OCRNL);    /* disable translating NL <-> CR */
    tty.c_oflag &= ~OFILL;              /* disable fill characters */
    tty.c_cflag |=  CLOCAL;             /* prevent changing ownership */
    tty.c_cflag |=  CREAD;              /* enable receiver */
    tty.c_cflag &= ~PARENB;             /* disable parity */
    if (baudrate >= B115200)
        tty.c_cflag |=  CSTOPB;         /* enable 2 stop bits for the high baudrate */
    else
	    tty.c_cflag &= ~CSTOPB;         /* disable stop bits */
    tty.c_cflag &= ~CSIZE;              /* remove size flag */
    tty.c_cflag |=  CS8;                /* enable 8 bit characters */
    tty.c_cflag |=  HUPCL;              /* enable lower control lines on close - hang up */
#ifdef USE_HARDWARE_FLOW
	tty.c_cflag |=  CRTSCTS;            /* enable hardware CTS/RTS flow control */
#else
	tty.c_cflag &= ~CRTSCTS;            /* disable hardware CTS/RTS flow control */
#endif
    tty.c_lflag &= ~ISIG;               /* disable generating signals */
    tty.c_lflag &= ~ICANON;             /* disable canonical mode - line by line */
    tty.c_lflag &= ~ECHO;               /* disable echoing characters */
    tty.c_lflag &= ~ECHONL;
    tty.c_lflag &= ~NOFLSH;             /* disable flushing on SIGINT */
    tty.c_lflag &= ~IEXTEN;             /* disable input processing */

    memset(tty.c_cc,0,sizeof(tty.c_cc));    /* Null out control characters */
    tty.c_cc[VMIN]  = 0;                /* read doesn't block */
    tty.c_cc[VTIME] = 0;                /* 0.5 seconds read timeout */
    cfsetspeed(&tty, baudrate);

    tcflush(port, TCIFLUSH);
    if (tcsetattr(port, TCSANOW, &tty) != 0)
    {
        printf("Unable to set serial parameters: %d\n", errno);
        return false;
    }
    return true;
}

