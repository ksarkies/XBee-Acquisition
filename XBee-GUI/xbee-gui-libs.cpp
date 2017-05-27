/*
Title:    XBee Control and Display GUI Tool Common Code Library
*/

/****************************************************************************
 *   Copyright (C) 2017 by Ken Sarkies ksarkies@internode.on.net            *
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

#include <QMessageBox>
#include <QtNetwork>
#include <QTcpSocket>
#include <QTextStream>
#include <QDebug>
#include "xbee-gui-libs.h"
#include "xbee-control.h"

comStatus communicationStatus;

//-----------------------------------------------------------------------------
/** @brief Send a command via the Internet Socket.

This will compute and fill the length, set the sync bytes, send the command and
wait for a reply up to 5s. If no reply it will return an error code. The return
is delayed while the communications status has not been changed to receive.

Globals: comStatus, comSent

@parameter  QByteArray *command: An array to send with length, command and any
                                parameters if appropriate.
            QTcpSocket *tcpSocket: TCP socket for base station.
@returns    0 if no error occurred.
            1 Invalid TCP socket.
            3 timeout waiting for response.
*/

int sendCommand(QByteArray *command, QTcpSocket *tcpSocket)
{
    if (tcpSocket == 0) return 1;
    int errorCode = 0;
    command->prepend(command->size()+1);
    setComStatus(comSent);
    QChar tcpCommand = command->at(1);
#ifdef DEBUG
    if ((tcpCommand != 'r') && (tcpCommand != 'l') && (tcpCommand != 's'))
        qDebug() << "sendCommand command sent:" << tcpCommand
                 << "Row" << (int)command->at(2)
                 << "string" << command->right(command->size()-3);
#endif
    command->append('\0');                  // Do this to terminate the string
    tcpSocket->write(*command);
// Wait up to 5s for a response. Should pass over to receiver function.
    if (! tcpSocket->waitForReadyRead(5000)) errorCode = 3;
    else
    {
// The receiver function should pick up and change away from sent status
// We need this to synchronize across the network and to avoid overload.
        while (getComStatus() == comSent);
    }
    return errorCode;
}

//-----------------------------------------------------------------------------
/** @brief Return the current communications status.

The status indicates whether the last operation was a transmit or receive.
This allows the GUI to wait for a response after a command was sent.
*/

comStatus getComStatus()
{
    return communicationStatus;
}

//-----------------------------------------------------------------------------
/** @brief Return the current communications status.

The status indicates whether the last operation was a transmit or receive.
This allows the GUI to wait for a response after a command was sent.
*/

void setComStatus(comStatus status)
{
    communicationStatus = status;
}

//-----------------------------------------------------------------------------
/** @brief Sleep for a number of centiseconds but keep processing events

*/

void ssleep(int centiseconds)
{
    for (int i=0; i<centiseconds*10; i++)
    {
        qApp->processEvents();
        millisleep(10);
    }
}

