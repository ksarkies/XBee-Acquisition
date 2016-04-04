/*      POSIX/QT serial I/O

Substitute functions for XBee-node-test to provide an emulated communications
environment.
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

#define SERIAL_PORT     "/dev/ttyUSB0"
#define BAUDRATE        38400

#define  high(x) ((unsigned char) (x >> 8) & 0xFF)
#define  low(x) ((unsigned char) (x & 0xFF))

static QSerialPort* socket1;

/*-----------------------------------------------------------------------------*/
/* Initialise the UART

Setting baudrate, Rx/Tx enables, and flow controls.
Currently uses fixed values as the test code uses a fixed port and baudrate.
*/
void uartInit(void)
{
    socket1 = new QSerialPort(SERIAL_PORT);
    bool ok = socket1->open(QIODevice::ReadWrite);
    if (ok)
    {
        socket1->setBaudRate(BAUDRATE);
        socket1->setDataBits(QSerialPort::Data8);
        socket1->setParity(QSerialPort::NoParity);
        socket1->setStopBits(QSerialPort::OneStop);
        socket1->setFlowControl(QSerialPort::NoFlowControl);
    }
}

/*-----------------------------------------------------------------------------*/
/* Send a character */

void sendch(unsigned char c)
{
    socket1->putChar(c);
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready (non blocking)

The function asserts RTS low then waits for the receive complete bit is set.
RTS is then cleared high. The character is then retrieved.

returns: unsigned int. The upper byte is zero or NO_DATA if no character present.
*/

unsigned int getchn(void)
{
    char c;
    bool ok = socket1->getChar(&c);
    if (ok) return (unsigned int)c;
    else return 0x100+(unsigned int)c;
}

/*-----------------------------------------------------------------------------*/
/* Get a character when the Rx is ready (blocking)

returns: unsigned int. The upper byte is zero or NO_DATA if no character present.
*/

unsigned char getch(void)
{
    unsigned int c;
    do
    {
        c = getchn();
    }
    while (high(c) == 0);
    return low(c);
}


