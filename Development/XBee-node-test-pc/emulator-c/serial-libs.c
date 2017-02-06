/*      POSIX serial I/O library

Substitute functions for XBee-node-test to provide an emulated communications
environment.

This uses Linux system calls to emulate the serial communications with the XBee.

Note that the serial port "port" is created and opened in the wrapper main
program. It is shared as extern through the header file. This allows it to be
used here but set through the command line interface.
*/
/****************************************************************************
 *   Copyright (C) 2016 by Ken Sarkies (www.jiggerjuice.info)               *
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

#include "../../../libs/serial.h"
#include "xbee-node-test.h"

#define  high(x) ((unsigned char) (x >> 8) & 0xFF)
#define  low(x) ((unsigned char) (x & 0xFF))

#define BUFFER_SIZE 256

static int bufferPointer;
static int bufferSize;
static char inBuf[BUFFER_SIZE];

/*-----------------------------------------------------------------------------*/
/* Initialise the UART

Setting baudrate, Rx/Tx enables, and flow controls.
The buffer pointer and size are reset.

The UART has already been initialised in the emulator code.
*/
void uartInit(void)
{
    bufferPointer = 0;
    bufferSize = 0;
}

/*-----------------------------------------------------------------------------*/
/* Send a character

Use the POSIX write system call to send a single character.
*/

void sendch(unsigned char c)
{
    write(port,&c,1);
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready (non blocking)

This uses the POSIX read system call to access all data in the buffer. One
character is returned at a time.

returns: unsigned int. Upper byte is zero or NO_DATA if no character present.
*/

unsigned int getch(void)
{
    if (bufferSize == 0)         /* If empty try to fill buffer */
    {
        int numRead = read(port, inBuf, BUFFER_SIZE);
        if (numRead == 0) return (NO_DATA << 8);
        if (numRead < 0) return (FRAME_ERROR<<8);
        bufferSize += numRead;
        bufferPointer = 0;      /* Restart reading at the beginning of buffer */
    }
    bufferSize--;
    return inBuf[bufferPointer++];
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready (blocking)

returns: unsigned int. The upper byte is zero or NO_DATA if no character present.
*/

unsigned char getchb(void)
{
    char c;
    do
    {
        c = getch();
    }
    while (high(c) == NO_DATA);
    return low(c);
}

