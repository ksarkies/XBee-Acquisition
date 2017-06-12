/*       Node Configuration Dialogue GUI Tool

This provides a GUI interface to an XBee coordinator process running on the
local or remote Internet connected PC or Linux based controller for the XBee
acquisition network. This file manages the configuration of the remote node
through the inbuilt command interface in the node MCU.
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

#include <QApplication>
#include <QString>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>
#include <QBasicTimer>
#include <QtNetwork>
#include <QTcpSocket>
#include <QTextStream>
#include <QCloseEvent>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include "node-config.h"
#include "xbee-control.h"
#include "xbee-gui-libs.h"

// Global data
QByteArray dataReplyString;

//-----------------------------------------------------------------------------
/** Constructor

@parameter int row. The table row number for the remote node.
@parameter bool remote. True if a remote node, false if local XBee.
@parameter parent Parent widget.
*/

NodeConfigWidget::NodeConfigWidget(QString tcpAddress, uint tcpPort,
                                   int nodeRow, int xtimeout,
                                   QWidget* parent)
                    : QWidget(parent), row(nodeRow)
{
    timeout = xtimeout;
// Create the TCP socket to the internet process
    tcpSocket = new QTcpSocket(this);
// Setup QT signal/slots for reading and error handling
// The readyRead signal from the QAbstractSocket is linked to the readXbeeProcess slot
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readXbeeProcess()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
             this, SLOT(displayError(QAbstractSocket::SocketError)));
// Connect to the host
    tcpSocket->abort();
    tcpSocket->connectToHost(tcpAddress, tcpPort);
    if (! tcpSocket->waitForConnected(10000)) exit(1);
// Ask for information about the remote node as stored in the base station.
    QByteArray rowGetCommand;
    rowGetCommand.append('I');
    rowGetCommand.append(char(row));
    comCommand = rowGetCommand.at(0);
    sendCommand(&rowGetCommand, tcpSocket);
// Setup Form
    NodeConfigFormUi.setupUi(this);
}

NodeConfigWidget::~NodeConfigWidget()
{
}

//-----------------------------------------------------------------------------
/** @brief Quit the window.

This closes the window from the quit button and restores the sleep setting if
it was changed.
*/
void NodeConfigWidget::on_closeButton_clicked()
{
    accept();
    close();
}

/** Also deal the same way with the close window button */
void NodeConfigWidget::closeEvent(QCloseEvent *event)
{
    accept();
    QWidget::closeEvent(event);
}

//-----------------------------------------------------------------------------
/** @brief Quit the dialogue window.

This closes the dialogue window and restores the sleep setting if it was changed.
*/
void NodeConfigWidget::accept()
{
/* Emit the "terminated" signal with the row number so that the row status can
be restored */
    emit terminated(row);
}

//-----------------------------------------------------------------------------
/** @brief Send a Command to the Node.

This sends a user defined data message to the XBeee.
*/
void NodeConfigWidget::on_commandButton_clicked()
{
    QByteArray generalCommand;
    generalCommand.clear();
    generalCommand.append('D');
    generalCommand.append(NodeConfigFormUi.commandLine->text());
    int error = sendString(&generalCommand, tcpSocket, row, timeout);
    if (error > 0)
    {
        QMessageBox::warning(this,"","Error accessing remote node.");
#ifdef DEBUG
        qDebug() << "Error accessing remote node.";
#endif
    }
}

//-----------------------------------------------------------------------------
/** @brief Wake the node.

This sends a data message to the XBee to cause it to remain awake.
*/
void NodeConfigWidget::on_wakeButton_clicked()
{
/* Remote End Device nodes must be kept awake. Send an instruction to the AVR
to keep the XBee awake. */
    if (deviceType == 2)
    {
qDebug() << "XW Sent.";
        QByteArray stayAwakeCommand;
        stayAwakeCommand.clear();
        stayAwakeCommand.append("DXW");
        int error = sendString(&stayAwakeCommand, tcpSocket, row, timeout);
qDebug() << "XW OK.";
        if (error > 0)
        {
            QMessageBox::warning(this,"","Error accessing remote node.");
#ifdef DEBUG
            qDebug() << "Error accessing remote node.";
#endif
        }
        else
        {
            NodeConfigFormUi.nodeLabel->setText(nodeID);
        }
    }
}

//-----------------------------------------------------------------------------
/** @brief Set node to sleep.

This sends a data message to the XBee to cause it to return to sleep.
*/
void NodeConfigWidget::on_sleepButton_clicked()
{
    if (deviceType == 2)
    {
qDebug() << "XS Sent.";
        QByteArray sleepCommand;
        sleepCommand.clear();
        sleepCommand.append("DXS");
        int error = sendString(&sleepCommand, tcpSocket, row, timeout);
qDebug() << "XS OK.";
        if (error > 0)
        {
            QMessageBox::warning(this,"","Error accessing remote node.");
#ifdef DEBUG
            qDebug() << "Error accessing remote node.";
#endif
        }
    }
}

//-----------------------------------------------------------------------------
/** @brief Pull in the return data from the Internet Process.

This interprets the echoed commands and performs most of the processing.
*/
void NodeConfigWidget::readXbeeProcess()
{
    if (tcpSocket == 0) return;
    QByteArray reply = tcpSocket->readAll();
    int length = reply[0];
    char command = reply[1];
    int status = reply[2];
    while (length > reply.size())
    {
// Wait for any more data up to 5 seconds, then bomb out if none
        if (tcpSocket->waitForReadyRead(5000))
            reply.append(tcpSocket->readAll());
        else
        {
            setComStatus(comError);
            return;
        }
    }
#ifdef DEBUG
    if (reply.size() == 0) qDebug() << "Main sendCommand Null Response";
// Attempt to exclude responses representing nothing happening
    else if (!(((command == 'r') || (command == 'l') || (command == 's'))
                && (reply.size() == 3)))
    {
        qDebug() << "Main sendCommand response received: length" << length
                 << "Command" << command << "Status" << status;
        if (reply.size() > 3) qDebug() << "Contents" << reply.right(reply.size()-3).toHex();
    }
#endif
// The command sent should match the received echo unless it is a query response.
    if ((command != 'r') && (command != 'l') && (command != 's') && (command != comCommand))
    {
        setComStatus(comError);
        return;
    }
// Start command interpretation
    switch (command)
    {
/* Response to a sent data message. The response data is stored globally. */
        case 'E':
            response = status;
            break;
        case 'r':
            response = status;
            for (int i = 3; i < reply.size(); i++)
            {
                replyBuffer[i-3] = reply[i];
            }
            break;
        case 'l':
            response = status;
            for (int i = 3; i < reply.size(); i++)
            {
                replyBuffer[i-3] = reply[i];
            }
            break;
        case 's':
            response = status;
            if (response > 0)
            {
                for (int i=3; i<length; i++)
                {
                    dataReplyString[i-3] = reply[i];
                }
            }
            break;
        case 'I':
            int row = status;
            snH = convertNum(reply,5,4,16);
            snL = convertNum(reply,9,4,16);
            deviceType = reply[13];
            switch (deviceType)
            {
                case 0:
                    deviceTypeString = "Coordinator";
                    break;
                case 1:
                    deviceTypeString = "Router";
                    break;
                case 2:
                    deviceTypeString = "End Device";
                    break;
            }
            nodeID = (QString) reply.mid(15);
            break;
    }
    setComStatus(comReceived);
}

//-----------------------------------------------------------------------------
/** @brief Notify of connection failure.

This is a slot.
*/

void NodeConfigWidget::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, "XBee GUI",
                                 "The host was not found.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, "XBee GUI",
                                 "The connection was refused by the peer.");
        break;
    default:
        QMessageBox::information(this, QString("XBee GUI"),
                                 QString("The following error occurred: %1.")
                                 .arg(tcpSocket->errorString()));
    }
}

//-----------------------------------------------------------------------------
/** @brief Send an AT command over TCP connection and wait for response.

The command must be formatted as for the XBee AT commands. When a response is
checked, the result is returned in "response" with more detail provided in
"replyBuffer".

Globals: row, response, replyBuffer

@parameter  atCommand-> The AT command string as a QByteArray to send
@parameter  remote. True if the node is a remote node
@parameter  timeout. Number of 100ms delays in the wait loop.
@return 0 no error
        1 Invalid TCP socket.
        2 timeout error sending command
        3 timeout waiting for response from sending the initial command.
        4 timeout waiting for response from sending the response request command.
*/

int NodeConfigWidget::sendAtCommand(QByteArray *atCommand, QTcpSocket *tcpSocket,
                                    int row, bool remote, int timeout)
{
    if (remote)
    {
        atCommand->prepend(row);     // row
        atCommand->prepend('R');
    }
    else
    {
        atCommand->prepend('\0');     // Dummy "row" value
        atCommand->prepend('L');
    }
    comCommand = atCommand->at(0);
    int errorCode = sendCommand(atCommand,tcpSocket);
    if (errorCode == 0)
    {
/* Ask for a confirmation or error code */
        replyBuffer.clear();
        int count = 0;
        response = 0;
/* Query node every 100ms until response is received or an error occurs. */
        while((response == 0) && (errorCode == 0))
        {
            atCommand->clear();
            if (remote)
            {
                atCommand->append('r');
                atCommand->append(row);
            }
            else
            {
                atCommand->append('l');
                atCommand->append("\0");
            }
            errorCode = sendCommand(atCommand,tcpSocket);
            if ((timeout > 0) && (count++ > timeout)) errorCode = 4;  //Timeout
            else if (remote) ssleep(1);
        }
    }
    return errorCode;
}

//-----------------------------------------------------------------------------
/** @brief Send a String over TCP connection and wait for response.

The string is intended to be a command to the node MCU. When a response is
checked, the result is returned in "response" with more detail provided in
"replyBuffer".

Globals: row, response, replyBuffer

@parameter  command-> The string as a QByteArray to send
@parameter  remote. True if the node is a remote node
@parameter  timeout. Number of 100ms delays in the wait loop.
@return 0 no error
        1 Invalid TCP socket.
        2 timeout error sending command
        3 timeout waiting for response from sending the initial command.
        4 timeout waiting for response from sending the response request command.
*/

int NodeConfigWidget::sendString(QByteArray *command, QTcpSocket *tcpSocket,
                                    int row, int timeout)
{
    command->prepend(row);     // row
    command->prepend('S');
    comCommand = command->at(0);
    int errorCode = sendCommand(command,tcpSocket);
    if (errorCode == 0)
    {
/* Ask for a confirmation or error code */
        replyBuffer.clear();
        command->clear();
        command->append('s');
        command->append(row);
        int count = 0;
        response = 0;
/* Query node every 100ms until response is received or an error occurs. */
        while((response == 0) && (errorCode == 0))
        {
            errorCode = sendCommand(command,tcpSocket);
            if ((timeout > 0) && (count++ > timeout)) errorCode = 4;  //Timeout
            ssleep(1);
        }
    }
    return errorCode;
}

