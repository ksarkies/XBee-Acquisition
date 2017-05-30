/*       XBee Control and Display GUI Tool

This provides a GUI interface to an XBee coordinator process running on the
local or a remote Internet connected PC or Linux based controller for the XBee
acquisition network.
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
#include <QCloseEvent>
#include <QString>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QTextEdit>
#include <QStandardItemModel>
#include <QTableView>
#include <QtNetwork>
#include <QTcpSocket>
#include <QTextStream>
#include <QFile>
#include <QFileDialog>
#include <QDebug>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include "xbee-control.h"
#include "xbee-dialog.h"
#include "xbee-gui-libs.h"

// Local Prototypes
QString convertNum(const QByteArray response, const uchar startIndex,
                   const uchar length, const int base);

// Global data
QByteArray dataReplyMessage;

//-----------------------------------------------------------------------------
/** XBee-AP-Tool Constructor

@param[in] parent Parent widget.
*/

XbeeControlTool::XbeeControlTool(QString tcpAddress, uint tcpPort,
                                QWidget* parent) : QWidget(parent)
{
// Build the User Interface display from the Ui class in ui_mainwindowform.h
    XbeeControlFormUi.setupUi(this);
// Build the table view with basic details of each remote XBee
    table = new QStandardItemModel(20,5,this);
    table->setHorizontalHeaderItem(0, new QStandardItem(QString("Row")));
    table->setHorizontalHeaderItem(1, new QStandardItem(QString("Address 16")));
    table->setHorizontalHeaderItem(2, new QStandardItem(QString("SH")));
    table->setHorizontalHeaderItem(3, new QStandardItem(QString("SL")));
    table->setHorizontalHeaderItem(4, new QStandardItem(QString("Node Identifier")));
    table->setHorizontalHeaderItem(5, new QStandardItem(QString("Device Type")));
    table->setHorizontalHeaderItem(6, new QStandardItem(QString("Select")));
    XbeeControlFormUi.nodeTable->setModel(table);
    XbeeControlFormUi.nodeTable->resizeColumnToContents(0);
    XbeeControlFormUi.nodeTable->resizeColumnToContents(4);
    XbeeControlFormUi.nodeTable->resizeColumnToContents(5);
    XbeeControlFormUi.nodeTable->resizeColumnToContents(6);
// Additional widget settings
    XbeeControlFormUi.remoteConfigButton->setEnabled(false);
    XbeeControlFormUi.tcpConnectAddress->setText(tcpAddress);
    XbeeControlFormUi.tcpConnectPort->setValue(tcpPort);
    XbeeControlFormUi.uploadProgressBar->setVisible(false);
    tcpSocket = NULL;
// Set timeout to indefinite.
    timeout = 0;
    XbeeControlFormUi.waitTime->setValue(timeout);
}

XbeeControlTool::~XbeeControlTool()
{
    if (tcpSocket != NULL) tcpSocket->close();
}

//-----------------------------------------------------------------------------
/** @brief Attempt to connect to the coordinator process.

This opens a TCP socket for the specified address and port (58532) to connect to
a remote coordinator process listening on another machine or the same machine
(address 127.0.0.1). This remote process manages the XBee network.
*/

void XbeeControlTool::on_tcpConnectButton_clicked()
{
    if (tcpSocket != NULL) delete tcpSocket;
// Create the TCP socket to the internet process
    tcpSocket = new QTcpSocket(this);
// Setup QT signal/slots for reading and error handling
// The readyRead signal from the QAbstractSocket is linked to the readXbeeProcess slot
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readXbeeProcess()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
             this, SLOT(displayError(QAbstractSocket::SocketError)));
// Connect to the host
    blockSize = 0;
// Pull in all the information table
    QMessageBox msgBox;
    msgBox.open();
    msgBox.setText("Attempting to connect. Please wait.");
    int count;
    for (count=0; count<100;count++)
    {
        ssleep(1);
        tcpSocket->abort();
        tcpSocket->connectToHost(XbeeControlFormUi.tcpConnectAddress->text(), 
                                 XbeeControlFormUi.tcpConnectPort->value());
        if (tcpSocket->waitForConnected(1000)) break;
    }
    if (count >= 100) 
    {
#ifdef DEBUG
        qDebug() << "Timeout on connect attempt";
#endif
        QMessageBox::warning(this,"","Timeout while attempting to access coordinator process.");
    }
    msgBox.close();
    on_refreshListButton_clicked();
}

//-----------------------------------------------------------------------------
/** @brief Send commands to bring in node information table.

The node information table is managed in the remote coordinator process. It
contains information for nodes already known to be on the network, and adds new
nodes as they are connected.
*/

bool XbeeControlTool::on_refreshListButton_clicked()
{
// Delete all items in all rows ready for a refresh
    for (int row=0; row<tableLength; row++)
    {
        for (int column=0; column<7; column++)
        {
            delete table->takeItem(row,column);
            XbeeControlFormUi.nodeTable->setRowHidden(row,true);
        }
    }
// Add command and data characters to the array, then prepend with the length.
// Get the number of rows
    QByteArray numRowCommand;
    numRowCommand.append('N');
    comCommand = numRowCommand.at(0);
    sendCommand(&numRowCommand,tcpSocket);
// Ask for each row in turn
    int row;
    for (row = 0; row < tableLength; row++)
    {
        QByteArray rowGetCommand;
        rowGetCommand.append('I');
        rowGetCommand.append(char(row));
        comCommand = rowGetCommand.at(0);
        sendCommand(&rowGetCommand,tcpSocket);
    }
    return true;
}

//-----------------------------------------------------------------------------
/** @brief Remove a selected node.

If the remote checkbox is selected, find a selected row and delete the
corresponding node. The table must be refreshed otherwise it is out of sync.
*/

void XbeeControlTool::on_removeNodeButton_clicked()
{
    for (row = 0; row < tableLength; row++)
    {
        if (table->item(row,6)->checkState() == Qt::Checked) break;
    }
// If nothing is selected, quit.
    if (row == tableLength) return;
    QByteArray rowDeleteCommand;
    rowDeleteCommand.append('D');
    rowDeleteCommand.append(char(row));
    comCommand = rowDeleteCommand.at(0);
    sendCommand(&rowDeleteCommand,tcpSocket);
    on_refreshListButton_clicked();
}

//-----------------------------------------------------------------------------
/** @brief Attempt to reconnect with a remote node.

If a node is selected, reset the connections and send an SM command to check
that the device is present.
If no node selected, build an address from the edit box and try to connect.
If successful, add a new entry to the table.
*/

void XbeeControlTool::on_queryNodeButton_clicked()
{
    findNode();                     // Just test if a row is selected.
    if (row == tableLength)         // Nothing selected, so check editbox
    {
        QString nodeAddress;
        QString addressParm = XbeeControlFormUi.nodeAddress->text().toUpper().simplified();
        if (addressParm.size() == 0) return;    // No entry, so give up.
// A valid address will be 16 characters and each will be a hex character
        bool validAddress = (addressParm.size() == 16);
        if (validAddress)
        {
            for (int i=0; i<16; i++)
            {
                char hexchar = addressParm.at(i).toLatin1();
                if (! (((hexchar - '0') <= 9) || ((hexchar - 'A') <= 5)))
                    validAddress = false;
            }
        }
// If not a valid address, try as a node name. This might be picked
// up from the local XBee or a router if it is known, so it can come quickly.
        if (! validAddress)
        {
            QByteArray commandDN;
            commandDN.clear();
            commandDN.append("DN" + addressParm);
            if (sendAtCommand(&commandDN, tcpSocket, row, false, timeout) != 0)
            {
                QMessageBox::warning(this,"","Unable to find named node");
#ifdef DEBUG
                qDebug() << "Timeout waiting for DN request response";
#endif
                return;
            }
            nodeAddress = convertNum(replyBuffer,2,8,16);
            XbeeControlFormUi.nodeAddress->setText(nodeAddress);
        }
        else
        {
            nodeAddress = addressParm;
        }
// Now we seem to have a valid node address. Send an E command to add it to the
// node table.
        bool ok;
        QByteArray newNodeCommand;
        newNodeCommand.append('E');
        newNodeCommand.append('\0');    // Dummy node
        for (int i=0; i<16; i+=2)
        {
            newNodeCommand.append(nodeAddress.mid(i,2).toUShort(&ok,16));
        }
        comCommand = newNodeCommand.at(0);
        sendCommand(&newNodeCommand,tcpSocket);
        if (response == 'N')
        {
            QMessageBox::warning(this,"","Node exists or too many nodes");
            return;
        }
// Get the number of rows. The one we want is the last.
        QByteArray numRowCommand;
        numRowCommand.append('N');
        comCommand = numRowCommand.at(0);
        sendCommand(&numRowCommand,tcpSocket);
        row = tableLength-1;
        QByteArray comPbCommand;
        comPbCommand.clear();
        comPbCommand.append("CB");
        comPbCommand.append('\1');
// Wait a bit as we may have a sleeping device
        if (sendAtCommand(&comPbCommand, tcpSocket, row, true, timeout) > 0)
        {
#ifdef DEBUG
            qDebug() << "Timeout accessing remote node while requesting number of nodes";
#endif
        }
        else
            on_refreshListButton_clicked();
    }
    else
    {
// Node is mentioned in the table
// Query sleep mode.
        QByteArray reconnectCommand;
        reconnectCommand.append('Q');
        reconnectCommand.append(char(row));
        comCommand = reconnectCommand.at(0);
        sendCommand(&reconnectCommand,tcpSocket);
        QByteArray sleepModeCommand;
        sleepModeCommand.clear();
        sleepModeCommand.append("SM");
        int error = sendAtCommand(&sleepModeCommand, tcpSocket, row, true, timeout);
        if (error > 0)
        {
            QMessageBox::warning(this,"","Error accessing remote node while querying sleep mode.");
#ifdef DEBUG
            qDebug() << "Error accessing remote node while querying sleep mode";
#endif
        }
        else
        {
            table->item(row,6)->setCheckState(Qt::Unchecked);
            XbeeControlFormUi.nodeAddress->setText(table->item(row,4)->text());
        }
    }
}

//-----------------------------------------------------------------------------
/** @brief Start loading AVR firmware.

This assumes the existence of an on-board bootloader in the AVR.

Open a selected file with the new firmware, reset the AVR and download the iHex
lines.

@returns int: error number. 0=OK, 1=erase fail, 2=upload fail, 3=command fail.
*/

int XbeeControlTool::on_firmwareButton_clicked()
{
    QString errorMessage;
/* Open file for reading */
    bool error = false;
/* Get the selected row (actually gets the first one selected */
    int row = 0;
    for (row = 0; row < tableLength; row++)
    {
        if (table->item(row,6)->checkState() == Qt::Checked) break;
    }
    if (row == tableLength)
    {
        QMessageBox::warning(this,"","No Row Selected");
        return false;
    }
// Get filename of hex format to load
    QString filename = QFileDialog::getOpenFileName(this,
                                        "Intel Hex File to Upload",
                                        "./",
                                        "Intel Hex Files (*.hex)");
    if (! filename.isEmpty())
    {
        QFileInfo fileInfo(filename);
        QFile file(filename);
        if (file.open(QIODevice::ReadOnly))
        {
/* Read file and transmit to node */
            error = loadHexGUI(&file, row);
        }
    }
    if (error == 0)  return 0;
    if (error == 1) errorMessage = "Timeout waiting for erase to complete";
    if (error == 2) errorMessage = "Timeout waiting for a sent line to program";
    if (error == 3) errorMessage = "General Command Timeout";
    QMessageBox::warning(this,"",errorMessage);
    return error;
}

//-----------------------------------------------------------------------------
/** @brief Pull in the return data from the Internet Process.

This interprets the echoed commands and performs most of the processing.
If no response within a time limit, returns a communication error code.

The incoming commands consist of a message length, ASCII command and a status.
The status may also be a part of the returned data.

This is a QT slot.
*/

void XbeeControlTool::readXbeeProcess()
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
                    dataReplyMessage[i-3] = reply[i];
                }
            }
            break;
        case 'N':
            tableLength = status;
            break;
        case 'I':
            int row = status;
            QStandardItem *rowItem = new QStandardItem(convertNum(reply,2,1,16));
            table->setItem(row,0,rowItem);
            QStandardItem *adrItem = new QStandardItem(convertNum(reply,3,2,16));
            table->setItem(row,1,adrItem);
            QStandardItem *shItem = new QStandardItem(convertNum(reply,5,4,16));
            table->setItem(row,2,shItem);
            QStandardItem *slItem = new QStandardItem(convertNum(reply,9,4,16));
            table->setItem(row,3,slItem);
            QString deviceTypeString;
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
            QStandardItem *devTypeItem = new QStandardItem(deviceTypeString);
            table->setItem(row,5,devTypeItem);
            QStandardItem *nodeIdItem = new QStandardItem((QString) reply.mid(15));
            table->setItem(row,4,nodeIdItem);
            QStandardItem *checkItem = new QStandardItem();
            table->setItem(row,6,checkItem);
            checkItem->setEnabled(true);
            checkItem->setCheckable(true);
            checkItem->setCheckState(Qt::Unchecked);
            if ((int)reply[14] == 0)
            {
                checkItem->setText("*");
            }
            XbeeControlFormUi.nodeTable->setRowHidden(row,false);
            break;
    }
    setComStatus(comReceived);
}

//-----------------------------------------------------------------------------
/** @brief Notify of connection failure.

This is a slot.
*/

void XbeeControlTool::displayError(QAbstractSocket::SocketError socketError)
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
/** @brief Open a dialogue box for configuring an XBee.

If the remote checkbox is selected, find a selected row and use the
corresponding node. If no node is selected, use the coordinator XBee to
configure.
*/

void XbeeControlTool::on_configButton_clicked()
{
    findNode();
    bool remote = (row < tableLength);
/* If nothing is selected, will go to the coordinator XBee. Row has no meaning
in this case. */
    QString tcpAddress = XbeeControlFormUi.tcpConnectAddress->text();
    uint tcpPort = XbeeControlFormUi.tcpConnectPort->value();
    XBeeConfigWidget* XBeeConfigWidgetForm =
                new XBeeConfigWidget(tcpAddress,tcpPort,row,remote,timeout,0);
    connect(XBeeConfigWidgetForm, SIGNAL(terminated(int)),this,SLOT(configDialogDone(int)));
    XBeeConfigWidgetForm->show();
    if (remote)
    {
        table->item(row,6)->setEnabled(false);
        table->item(row,6)->setCheckState(Qt::Unchecked);
    }
}

//-----------------------------------------------------------------------------
/** @brief  Catch a signal from the dialogue when it is closed, and restore the
table item for re-use */
void XbeeControlTool::configDialogDone(int row)
{
    if (row < tableLength)
    {
        table->item(row,6)->setEnabled(true);
        table->item(row,6)->setCheckState(Qt::Unchecked);
    }
}

//-----------------------------------------------------------------------------
/** @brief Find a selected row if any, and check the node responds.

The rows are searched to find one that is selected, then the node is queried.
If no row is selected, this just returns.

Globals: tableLength, row

@returns true if the node responds.
*/

bool XbeeControlTool::findNode()
{
/* Search for a box checked. If none, row will equal tableLength */
    for (row = 0; row < tableLength; row++)
    {
        if (table->item(row,6)->checkState() == Qt::Checked) break;
    }
    if (row == tableLength) return false;
    return true;
}

//-----------------------------------------------------------------------------
/** @brief Contact node and set to stay awake.

The node selected by the row is contacted and commanded to change to a stay
awake mode for further instructions.

Globals: timeout

@returns    bool: false if unable to be contacted.
*/

bool XbeeControlTool::setNodeAwake()
{
/* First probe for the node - use a sleep mode query but anything would do.
Note that the row used is a global and is accessed by the sendAtCommand function. */
    QMessageBox msgBox;
    msgBox.open();
    msgBox.setText("Node being contacted. Please wait.");
    qApp->processEvents();
    QByteArray stayAwakeCommand;
    stayAwakeCommand.clear();
    stayAwakeCommand.append("DW");
    if (sendAtCommand(&stayAwakeCommand, tcpSocket, row, true, timeout) > 0)
    {
#ifdef DEBUG
        qDebug() << "setNodeAwake: Timeout accessing remote node.";
#endif
        QMessageBox::warning(this,"","Timeout accessing remote node.");
        return false;
    }
    msgBox.close();
    return true;
}

//-----------------------------------------------------------------------------
/** @brief Check if a socket is valid, that is, connected.
*/

bool XbeeControlTool::validTcpSocket()
{
    if (tcpSocket == NULL)
    {
        QMessageBox::warning(this,"","Not yet connected. Please click connect button.");
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
/** @brief Successful Setup

@returns TRUE if the device gave no error
*/

bool XbeeControlTool::success()
{
    return (errorMessage.size() == 0);
}

//-----------------------------------------------------------------------------
/** @brief Error Message

@returns a message when the device didn't respond.
*/

QString XbeeControlTool::error()
{
    return errorMessage;
}

//-----------------------------------------------------------------------------
/** @brief Close off the window and deallocate resources

This may not be necessary as QT may implement it implicitly.
*/

void XbeeControlTool::closeEvent(QCloseEvent *event)
{
    event->accept();
}

//-----------------------------------------------------------------------------
/** @brief Convert section of byte string to QString number

*/

QString convertNum(const QByteArray response, const uchar startIndex,
                   const uchar length, const int base)
{
    QString commandData = "";
    for (uchar i = startIndex; i < length+startIndex; i++)
        commandData += QString("%1").arg(QString::number(response[i],base)
                                .toUpper().right(2),2,'0');
    return commandData;
}

//-----------------------------------------------------------------------------
/** @brief Send an AT command over TCP connection and wait for response.

The command must be formatted as for the XBee AT commands. A response is
expected from the addressed base station and this is awaited with a
timeout (which may be indefinite).

When a response is checked, the result is returned in "response" with more
detail provided in "replyBuffer".

Globals: row, response, replyBuffer

@parameter[in]  atCommand. The AT command string as a QByteArray to send.
@parameter[in]  remote. True if the node is a remote node
@parameter[in]  timeout. Number of 100ms delays in the wait loop, or 0 for indefinite.
@return 0 no error
        1 Invalid TCP socket.
        2 timeout error sending command
        3 timeout waiting for response from sending the initial command.
        4 timeout waiting for response from sending the response request command.
*/

int XbeeControlTool::sendAtCommand(const QByteArray *xbeeCommand, QTcpSocket *tcpSocket,
                                   const int row, const bool remote, const int timeout)
{
    QByteArray atCommand = *xbeeCommand;
    if (remote)
    {
        atCommand.prepend(row);     // row
        atCommand.prepend('R');
    }
    else
    {
        atCommand.prepend('\0');     // Dummy "row" value
        atCommand.prepend('L');
    }
    comCommand = atCommand.at(0);
    int errorCode = sendCommand(&atCommand,tcpSocket);
    if (errorCode == 0)
    {
/* Ask for a confirmation or error code */
        replyBuffer.clear();
        atCommand.clear();
        if (remote)
        {
            atCommand.append('r');
            atCommand.append(row);
        }
        else
        {
            atCommand.append('l');
            atCommand.append("\0");
        }
        int count = 0;
        response = 0;
/* Query node every 100ms until response is received or an error occurs. */
        while((response == 0) && (errorCode == 0))
        {
            errorCode = sendCommand(&atCommand,tcpSocket);
            if ((timeout > 0) && (count++ > timeout)) errorCode = 4;  //Timeout
            else if (remote) ssleep(1);
        }
    }
    return errorCode;
}

//-----------------------------------------------------------------------------
/** @brief Load a .hex file to either Flash or EEPROM with GUI feedback

This sets up the progress bar and calls the loadHex method to do the actual GUI
independent loading.

The remote XBee uses ports DIO12 and DIO11 to reset and set the bootloader jump
to application signal. A 0.5s delay is used to wait for the reset is complete.
Then the entire memory is erased. Each line is then sent and a response awaited.
If nothing comes back in a reasonable time, the line is resent. If the AVR is
not present or responding the retransmits are limited.

@param[in]  QFile file: already opened for loading.
@param[in]  int row:  Table row number for remote node to address.
@returns    int error code: 0 if OK, 1 if erase timeout, 2 if resend program
            line timeout, 3 if send command error.
*/

int XbeeControlTool::loadHexGUI(QFile* file, int row)
{
    unsigned int errorCode = 0;
    int failPoint = 0;
    char oldSleepMode = 0;
    if (! findNode()) return 3;     // Check that we can communicate, and sync
// To get the device type, ask for the node details
    QByteArray rowGetCommand;
    rowGetCommand.append('I');
    rowGetCommand.append(char(row));
    comCommand = rowGetCommand.at(0);
    sendCommand(&rowGetCommand,tcpSocket);
#ifdef DEBUG
    qDebug() << "Device type" << (int)deviceType;
#endif
// Change sleep mode on end device only; coordinator and routers never sleep
    if (deviceType == 2)
    {
// Keep old value.
        oldSleepMode = replyBuffer[0];
#ifdef DEBUG
        qDebug() << "Original sleep mode" << (int)oldSleepMode << "changing to pin sleep";
#endif
// Set the sleep mode to 1 for the duration
        QByteArray sleepModeCommand;
        sleepModeCommand.clear();
        sleepModeCommand.append("SM");
        sleepModeCommand.append("\1");
        if (sendAtCommand(&sleepModeCommand, tcpSocket, row, true,3000) > 0)
        {
#ifdef DEBUG
            qDebug() << "Timeout setting remote node sleep mode";
#endif
            return 3;
        }
    }
    QByteArray firmwareCommand;
/* Start by resetting the node into bootloader mode */
/* Clear the DIO11 port on the XBee (bootloader pin) */
    firmwareCommand.clear();
    firmwareCommand.append('R');
    firmwareCommand.append(char(row));
    firmwareCommand.append("P1");
    firmwareCommand.append(4);
    comCommand = firmwareCommand.at(0);
    errorCode = sendCommand(&firmwareCommand,tcpSocket);
    if (errorCode > 0) failPoint = 1;
/* Clear the DIO12 port on the XBee (AVR Reset) */
    firmwareCommand.clear();
    firmwareCommand.append('R');
    firmwareCommand.append(char(row));
    firmwareCommand.append("P2");
    firmwareCommand.append(4);
    comCommand = firmwareCommand.at(0);
    errorCode = sendCommand(&firmwareCommand,tcpSocket);
    if (errorCode > 0) failPoint = 2;
    millisleep(500);                 // Wait a bit for reset to complete
/* Set the DIO12 port on the XBee (AVR Reset) */
    firmwareCommand.clear();
    firmwareCommand.append('R');
    firmwareCommand.append(char(row));
    firmwareCommand.append("P2");
    firmwareCommand.append(5);
    comCommand = firmwareCommand.at(0);
    errorCode = sendCommand(&firmwareCommand,tcpSocket);
    if (errorCode > 0) failPoint = 3;
/* Setup the progress bar */
    XbeeControlFormUi.uploadProgressBar->setVisible(true);
    XbeeControlFormUi.uploadProgressBar->setMinimum(0);
    XbeeControlFormUi.uploadProgressBar->setMaximum(file->size());
    XbeeControlFormUi.uploadProgressBar->setValue(0);
    firmwareCommand.clear();
    firmwareCommand.append('S');
    firmwareCommand.append(char(row));
/* Clear all FLASH */
    response = 0;
    firmwareCommand.append('X');
    comCommand = firmwareCommand.at(0);
    errorCode = sendCommand(&firmwareCommand,tcpSocket);
    if (errorCode > 0) failPoint = 4;
/* Ask for a confirmation or error code, loop until something received
or abort if nothing comes back in a reasonable time. */
    int count = 0;
    while (response == 0)
    {
        qApp->processEvents();              // Keep GUI ticking
        if (count++ > 20)
        {
            failPoint = 5;
            break;
        }
        millisleep(100);
        firmwareCommand.clear();
        firmwareCommand.append('s');
        firmwareCommand.append(char(row));
        comCommand = firmwareCommand.at(0);
        errorCode = sendCommand(&firmwareCommand,tcpSocket);
    }
    if (failPoint == 0)
    {
/* Open file stream, read each line, test and send off to target */
        QTextStream stream(file);
        bool verifyOK = true;
        int progress = 0;
        while (! stream.atEnd() && verifyOK && (failPoint == 0))
        {
            qApp->processEvents();
            bool timeout = false;
          	QString line = stream.readLine();
            verifyOK = (line[0] == ':');
            if (verifyOK)
            {
                int resendTimer = 0;
// Send the line and wait for a response. If it times out, try resending the
// line up to three times, then leave with an error.
                do
                {
#ifdef DEBUG
                    qDebug() << "Firmware iHex sent:" << line;
#endif
/* Send the line as a data communication */
                    firmwareCommand.clear();
                    firmwareCommand.append('S');
                    firmwareCommand.append(char(row));
                    firmwareCommand.append(line);
                    comCommand = firmwareCommand.at(0);
                    errorCode = sendCommand(&firmwareCommand,tcpSocket);
                    if (errorCode > 0)
                    {
                        failPoint = 6;
                        break;
                    }
                    response = 0;
/* Ask for a confirmation or error code */
                    int count = 0;
                    while((response == 0) && (count++ < 5))
                    {
                        millisleep(100);
                        firmwareCommand.clear();
                        firmwareCommand.append('s');
                        firmwareCommand.append(char(row));
                        comCommand = firmwareCommand.at(0);
                        errorCode = sendCommand(&firmwareCommand,tcpSocket);
                    }
                    timeout = (count > 4);
#ifdef DEBUG
                    if (timeout) qDebug() << "Timed out. Retry.";
#endif
                    if (resendTimer++ > 3)
                    {
                        failPoint = 7;
                        break;
                    }
                }
                while (timeout);                 // retry entire message if timed out
            }
            progress += line.length();
            XbeeControlFormUi.uploadProgressBar->setValue(progress);
            qApp->processEvents();
        }
    }
/* Remove bootloader signal and reset again. Node will enter application mode */
/* Set the DIO11 port on the XBee (back to application) */
    firmwareCommand.clear();
    firmwareCommand.append('R');
    firmwareCommand.append(char(row));
    firmwareCommand.append("P1");
    firmwareCommand.append(5);
    comCommand = firmwareCommand.at(0);
    errorCode = sendCommand(&firmwareCommand,tcpSocket);
    if (errorCode > 0) failPoint = 6;
/* Clear the DIO12 port on the XBee (AVR Reset) */
    firmwareCommand.clear();
    firmwareCommand.append('R');
    firmwareCommand.append(char(row));
    firmwareCommand.append("P2");
    firmwareCommand.append(4);
    comCommand = firmwareCommand.at(0);
    errorCode = sendCommand(&firmwareCommand,tcpSocket);
    if (errorCode > 0) failPoint = 7;
    millisleep(500);                 // Wait a bit for reset to complete
/* Set the DIO12 port on the XBee (AVR Reset) */
    firmwareCommand.clear();
    firmwareCommand.append('R');
    firmwareCommand.append(char(row));
    firmwareCommand.append("P2");
    firmwareCommand.append(5);
    comCommand = firmwareCommand.at(0);
    errorCode = sendCommand(&firmwareCommand,tcpSocket);
    if (errorCode > 0) failPoint = 8;
/* Turn off progress bar when ended */
    XbeeControlFormUi.uploadProgressBar->setValue(file->size());
    XbeeControlFormUi.uploadProgressBar->setVisible(false);
#ifdef DEBUG
    if ((errorCode > 0) || (failPoint > 0)) qDebug() << "Error in firmware: Code"
                         << errorCode << "Failure Point" << failPoint;
#endif
    if ((errorCode > 0) || (failPoint > 0))
        QMessageBox::warning(this,"",QString("Upload Failure: error code %1").arg(failPoint));
// Set the sleep mode back to original for end device only
    if (deviceType == 2)
    {
        QByteArray sleepModeCommand;
        sleepModeCommand.clear();
        sleepModeCommand.append("SM");
        sleepModeCommand.append(oldSleepMode);
        if (sendAtCommand(&sleepModeCommand, tcpSocket, row, true,3000) > 0)
        {
#ifdef DEBUG
            qDebug() << "Timeout reloading remote node sleep mode";
#endif
        }
    }
    return (errorCode);
}

//-----------------------------------------------------------------------------

