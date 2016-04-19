/*      POSIX/QT serial I/O

Substitute functions for XBee-node-test to provide an emulated communications
environment.

Note that the serial port object "port" is created and opened in the wrapper
main program. It is shared as extern through the header file and is not
part of the XbeeNodeTest object. This allows it to be used here but defined
through the command line interface or GUI.
*/
/****************************************************************************
 *   Copyright (C) 2016 by Ken Sarkies ksarkies@internode.on.net            *
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

#include <QSerialPort>
#include <QSerialPortInfo>
#include <QDebug>
#include "../libs/serial.h"
#include "xbee-node-test.h"

#define  high(x) ((unsigned char) (x >> 8) & 0xFF)
#define  low(x) ((unsigned char) (x & 0xFF))

/*-----------------------------------------------------------------------------*/
/* Initialise the UART

Setting baudrate, Rx/Tx enables, and flow controls.

The UART has already been initialised in the emulator code.
*/
void uartInit(void)
{
}

/*-----------------------------------------------------------------------------*/
/* Send a character */

void sendch(unsigned char c)
{
    port->putChar(c);
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready (non blocking)

The function asserts RTS low then waits for the receive complete bit is set.
RTS is then cleared high. The character is then retrieved.

returns: unsigned int. The upper byte is zero or NO_DATA if no character present.
*/

unsigned int getch(void)
{
    unsigned int result = 0;
    if (port->bytesAvailable() == 0) result = (NO_DATA<<8);
    else
    {
        char c;
        bool ok = port->getChar(&c);
        if (ok) result = (unsigned int)c & 0xFF;
        else result = c+(FRAME_ERROR<<8);
    }
    return result;
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready (blocking)

returns: unsigned int. The upper byte is zero or NO_DATA if no character present.
*/

unsigned char getchb(void)
{
    unsigned int c;
    do
    {
        c = getch();
    }
    while (high(c) == 0);
    return low(c);
}

