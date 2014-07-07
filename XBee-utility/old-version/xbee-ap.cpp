/*       XBee AP Mode Command Tool
*/
/****************************************************************************
 *   Copyright (C) 2007 by Ken Sarkies                                      *
 *   ksarkies@trinity.asn.au                                                *
 *                                                                          *
 *   This file is part of xbee-ap-tool                                      *
 *                                                                          *
 *   xbee-ap-tool is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU General Public License as         *
 *   published by the Free Software Foundation; either version 2 of the     *
 *   License, or (at your option) any later version.                        *
 *                                                                          *
 *   xbee-ap-tool is distributed in the hope that it will be useful,        *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with xbee-ap-tool if not, write to the                           *
 *   Free Software Foundation, Inc.,                                        *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.              *
 ***************************************************************************/

#include "xbeep.h"
#include "xbee-ap.h"
#include "serialport.h"
#include <QApplication>
#include <QString>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QTextEdit>
#include <QCloseEvent>
#include <QDebug>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

// Local Prototypes
QString convertNum(const QString response, const uchar startIndex, const uchar length, const int base);

//-----------------------------------------------------------------------------
/** XBee-AP-Tool Constructor

@param[in] p Serial Port object pointer
@param[in] parent Parent widget.
*/

XbeeApTool::XbeeApTool(SerialPort* p, QWidget* parent) : QDialog(parent)
{
    port = p;
    synchronized =false;
    baudrate = 5;
    line = 0;
// Build the User Interface display from the Ui class in ui_mainwindowform.h
    XbeeApFormUi.setupUi(this);
// Setup BaudRate selector
    XbeeApFormUi.baudRate->addItem("2400");
    XbeeApFormUi.baudRate->addItem("4800");
    XbeeApFormUi.baudRate->addItem("9600");
    XbeeApFormUi.baudRate->addItem("19200");
    XbeeApFormUi.baudRate->addItem("38400");
    XbeeApFormUi.baudRate->addItem("57600");
    XbeeApFormUi.baudRate->addItem("115200");
    XbeeApFormUi.baudRate->setCurrentIndex(baudrate);
    XbeeApFormUi.commandEntry->setFocus();
// Initialise serial port first
    if (port->initPort(baudrate,100))
    {
        synchronized = true;
    }
    else
    {
        errorMessage = QString("Unable to access the serial port\n"
                    "Check the connections and power.");
        return;
    }
// Hide all invisible widgets
    hideWidgets();
// Setup the AP Command selector    
    XbeeApFormUi.atCommandList->addItem("ND Node Discovery");
    XbeeApFormUi.atCommandList->addItem("MY 16-bit Network Addr");
    XbeeApFormUi.atCommandList->addItem("NC Remaining Children");
    XbeeApFormUi.atCommandList->addItem("NI Node Identifier");
    XbeeApFormUi.atCommandList->addItem("NP RF Payload Bytes");
    XbeeApFormUi.atCommandList->addItem("DB Last Received SS");
    XbeeApFormUi.atCommandList->addItem("CH Channel Used");
    XbeeApFormUi.atCommandList->addItem("ID Extended PAN ID");
    XbeeApFormUi.atCommandList->addItem("DD Device Type");
    XbeeApFormUi.atCommandList->addItem("NO Node Discovery Opts");
    XbeeApFormUi.atCommandList->addItem("SC Scan Channel List");
    XbeeApFormUi.atCommandList->addItem("PP Peak Power");
    XbeeApFormUi.atCommandList->addItem("PL Peak Level");
    XbeeApFormUi.atCommandList->addItem("DN Destination Node");
    XbeeApFormUi.atCommandList->addItem("SH S/N High");
    XbeeApFormUi.atCommandList->addItem("SL S/N Low");

// Set up a default packet
    on_atCommandList_currentIndexChanged(0);
}

XbeeApTool::~XbeeApTool()
{
    port->close();
}

//-----------------------------------------------------------------------------
/** @brief Successful synchronization

@returns TRUE if the device responded
*/
bool XbeeApTool::success()
{
    return synchronized;                    //synchronized;
}

//-----------------------------------------------------------------------------
/** @brief Hide all Invisible Widgets

*/
void XbeeApTool::hideWidgets()
{
    XbeeApFormUi.frameType->setVisible(false);
    XbeeApFormUi.frameType->setText("");
    XbeeApFormUi.frameTypeLabel->setVisible(false);
    XbeeApFormUi.frameID->setVisible(false);
    XbeeApFormUi.frameID->setText("");
    XbeeApFormUi.frameIDLabel->setVisible(false);
    XbeeApFormUi.atCommand->setVisible(false);
    XbeeApFormUi.atCommand->setText("");
    XbeeApFormUi.atCommandLabel->setVisible(false);
    XbeeApFormUi.commandStatus->setVisible(false);
    XbeeApFormUi.commandStatus->setText("");
    XbeeApFormUi.commandStatusLabel->setVisible(false);
    XbeeApFormUi.commandDataLabel->setVisible(false);
    XbeeApFormUi.commandData_1->setVisible(false);
    XbeeApFormUi.commandDataLabel_1->setVisible(false);
    XbeeApFormUi.commandData_2->setVisible(false);
    XbeeApFormUi.commandDataLabel_2->setVisible(false);
    XbeeApFormUi.commandData_3->setVisible(false);
    XbeeApFormUi.commandDataLabel_3->setVisible(false);
    XbeeApFormUi.commandData_4->setVisible(false);
    XbeeApFormUi.commandDataLabel_4->setVisible(false);
    XbeeApFormUi.commandData_5->setVisible(false);
    XbeeApFormUi.commandDataLabel_5->setVisible(false);
    XbeeApFormUi.commandData_6->setVisible(false);
    XbeeApFormUi.commandDataLabel_6->setVisible(false);
    XbeeApFormUi.commandData_7->setVisible(false);
    XbeeApFormUi.commandDataLabel_7->setVisible(false);
    XbeeApFormUi.commandData_8->setVisible(false);
    XbeeApFormUi.commandDataLabel_8->setVisible(false);
    XbeeApFormUi.atCommandBuffered->setChecked(false);
    XbeeApFormUi.atCommandSet->setVisible(false);
    XbeeApFormUi.atCommandBinOpt_1->setVisible(false);
    XbeeApFormUi.atCommandBinOpt_2->setVisible(false);
    XbeeApFormUi.atCommandSet->setChecked(false);
    XbeeApFormUi.atCommandBinOpt_1->setChecked(false);
    XbeeApFormUi.atCommandBinOpt_2->setChecked(false);
}

//-----------------------------------------------------------------------------
/** @brief Error Message

@returns a message when the device didn't respond.
*/
QString XbeeApTool::error()
{
    return errorMessage;
}

//-----------------------------------------------------------------------------
/** @brief Setup Options when AT dropbox is changed

*/
void XbeeApTool::on_atCommandList_currentIndexChanged(int atCommand)
{
    hideWidgets();
    switch(atCommand)
    {
        case 9:             // Node Discovery Options command
            XbeeApFormUi.atCommandSet->setVisible(true);
            XbeeApFormUi.atCommandSet->setChecked(false);
            XbeeApFormUi.atCommandBinOpt_1->setVisible(true);
            XbeeApFormUi.atCommandBinOpt_1->setText("Send Device ID");
            XbeeApFormUi.atCommandBinOpt_2->setVisible(true);
            XbeeApFormUi.atCommandBinOpt_2->setText("Send Local Results");
            break;
    }
}
//-----------------------------------------------------------------------------
/** @brief Build command when User has completed settings

*/
void XbeeApTool::buildPacket()
{
    uint index = 0;
    uchar checksum = 0xFF;
    uchar sendOptions;
    QString parameter = XbeeApFormUi.commandEntry->text().toUpper();
// Start building the binary packet
    packet[0] = 0x7E;
    packet[3] = 0x08;       // AT command has the Frame type 8 or 9 (buffered)
    if (XbeeApFormUi.atCommandBuffered->isChecked())
        packet[3] = 0x09;
    packet[4] = frameID++;  // Frame ID field incremented to ensure uniqueness
    length = 0x04;          // Minimum length to be incremented
// Setup packet from user entered data
    switch(XbeeApFormUi.atCommandList->currentIndex())
    {
        case 0:             // Node Discovery Command
            packet[5] = 'N';
            packet[6] = 'D';
            break;
        case 1:             // 16-bit Network Address command
            packet[5] = 'M';
            packet[6] = 'Y';
            break;
        case 2:             // Number of Children Remaining command
            packet[5] = 'N';
            packet[6] = 'C';
            break;
        case 3:             // Node Identifier command
            packet[5] = 'N';
            packet[6] = 'I';
            break;
        case 4:             // Number of Payload Bytes command
            packet[5] = 'N';
            packet[6] = 'P';
            break;
        case 5:             // Last Received Power Level command
            packet[5] = 'D';
            packet[6] = 'B';
            break;
        case 6:             // 802.15.4 Channel command
            packet[5] = 'C';
            packet[6] = 'H';
            break;
        case 7:             // Extended 64-bit PAN ID command
            packet[5] = 'I';
            packet[6] = 'D';
            break;
        case 8:             // Device Type command
            packet[5] = 'D';
            packet[6] = 'D';
            break;
        case 9:             // Node Discovery Options command
            packet[5] = 'N';
            packet[6] = 'O';
            sendOptions = (XbeeApFormUi.atCommandBinOpt_1->isChecked() ? 1 : 0)
                                 + ((XbeeApFormUi.atCommandBinOpt_2->isChecked() ? 1 : 0) << 1);
            if (XbeeApFormUi.atCommandSet->isChecked())
            {
                packet[3+length++] = sendOptions;
            }
            break;
        case 10:             // Scan channel list command
            packet[5] = 'S';
            packet[6] = 'C';
            break;
        case 11:             // Peak Power command
            packet[5] = 'P';
            packet[6] = 'P';
            break;
        case 12:             // Power Level command
            packet[5] = 'P';
            packet[6] = 'L';
            break;
        case 13:             // Destination Node command
            packet[5] = 'D';
            packet[6] = 'N';
            for (uchar i = 0; i < parameter.size(); i++)
            {
                packet[3+length++] = parameter[i].toAscii();
            }
            qDebug() << parameter << length;
            break;
        case 14:             // S/N High command
            packet[5] = 'S';
            packet[6] = 'H';
            break;
        case 15:             // S/N Lowl command
            packet[5] = 'S';
            packet[6] = 'L';
            break;
    }
    packet[1] = (length >> 8);
    packet[2] = (length & 0xFF);
    for (index = 0; index < length; index++)
        checksum -= packet[3+index];
    packet[3+length] = checksum;
    for (uint i = 0; i < length+3; ++i)
    {
        qDebug() << QString("%1").arg(packet[i],2,16);
    }
}
//-----------------------------------------------------------------------------
/** @brief Interpret and send off command string

After sending, the received characters are read in and displayed.
No CRC check.
*/
void XbeeApTool::on_sendButton_clicked()
{
    buildPacket();
    uint index = 0;
    QString commandData;
// send off the packet
    for (uint i = 0; i < length+4; ++i)
        port->putChar(packet[i]);       // Send the interpreted string
// Read back the response and interpret
    QString response = getSerialData();
    uint responseSize = response.size();
    for (uint i = 0; i < responseSize; ++i)
    {
        qDebug() << QString::number(response[i].toAscii(),16).toUpper().right(2);
    }
// Look for the start delimiter
    for (index = 0; index < responseSize; index++)
        if (response[index].toAscii() == 0x7E) break;
    if (index+2 < responseSize)         // Check if length was received
        length = ((uchar)response[index+1].toAscii() << 8) + (uchar)response[index+2].toAscii();
    else return;
// At this point we check the command returned and display fields
// Frame type is always present
    XbeeApFormUi.frameTypeLabel->setVisible(true);
    XbeeApFormUi.frameType->setVisible(true);
// Find the frame type to start with
    uchar frameType = response[index+3].toAscii();
    XbeeApFormUi.frameType->setText(QString::number(frameType,16).toUpper().right(2));
// Display AT Command response preamble
    if (frameType == 0x88)
    {
        XbeeApFormUi.frameID->setVisible(true);
        XbeeApFormUi.frameIDLabel->setVisible(true);
        XbeeApFormUi.atCommand->setVisible(true);
        XbeeApFormUi.atCommandLabel->setVisible(true);
        XbeeApFormUi.commandStatus->setVisible(true);
        XbeeApFormUi.commandStatusLabel->setVisible(true);
// AT Command response preamble is always displayed
        XbeeApFormUi.frameID->setText(QString::number(response[index+4].toAscii(),16).toUpper().right(2));
        QString atResponseCommand = QString(response[index+5])+QString(response[index+6]);
        XbeeApFormUi.atCommand->setText(atResponseCommand);
        uchar commandStatus = response[index+7].toAscii();
        XbeeApFormUi.commandStatus->setText((commandStatus == 0) ? "OK" : "Err");
// The Node Discovery response
        if (atResponseCommand == "ND")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Node Discovery Response");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("Serial number");
            XbeeApFormUi.commandData_2->setVisible(true);
            XbeeApFormUi.commandDataLabel_2->setVisible(true);
            XbeeApFormUi.commandDataLabel_2->setText("Node Identifier");
            XbeeApFormUi.commandData_3->setVisible(true);
            XbeeApFormUi.commandDataLabel_3->setVisible(true);
            XbeeApFormUi.commandDataLabel_3->setText("Parent Address");
            XbeeApFormUi.commandData_4->setVisible(true);
            XbeeApFormUi.commandDataLabel_4->setVisible(true);
            XbeeApFormUi.commandDataLabel_4->setText("Device Type");
            XbeeApFormUi.commandData_5->setVisible(true);
            XbeeApFormUi.commandDataLabel_5->setVisible(true);
            XbeeApFormUi.commandDataLabel_5->setText("Status");
            XbeeApFormUi.commandData_6->setVisible(true);
            XbeeApFormUi.commandDataLabel_6->setVisible(true);
            XbeeApFormUi.commandDataLabel_6->setText("Profile ID");
            XbeeApFormUi.commandData_7->setVisible(true);
            XbeeApFormUi.commandDataLabel_7->setVisible(true);
            XbeeApFormUi.commandDataLabel_7->setText("Mfgr ID");
// ND data field breakdown
            uint i = 8;                  // Start of data block
            commandData = "?("+convertNum(response,i,6,16)+") ";
            i += 6;
            commandData += convertNum(response,i,4,16);
            i += 4;
            XbeeApFormUi.commandData_1->setText(commandData);
            for (commandData = ""; response[i] != 0; i++) commandData += response[i];
            XbeeApFormUi.commandData_2->setText(commandData);
            i++;
            commandData = convertNum(response,i,2,16);
            i += 2;
            XbeeApFormUi.commandData_3->setText(commandData);
            switch (response.at(i++).toAscii())
            {
                case 0:
                    commandData = "Coordinator"; break;
                case 1:
                    commandData = "Router"; break;
                case 2:
                    commandData = "Endpoint"; break;
                default:
                    commandData = "Unknown"; break;
            }
            XbeeApFormUi.commandData_4->setText(commandData);
            commandData = QString::number(response[i++].toAscii(),16).toUpper().right(2);
            XbeeApFormUi.commandData_5->setText(commandData);
            commandData = convertNum(response,i,2,16);
            i += 2;
            XbeeApFormUi.commandData_6->setText(commandData);
            commandData = convertNum(response,i,2,16);
            i += 2;
            XbeeApFormUi.commandData_7->setText(commandData);
            if (i == responseSize-5)                // DD value appended (see NO command)
            {
                XbeeApFormUi.commandData_8->setVisible(true);
                XbeeApFormUi.commandDataLabel_8->setVisible(true);
                XbeeApFormUi.commandDataLabel_8->setText("Device ID");
                commandData = convertNum(response,i,4,16);
                XbeeApFormUi.commandData_8->setText(commandData);
            }
        }
        else if (atResponseCommand == "MY")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Module 16-bit Network Address");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = convertNum(response,8,2,16);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "NC")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Number of Children Remaining");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            uint i = 8;                  // Start of data block
            commandData = QString::number(response[i++].toAscii(),10);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "NI")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Module Node Identifier");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            uint i = 8;                  // Start of data block
            for (commandData = ""; i < responseSize-1; i++) commandData += response[i];
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "NP")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Number of Payload Bytes");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            uint i = 8;                  // Start of data block
            uint payloadBytes = (uchar)response[i++].toAscii() << 8;
            payloadBytes += (uchar)response[i++].toAscii();
            commandData = QString::number(payloadBytes,10);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "DB")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Last Received Power Level");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            uint i = 8;                  // Start of data block
            commandData = "-"+QString::number((uchar)response[i++].toAscii(),10) + "dBm";
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "CH")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("802.15.4 Channel Used");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            uint i = 8;                  // Start of data block
            uchar channel = (uchar)response[i++].toAscii();
            if (channel > 0)
                commandData = QString::number(channel,10);
            else
                commandData = "Not Joined";
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "ID")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("64-Bit Extended PAN ID");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = convertNum(response,8,8,16);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "DD")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Device ID");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = convertNum(response,8,4,16);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "NO")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Node Discovery Options");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = "";
            uchar options = response[8].toAscii();
            if (options == 0) commandData += "None";
            if ((options & 0x01) > 0) commandData += "Append Device ID ";
            if ((options & 0x02) > 0) commandData += "Send also local ND results";
            if (options > 3) commandData = "No Value";
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "SC")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Scan Channel List");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = "";
            for (uchar i = 0; i < 16; i++)
            {
                if (((response[8].toAscii() + (response[9].toAscii() << 8)) & (1 << i)) > 0)
                    commandData += QString::number(11+i,10).toUpper().right(2)+" ";
            }
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "PP")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Peak Power");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            uint i = 8;                  // Start of data block
            commandData = QString::number((uchar)response[i++].toAscii(),10) + "dBm";
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "PL")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Power Level");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            uint i = 8;                  // Start of data block
            commandData = QString::number((uchar)response[i++].toAscii(),10);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "DN")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Destination Node");
            if (commandStatus == 0)
            {
                XbeeApFormUi.commandData_1->setVisible(true);
                XbeeApFormUi.commandDataLabel_1->setVisible(true);
                XbeeApFormUi.commandDataLabel_1->setText("Network Addr");
                commandData = convertNum(response,8,2,16);
                XbeeApFormUi.commandData_1->setText(commandData);
                XbeeApFormUi.commandData_2->setVisible(true);
                XbeeApFormUi.commandDataLabel_2->setVisible(true);
                XbeeApFormUi.commandDataLabel_2->setText("Extended Addr");
                commandData = convertNum(response,10,8,16);
                XbeeApFormUi.commandData_2->setText(commandData);
            }
        }
        else if (atResponseCommand == "SH")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("S/N High");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = convertNum(response,8,4,16);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "SL")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("S/N Low");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = convertNum(response,8,4,16);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
    }
}

//-----------------------------------------------------------------------------
/** @brief Clear the command string

*/
void XbeeApTool::on_clearButton_clicked()
{
    XbeeApFormUi.commandEntry->clear();
    XbeeApFormUi.commandEntry->setFocus();
}
//-----------------------------------------------------------------------------
/** @brief Change the baud rate

This requires the port to be closed and reopened with the new baudrate.
This only happens if the user selects a different baud rate.
*/
void XbeeApTool::on_baudRate_currentIndexChanged(int baudrate)
{
    synchronized = false;
    port->close();
    if (port->initPort(baudrate,100))
    {
        synchronized = true;
    }
    else
        errorMessage = QString("Unable to access the serial port\n");
}
//-----------------------------------------------------------------------------
/** @brief Close off the window and deallocate resources

This may not be necessary as QT may implement it implicitly.
*/
void XbeeApTool::closeEvent(QCloseEvent *event)
{
    event->accept();
}

//-----------------------------------------------------------------------------
/** @brief Read returned serial data

Determine if any serial data is available, and read it. Continue until a timeout
occurs. Returns the data as a QString.
*/

QString XbeeApTool::getSerialData()
{
    char buffer[256];               // Read buffer

    QString response;
    uint counter = 0;
    while (port->bytesAvailable() == 0);    // Wait for something to arrive
    while (counter++ < 100000)
    {
        qint64 numberBytes = port->bytesAvailable();
        if (numberBytes > 0)        // If we received something
        {
            counter = 0;            // Reset the counter if something came
            numberBytes = port->read(buffer,256);
            for (int n=0; n < numberBytes; ++n)
            {
                response += buffer[n];
            }
        }
    }
    return response;
}
//-----------------------------------------------------------------------------
/** @brief Convert section of byte string to QString number

*/

QString convertNum(const QString response, const uchar startIndex, const uchar length, const int base)
{
    QString commandData = "";
    for (uchar i = startIndex; i < length+startIndex; i++)
        commandData += QString("%1").arg(QString::number(response[i].toAscii(),base).toUpper().right(2),2,'0');
    return commandData;
}
//-----------------------------------------------------------------------------

