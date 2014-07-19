/*       XBee AP Mode Command Tool
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

#include "xbeep.h"
#include "xbee-ap.h"
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
#include <sys/stat.h>

// Default remote node address
#define DEFAULT_ADDRESS "0013A200408B4B82"

// Local Prototypes
QString convertNum(const QString response, const uchar startIndex, const uchar length, const int base);
void showResponse(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data);

//-----------------------------------------------------------------------------
/** XBee-AP-Tool Constructor

@param[in] parent Parent widget.
*/

XbeeApTool::XbeeApTool(QWidget* parent) : QDialog(parent)
{
	xbee_err ret;
    port.device = "/dev/ttyUSB0";
    port.baudRate = 5;
    port.flowControl = FLOW_OFF;
    port.parity = PAR_NONE;
    port.dataBits = DATA_8;
    port.stopBits = STOP_1;
    line = 0;
// Build the User Interface display from the Ui class in ui_mainwindowform.h
    XbeeApFormUi.setupUi(this);
// Setup BaudRate selector
    XbeeApFormUi.baudRate->addItem("1200");
    XbeeApFormUi.baudRate->addItem("2400");
    XbeeApFormUi.baudRate->addItem("4800");
    XbeeApFormUi.baudRate->addItem("9600");
    XbeeApFormUi.baudRate->addItem("19200");
    XbeeApFormUi.baudRate->addItem("38400");
    XbeeApFormUi.baudRate->addItem("57600");
    XbeeApFormUi.baudRate->addItem("115200");
    XbeeApFormUi.baudRate->setCurrentIndex(port.baudRate);
    XbeeApFormUi.commandEntry->setFocus();
// Initialise xbee instance and serial port first
    struct stat st;
    if(stat("/dev/ttyUSB0",&st) == 0)       /* Check if USB0 exists */
        ret = xbee_setup(&xbee, "xbee2", "/dev/ttyUSB0", 38400);
    else
        ret = xbee_setup(&xbee, "xbee2", "/dev/ttyUSB1", 38400);
    if (ret != XBEE_ENONE)
    {
        errorMessage = QString("Unable to access the serial port\n"
                    "\nCheck the connections and power.");
        return;
    }
// Attempt to send an AP command to revert to mode 1 (in case set to mode 2)
    uchar parm = 1;
    QString commandAP = "AP" + (QString)parm;
    QString packetData;
    if (sendLocalATCommand(commandAP, &packetData) != XBEE_ENONE)
    {
        errorMessage = QString("Unable to change mode of XBee.");
        return;
    }
// Hide all invisible widgets
    hideWidgets();
// Setup the AP Command selector    
    XbeeApFormUi.atCommandList->addItem("NI Node Identifier");
    XbeeApFormUi.atCommandList->addItem("ID Extended PAN ID");
    XbeeApFormUi.atCommandList->addItem("MY 16-bit Network Addr");
    XbeeApFormUi.atCommandList->addItem("NC Remaining Children");
    XbeeApFormUi.atCommandList->addItem("NP RF Payload Bytes");
    XbeeApFormUi.atCommandList->addItem("DB Last Received SS");
    XbeeApFormUi.atCommandList->addItem("CH Channel Used");
    XbeeApFormUi.atCommandList->addItem("DD Device Type");
    XbeeApFormUi.atCommandList->addItem("NO Node Discovery Opts");
    XbeeApFormUi.atCommandList->addItem("SC Scan Channel List");
    XbeeApFormUi.atCommandList->addItem("PP Peak Power");
    XbeeApFormUi.atCommandList->addItem("PL Peak Level");
    XbeeApFormUi.atCommandList->addItem("SH S/N High");
    XbeeApFormUi.atCommandList->addItem("SL S/N Low");
    XbeeApFormUi.atCommandList->addItem("AI Association Indication");
    XbeeApFormUi.atCommandList->addItem("FR Software Reset");
    XbeeApFormUi.atCommandList->addItem("BD Baudrate Set");
    XbeeApFormUi.atCommandList->addItem("DN Destination Node");
    XbeeApFormUi.atCommandList->addItem("ND Node Discovery");
    atListCount = 19;
// Set up a default selection
    on_atCommandList_currentIndexChanged(0);
    XbeeApFormUi.commandEntry->setText(DEFAULT_ADDRESS);

// Setup the AP Query selector    
    XbeeApFormUi.atQueryList->addItem("Serial I/O");
    XbeeApFormUi.atQueryList->addItem("Addresses");
    XbeeApFormUi.atQueryList->addItem("Data");
// Set up a default selection
    on_atQueryList_currentIndexChanged(0);

// Setup a default address
    XbeeApFormUi.addrNode64->setChecked(true);
    nodeAddressDefault = DEFAULT_ADDRESS;
    XbeeApFormUi.commandEntry->setText(nodeAddressDefault);

    XbeeApFormUi.sampleTime->setMinimum(0x32);
    XbeeApFormUi.sampleTime->setMaximum(0xFFFF);

    XbeeApFormUi.powerV->setDecMode();
}

XbeeApTool::~XbeeApTool()
{
    xbee_shutdown(xbee);
}

//-----------------------------------------------------------------------------
/** @brief Successful synchronization

@returns TRUE if the device gave no error
*/
bool XbeeApTool::success()
{
    return (errorMessage.size() == 0);
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
    XbeeApFormUi.remoteAddrBox->setVisible(false);
    XbeeApFormUi.dataEntry->setVisible(false);
    if (XbeeApFormUi.atCommandRemote->isChecked())
    {
        XbeeApFormUi.remoteAddrBox->setVisible(true);
        XbeeApFormUi.atCommandList->setMaxCount(atListCount-2);
    }
    XbeeApFormUi.inV->setVisible(false);
    XbeeApFormUi.inVLabel->setVisible(false);
    XbeeApFormUi.inMax->setVisible(false);
    XbeeApFormUi.inMaxLabel->setVisible(false);
    XbeeApFormUi.powerV->setVisible(false);
    XbeeApFormUi.powerVLabel->setVisible(false);
    XbeeApFormUi.inVProgress->setVisible(false);
    XbeeApFormUi.inVProgress->setValue(0);
    XbeeApFormUi.inVProgress->setRange(0,100);
}

//-----------------------------------------------------------------------------
/** @brief Setup Options when AT Command dropbox is changed

*/
void XbeeApTool::on_atCommandList_currentIndexChanged(int atCommand)
{
    hideWidgets();
    switch(atCommand)
    {
        case 8:             // Node Discovery Options command
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
/** @brief Setup Options when AT Query dropbox is changed

*/
void XbeeApTool::on_atQueryList_currentIndexChanged(int atCommand)
{
    switch(atCommand)
    {
        case 2:             // Data transmission command
            XbeeApFormUi.dataEntry->setVisible(true);
            break;
    }
}
//-----------------------------------------------------------------------------
/** @brief Setup a Data Sampling Operation.

*/
void XbeeApTool::on_dataSetup_clicked()
{
    QString command;
    QString packetData;
    QChar parameter[] = {0x03};
    if (XbeeApFormUi.digital04Box->isChecked())
    {
// Enable digital input DIO2
        parameter[0] = QChar(0x03);
        command = "D2" + QString::fromRawData(parameter,1);
        sendRemoteATCommand(command, &packetData);
    }
    else
    {
// Disable digital input DIO2
        parameter[0] = QChar(0x00);
        command = "D2" + QString::fromRawData(parameter,1);
        sendRemoteATCommand(command, &packetData);
    }
    if (XbeeApFormUi.analogueInBox->isChecked())
    {
// Enable two analogue inputs
        parameter[0] = QChar(0x02);
        command = "D0" + QString::fromRawData(parameter,1);
        sendRemoteATCommand(command, &packetData);
        command = "D1" + QString::fromRawData(parameter,1);
        sendRemoteATCommand(command, &packetData);
// Disable the pullup resistor for the GPIO0 and GPIO1 inputs
        parameter[0] = QChar(0x3F);
        parameter[1] = QChar(0xE7);
        command = "PR" + QString::fromRawData(parameter,2);
        sendRemoteATCommand(command, &packetData);
    }
    else
    {
// Disable two analogue inputs
        parameter[0] = QChar(0x00);
        command = "D0" + QString::fromRawData(parameter,1);
        sendRemoteATCommand(command, &packetData);
        command = "D1" + QString::fromRawData(parameter,1);
        sendRemoteATCommand(command, &packetData);
    }
// Set the remote to sample cyclically
    if (XbeeApFormUi.cyclicSampleBox->isChecked())
    {
        int value = XbeeApFormUi.sampleTime->value();
        parameter[1] = QChar(value % 256);
        parameter[0] = QChar((value >> 8) % 256);
        command = "IR" + QString::fromRawData(parameter,2);
        sendRemoteATCommand(command, &packetData);
    }
    else
    {
// Disable cyclic sampling
        parameter[0] = QChar(0x00);
        parameter[1] = QChar(0x00);
        command = "IR" + QString::fromRawData(parameter,2);
        sendRemoteATCommand(command, &packetData);
    }
// Read back the enables
    sendRemoteATCommand("D0", &packetData);
    qDebug() << "D0 enable" << convertNum(packetData,0,packetData.size(),16);
    sendRemoteATCommand("D1", &packetData);
    qDebug() << "D1 enable" << convertNum(packetData,0,packetData.size(),16);
    sendRemoteATCommand("D2", &packetData);
    qDebug() << "D2 enable" << convertNum(packetData,0,packetData.size(),16);
    sendRemoteATCommand("D3", &packetData);
    qDebug() << "D3 enable" << convertNum(packetData,0,packetData.size(),16);
    sendRemoteATCommand("IR", &packetData);
    qDebug() << "Cyclic Sampling" << convertNum(packetData,0,packetData.size(),16);
}
//-----------------------------------------------------------------------------
/** @brief Request all enabled samples Now.

*/
void XbeeApTool::on_getDataSamples_clicked()
{
    bool ok;
    QString command;
    QString packetData;
// Get the supply voltage
    command = "%V";
    sendRemoteATCommand(command, &packetData);
    float powerVoltage = (float)(convertNum(packetData,0,packetData.size(),16).toUInt(&ok,16))*1.2/1024;
    qDebug() << powerVoltage << "(mV)";
    XbeeApFormUi.powerV->setVisible(true);
    XbeeApFormUi.powerVLabel->setVisible(true);
    XbeeApFormUi.powerV->display(powerVoltage);
// Get all the enabled digital and analogue inputs
    command = "IS";
    sendRemoteATCommand(command, &packetData);
    qDebug() << "Data Grab" << convertNum(packetData,0,packetData.size(),16);
// Interpret the response
    uint index = 1;
    uint digitalEnables = convertNum(packetData,index,2,16).toUInt(&ok,16);
    index += 2;
    uint analogueEnables = convertNum(packetData,index,1,16).toUInt(&ok,16);
    index += 1;
// Assume only the one digital and two analogue are enabled
    if (digitalEnables > 0)
    {
        uint stateD2 = ((convertNum(packetData,index,2,16).toUInt(&ok,16) >> 2) & 0x01);
        index += 2;
    }
    float voltageD0;
    float voltageD1;
    if ((analogueEnables & 0x02) > 0)
    {
        voltageD1 = (float)(convertNum(packetData,index,2,16).toUInt(&ok,16))*1.2/1024;
        XbeeApFormUi.inMax->setVisible(true);
        XbeeApFormUi.inMaxLabel->setVisible(true);
        XbeeApFormUi.inMax->display(voltageD1);
        index += 2;
    }
    if ((analogueEnables & 0x01) > 0)
    {
        voltageD0 = (float)(convertNum(packetData,index,2,16).toUInt(&ok,16))*1.2/1024;
        XbeeApFormUi.inV->setVisible(true);
        XbeeApFormUi.inVLabel->setVisible(true);
        XbeeApFormUi.inV->display(voltageD0);
        index += 2;
    }
    if ((analogueEnables & 0x03) == 3)
    {
        XbeeApFormUi.inVProgress->setVisible(true);
        XbeeApFormUi.inVProgress->setValue(voltageD0*100/voltageD1);
    }
}
//-----------------------------------------------------------------------------
/** @brief Display the options when remote command selected.

*/
void XbeeApTool::on_atCommandRemote_clicked()
{
    if (XbeeApFormUi.atCommandRemote->isChecked())
    {
        XbeeApFormUi.remoteAddrBox->setVisible(true);
        XbeeApFormUi.addrNode64->setChecked(true);
        XbeeApFormUi.atCommandList->setMaxCount(atListCount-2);
    }
    else
    {
// Wipe the last two inappropriate selections
        XbeeApFormUi.remoteAddrBox->setVisible(false);
        XbeeApFormUi.atCommandList->setMaxCount(atListCount);
        XbeeApFormUi.atCommandList->addItem("DN Destination Node");
        XbeeApFormUi.atCommandList->addItem("ND Node Discovery");
    }
}
//-----------------------------------------------------------------------------
/** @brief Build complex query when user has completed settings, and send off.

Wait in a loop until message arrives, or timeout.
*/
void XbeeApTool::on_sendQuery_clicked()
{
    QString atCommand;
    QString commandData;
    QString packetData;
    switch(XbeeApFormUi.atQueryList->currentIndex())
    {
        case 0:             // Get serial I/O parameters
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Serial I/O Parameters");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("Mode");
            XbeeApFormUi.commandData_2->setVisible(true);
            XbeeApFormUi.commandDataLabel_2->setVisible(true);
            XbeeApFormUi.commandDataLabel_2->setText("Baud rate");
            XbeeApFormUi.commandData_3->setVisible(true);
            XbeeApFormUi.commandDataLabel_3->setVisible(true);
            XbeeApFormUi.commandDataLabel_3->setText("Parity");
            XbeeApFormUi.commandData_4->setVisible(true);
            XbeeApFormUi.commandDataLabel_4->setVisible(true);
            XbeeApFormUi.commandDataLabel_4->setText("Stop Bits");
            XbeeApFormUi.commandData_5->setVisible(true);
            XbeeApFormUi.commandDataLabel_5->setVisible(true);
            XbeeApFormUi.commandDataLabel_5->setText("Options");
            atCommand = "AP";
            sendRemoteATCommand(atCommand, &packetData);
            commandData = convertNum(packetData,0,packetData.size(),16);
            XbeeApFormUi.commandData_1->setText(commandData);
            atCommand = "BD";
            sendRemoteATCommand(atCommand, &packetData);
            commandData = convertNum(packetData,0,packetData.size(),16);
            XbeeApFormUi.commandData_2->setText(commandData);
            atCommand = "NB";
            sendRemoteATCommand(atCommand, &packetData);
            commandData = convertNum(packetData,0,packetData.size(),16);
            XbeeApFormUi.commandData_3->setText(commandData);
            atCommand = "SB";
            sendRemoteATCommand(atCommand, &packetData);
            commandData = convertNum(packetData,0,packetData.size(),16);
            XbeeApFormUi.commandData_4->setText(commandData);
            atCommand = "AO";
            sendRemoteATCommand(atCommand, &packetData);
            commandData = convertNum(packetData,0,packetData.size(),16);
            XbeeApFormUi.commandData_5->setText(commandData);
            break;
        case 1:             // Get Address Details
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Addresses");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("Destination Addr");
            XbeeApFormUi.commandData_2->setVisible(true);
            XbeeApFormUi.commandDataLabel_2->setVisible(true);
            XbeeApFormUi.commandDataLabel_2->setText("16 bit Addr");
            XbeeApFormUi.commandData_3->setVisible(true);
            XbeeApFormUi.commandDataLabel_3->setVisible(true);
            XbeeApFormUi.commandDataLabel_3->setText("Parent Addr");
            XbeeApFormUi.commandData_4->setVisible(true);
            XbeeApFormUi.commandDataLabel_4->setVisible(true);
            XbeeApFormUi.commandDataLabel_4->setText("Serial Number");
            XbeeApFormUi.commandData_5->setVisible(true);
            XbeeApFormUi.commandDataLabel_5->setVisible(true);
            XbeeApFormUi.commandDataLabel_5->setText("Node Ident");
            atCommand = "DH";
            sendRemoteATCommand(atCommand, &packetData);
            commandData = convertNum(packetData,0,packetData.size(),16);
            atCommand = "DL";
            sendRemoteATCommand(atCommand, &packetData);
            commandData += convertNum(packetData,0,packetData.size(),16);
            XbeeApFormUi.commandData_1->setText(commandData);
            atCommand = "MY";
            sendRemoteATCommand(atCommand, &packetData);
            commandData = convertNum(packetData,0,packetData.size(),16);
            XbeeApFormUi.commandData_2->setText(commandData);
            atCommand = "MP";
            if (sendRemoteATCommand(atCommand, &packetData) == XBEE_ENONE)
                commandData = convertNum(packetData,0,packetData.size(),16);
            else
                commandData = "Failed";
            XbeeApFormUi.commandData_3->setText(commandData);
            atCommand = "SH";
            sendRemoteATCommand(atCommand, &packetData);
            commandData = convertNum(packetData,0,packetData.size(),16);
            atCommand = "SL";
            sendRemoteATCommand(atCommand, &packetData);
            commandData += convertNum(packetData,0,packetData.size(),16);
            XbeeApFormUi.commandData_4->setText(commandData);
            atCommand = "NI";
            sendRemoteATCommand(atCommand, &packetData);
            commandData = packetData;
            XbeeApFormUi.commandData_5->setText(commandData);
            break;
        case 2:                 // Send Data
            xbee_err retStatus;
            struct xbee_pkt *pkt;
            QString nodeAddress;
            QString addressParm = XbeeApFormUi.commandEntry->text().toUpper();
            int pktRemaining;
            bool ok;
            if (!XbeeApFormUi.atCommandRemote->isChecked())
            {
		        qDebug() << "Must be a remote command ";
                break;
        	}
// Start by finding the address from the node name if necessary
            if (XbeeApFormUi.addrNodeId->isChecked())
            {
                QString packetData;
                QString commandDN = "DN" + addressParm;
                sendLocalATCommand(commandDN, &packetData);
                nodeAddress = convertNum(packetData,2,8,16);
                XbeeApFormUi.addrNode64->setChecked(true);
                XbeeApFormUi.commandEntry->setText(nodeAddress);
                nodeAddressDefault = nodeAddress;
            }
            else
            {
                nodeAddress = addressParm;
            }
            qDebug() << "Node Address " << nodeAddress;
// Setup a data connection to send the data
            struct xbee_conAddress address;
            memset(&address, 0, sizeof(address));
            address.addr64_enabled = 1;
            for (uint i = 0; i < 8; i++)
            {
                address.addr64[i] = nodeAddress.mid(2*i, 2).toInt(&ok,16);
                if (! ok)
                {   qDebug() << "Not an address "; break; }
            }
        	if ((retStatus = xbee_conNew(xbee, &con, "Data", &address)) != XBEE_ENONE)
            {
		        qDebug() << "New Send-Data Connection error " << retStatus;
                break;
        	}
            qDebug() << XbeeApFormUi.dataEntry->text().toAscii().data();
    		if ((retStatus = xbee_conTx(con, NULL, XbeeApFormUi.dataEntry->text().toAscii().data())) != XBEE_ENONE)
            {
		        qDebug() << "Data Tx error " << retStatus;
		        break;
	        }
            if ((retStatus=xbee_conEnd(con)) != XBEE_ENONE)
            {
                qDebug() << "Connection Terminate error " << retStatus;
		        break;
            }
// Now setup a listening connection to wait for the response
        	if ((retStatus = xbee_conNew(xbee, &con, "Data", &address)) != XBEE_ENONE)
            {
		        qDebug() << "New Receive-Data Connection error " << retStatus;
                break;
        	}
	        if ((retStatus = xbee_conDataSet(con, xbee, NULL)) != XBEE_ENONE)
            {
		        qDebug() << "Dataset error " << retStatus;
		        break;
	        }
            if ((retStatus = xbee_conRxWait(con, &pkt, &pktRemaining)) != XBEE_ENONE)
            {
                qDebug() << "Rx data error " << retStatus;
            }   
            else
            {
                uint responseSize = (pkt)->dataLen;
                printf("Response Length %d ", responseSize);
                for (uint i = 0; i < responseSize; ++i)
                    packetData[i] = (pkt)->data[i];
                for (uint i = 0; i < responseSize; ++i)
                    printf("%02X ",(pkt)->data[i]);
                    printf("\n");
            }
            if ((retStatus=xbee_conEnd(con)) != XBEE_ENONE)
            {
                qDebug() << "Connection Terminate error " << retStatus;
            }
            if ((retStatus=xbee_pktFree(pkt)) != XBEE_ENONE)
            {
                qDebug() << "Release Packet error " << retStatus;
            }
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Data received");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            XbeeApFormUi.commandData_1->setText(packetData);
            break;
    }
}
//-----------------------------------------------------------------------------
/** @brief Build command when user has completed settings, and send off.

Wait in a loop until message arrives, or timeout.
*/
void XbeeApTool::on_sendButton_clicked()
{
    QString atCommand;
    QString parms;
    parms = "";
    uchar sendOptions;
    bool ok;
    xbee_err retStatus;
	uchar txRet;
    int pktRemaining;
    struct xbee_pkt *pkt;
    QString addressParm = XbeeApFormUi.commandEntry->text().toUpper();
// Start building the binary packet
//    if (XbeeApFormUi.atCommandBuffered->isChecked())
// Setup AT command string from user entered data
    switch(XbeeApFormUi.atCommandList->currentIndex())
    {
        case 0:             // Node Identifier command
            atCommand = "NI";
            break;
        case 1:             // Extended 64-bit PAN ID command
            atCommand = "ID";
            break;
        case 2:             // 16-bit Network Address command
            atCommand = "MY";
            break;
        case 3:             // Number of Children Remaining command
            atCommand = "NC";
            break;
        case 4:             // Number of Payload Bytes command
            atCommand = "NP";
            break;
        case 5:             // Last Received Power Level command
            atCommand = "DB";
            break;
        case 6:             // 802.15.4 Channel command
            atCommand = "CH";
            break;
        case 7:             // Device Type command
            atCommand = "DD";
            break;
        case 8:             // Node Discovery Options command
            atCommand = "NO";
            sendOptions = (XbeeApFormUi.atCommandBinOpt_1->isChecked() ? 1 : 0)
                                 + ((XbeeApFormUi.atCommandBinOpt_2->isChecked() ? 1 : 0) << 1);
            if (XbeeApFormUi.atCommandSet->isChecked())
            {
                parms = sendOptions;
            }
            break;
        case 9:             // Scan channel list command
            atCommand = "SC";
            break;
        case 10:             // Peak Power command
            atCommand = "PP";
            break;
        case 11:             // Power Level command
            atCommand = "PL";
            break;
        case 12:             // S/N High command
            atCommand = "SH";
            break;
        case 13:             // S/N Low command
            atCommand = "SL";
            break;
        case 14:             // Association indication
            atCommand = "AI";
            break;
        case 15:             // Software Reset command
            atCommand = "FR";
            break;
        case 16:             // Baud rate index command
            atCommand = "BD";
            parms = XbeeApFormUi.baudRate->currentIndex();
            break;
        case 17:             // Destination Node command
            atCommand = "DN";
            parms = addressParm;
            break;
        case 18:             // Node Discovery Command
            atCommand = "ND";
            break;
    }
    QString message = atCommand+parms;

// Attempt to set a connection, transmit command and analyse response.
    QString nodeAddress;
    QString packetData;
    struct xbee_conAddress address;
// Do we have a remote AT? If so, then we need the node address.
    if (XbeeApFormUi.atCommandRemote->isChecked())
    {
// If we are specifying a node ID, then first query for the address with a local AT DN command.
        if (XbeeApFormUi.addrNodeId->isChecked())
        {
            QString commandDN = "DN" + addressParm;
            sendLocalATCommand(commandDN, &packetData);
            nodeAddress = convertNum(packetData,2,8,16);
            XbeeApFormUi.addrNode64->setChecked(true);
            XbeeApFormUi.commandEntry->setText(nodeAddress);
            nodeAddressDefault = nodeAddress;
        }
        else
        {
// Pull address from the text box
            nodeAddress = addressParm;
        }
        qDebug() << "Node Address " << nodeAddress;
// Create a new connection for the command. Local AT or Remote AT
        memset(&address, 0, sizeof(address));
        address.addr64_enabled = 1;
        for (uint i = 0; i < 8; i++)
        {
            address.addr64[i] = nodeAddress.mid(2*i, 2).toInt(&ok,16);
            if (! ok)
            {   qDebug() << "Not an address "; return; }
        }
        retStatus = xbee_conNew(xbee, &con, "Remote AT", &address);
    }
    else
        retStatus = xbee_conNew(xbee, &con, "Local AT", NULL);
    if (retStatus != XBEE_ENONE)
    {   qDebug() << "New Connection error " << retStatus; return; }
// Transmit command and wait for the response.
    if ((retStatus=xbee_conTx(con, &txRet, message.toAscii().data())) != XBEE_ENONE)
    {   qDebug() << "Tx error " << retStatus; }
// Wait up to 1 second for return packet regardless of Tx errors.
// In the ND case we need over 5 seconds to get the packets back.
    uchar rxWait = (message.left(2) == "ND") ? 100: 20;
    {
        for (uchar i = 0; i < rxWait; i++)
        {
            if ((retStatus = xbee_conRx(con, &pkt, &pktRemaining)) != XBEE_ENOTEXISTS) break;
            usleep(50000);
        }
    }
    if (retStatus != XBEE_ENONE)
    {   qDebug() << "Rx error " << retStatus; }
// Validate packet
    else if ((retStatus=xbee_pktValidate(pkt)) != XBEE_ENONE)
    {   qDebug() << "Validate error " << retStatus; }
// if OK display it
    else
        showResponse(&pkt);
// Terminate the connection and free packet memory
    if ((retStatus=xbee_conEnd(con)) != XBEE_ENONE)
    {   qDebug() << "Connection Terminate error " << retStatus; }
    if ((retStatus=xbee_pktFree(pkt)) != XBEE_ENONE)
    {   qDebug() << "Release Packet error " << retStatus; }
}
//-----------------------------------------------------------------------------
/** @brief Interpret response from device

The received characters are read in and displayed.
*/
void XbeeApTool::showResponse(struct xbee_pkt **pkt)
{
    QString commandData;
    QString packetData;
// Read back the response and interpret
    uint responseSize = (*pkt)->dataLen;
    qDebug() << "Received Packet Data Block";
    for (uint i = 0; i < responseSize; ++i)
    {
        packetData[i] = (*pkt)->data[i];
        qDebug() << QString::number((*pkt)->data[i],16).toUpper().right(2);
    }
    qDebug() << "End";
// At this point we check the command returned and display fields
// Frame type is always present (but showing connection type with libxbee)
    XbeeApFormUi.frameTypeLabel->setVisible(true);
    XbeeApFormUi.frameType->setVisible(true);
// Find the frame type to start with
    const char *conType = (*pkt)->conType;
    QString conTypeString = conType;
    XbeeApFormUi.frameType->setText(conTypeString);
// Display AT Command response preamble
    if ((conTypeString == "Local AT") || (conTypeString == "Remote AT"))
    {
        XbeeApFormUi.frameID->setVisible(true);
        XbeeApFormUi.frameIDLabel->setVisible(true);
        XbeeApFormUi.atCommand->setVisible(true);
        XbeeApFormUi.atCommandLabel->setVisible(true);
        XbeeApFormUi.commandStatus->setVisible(true);
        XbeeApFormUi.commandStatusLabel->setVisible(true);
// AT Command response preamble is always displayed
        XbeeApFormUi.frameID->setText(QString::number((*pkt)->frameId,16).toUpper().right(2));
        char atrc[2];
        atrc[0] = (*pkt)->atCommand[0];
        atrc[1] = (*pkt)->atCommand[1];
        QString atResponseCommand = QString::fromAscii(atrc).left(2);
        XbeeApFormUi.atCommand->setText(atResponseCommand);
        uchar commandStatus = (*pkt)->status;
        XbeeApFormUi.commandStatus->setText((commandStatus == 0) ? "OK" : "Err");
// The Node Discovery response
        if (atResponseCommand == "ND")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Node Discovery Response");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("16 Bit Address");
            XbeeApFormUi.commandData_2->setVisible(true);
            XbeeApFormUi.commandDataLabel_2->setVisible(true);
            XbeeApFormUi.commandDataLabel_2->setText("Serial number");
            XbeeApFormUi.commandData_3->setVisible(true);
            XbeeApFormUi.commandDataLabel_3->setVisible(true);
            XbeeApFormUi.commandDataLabel_3->setText("Node Identifier");
            XbeeApFormUi.commandData_4->setVisible(true);
            XbeeApFormUi.commandDataLabel_4->setVisible(true);
            XbeeApFormUi.commandDataLabel_4->setText("Parent Address");
            XbeeApFormUi.commandData_5->setVisible(true);
            XbeeApFormUi.commandDataLabel_5->setVisible(true);
            XbeeApFormUi.commandDataLabel_5->setText("Device Type");
            XbeeApFormUi.commandData_6->setVisible(true);
            XbeeApFormUi.commandDataLabel_6->setVisible(true);
            XbeeApFormUi.commandDataLabel_6->setText("Status");
            XbeeApFormUi.commandData_7->setVisible(true);
            XbeeApFormUi.commandDataLabel_7->setVisible(true);
            XbeeApFormUi.commandDataLabel_7->setText("Profile ID");
            XbeeApFormUi.commandData_8->setVisible(true);
            XbeeApFormUi.commandDataLabel_8->setVisible(true);
            XbeeApFormUi.commandDataLabel_8->setText("Mfgr ID");
// ND data field breakdown
            uint i = 0;                  // Start of data block
            commandData = convertNum(packetData,i,2,16);
            XbeeApFormUi.commandData_1->setText(commandData);
            i += 2;
            commandData = convertNum(packetData,i,4,16);
            i += 4;
            commandData += convertNum(packetData,i,4,16);
            XbeeApFormUi.commandEntry->setText(commandData);
            i += 4;
            XbeeApFormUi.commandData_2->setText(commandData);
            for (commandData = ""; packetData[i] != 0; i++) commandData += packetData[i];
            XbeeApFormUi.commandData_3->setText(commandData);
            i++;
            commandData = convertNum(packetData,i,2,16);
            i += 2;
            XbeeApFormUi.commandData_4->setText(commandData);
            switch (packetData[i++].toAscii())
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
            XbeeApFormUi.commandData_5->setText(commandData);
            commandData = QString::number(packetData[i++].toAscii(),16).toUpper().right(2);
            XbeeApFormUi.commandData_6->setText(commandData);
            commandData = convertNum(packetData,i,2,16);
            i += 2;
            XbeeApFormUi.commandData_7->setText(commandData);
            commandData = convertNum(packetData,i,2,16);
            i += 2;
            XbeeApFormUi.commandData_8->setText(commandData);
            if (i == responseSize-5)                // DD value appended (see NO command)
            {
                XbeeApFormUi.commandData_8->setVisible(true);
                XbeeApFormUi.commandDataLabel_8->setVisible(true);
                XbeeApFormUi.commandDataLabel_8->setText("Device ID");
                commandData = convertNum(packetData,i,4,16);
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
            commandData = convertNum(packetData,0,2,16);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "NC")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Number of Children Remaining");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = QString::number(packetData[0].toAscii(),10);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "NI")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Module Node Identifier");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            XbeeApFormUi.commandData_1->setText(packetData);
        }
        else if (atResponseCommand == "NP")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Number of Payload Bytes");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            uint i = 0;                  // Start of data block
            uint payloadBytes = (uchar)packetData[i++].toAscii() << 8;
            payloadBytes += (uchar)packetData[i++].toAscii();
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
            commandData = "-"+QString::number((uchar)packetData[0].toAscii(),10) + "dBm";
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "CH")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("802.15.4 Channel Used");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            uchar channel = (uchar)packetData[0].toAscii();
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
            commandData = convertNum(packetData,0,8,16);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "DD")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Device ID");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = convertNum(packetData,0,4,16);
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
            uchar options = packetData[0].toAscii();
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
                if (((packetData[0].toAscii() + (packetData[1].toAscii() << 8)) & (1 << i)) > 0)
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
            commandData = QString::number((uchar)packetData[0].toAscii(),10) + "dBm";
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "PL")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Power Level");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = QString::number((uchar)packetData[0].toAscii(),10);
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
                commandData = convertNum(packetData,0,2,16);
                XbeeApFormUi.commandData_1->setText(commandData);
                XbeeApFormUi.commandData_2->setVisible(true);
                XbeeApFormUi.commandDataLabel_2->setVisible(true);
                XbeeApFormUi.commandDataLabel_2->setText("Extended Addr");
                commandData = convertNum(packetData,2,8,16);
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
            commandData = convertNum(packetData,0,4,16);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "SL")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("S/N Low");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = convertNum(packetData,0,4,16);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
        else if (atResponseCommand == "AI")
        {
            XbeeApFormUi.commandDataLabel->setVisible(true);
            XbeeApFormUi.commandDataLabel->setText("Association Indication");
            XbeeApFormUi.commandData_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setVisible(true);
            XbeeApFormUi.commandDataLabel_1->setText("");
            commandData = QString::number((uchar)packetData[0].toAscii(),10);
            XbeeApFormUi.commandData_1->setText(commandData);
        }
    }
}

//-----------------------------------------------------------------------------
/** @brief Send a Local AT command and retrieve data

*/
xbee_err XbeeApTool::sendLocalATCommand(QString command, QString *packetData)
{
    xbee_err retStatus;
    uchar txRet;
    int pktRemaining;
    struct xbee_pkt *pkt;
    QString x;

    if ((retStatus = xbee_conNew(xbee, &con, "Local AT", NULL)) != XBEE_ENONE)
    {
        qDebug() << "New Local Connection error " << retStatus;
        return retStatus;
    }
    qDebug() << "Command " << convertNum(command, 0, command.size(), 16);
    if ((retStatus = xbee_conTx(con, &txRet, command.toAscii().data())) != XBEE_ENONE)
    {
        qDebug() << "Tx error " << retStatus;
    }
    if ((retStatus = xbee_conRxWait(con, &pkt, &pktRemaining)) != XBEE_ENONE)
    {
        qDebug() << "Rx error " << retStatus;
    }   
    else
    {
        uint responseSize = (pkt)->dataLen;
        for (uint i = 0; i < responseSize; ++i)
            x[i] = (pkt)->data[i];
        *packetData = x;
    }
// Terminate the connection and free packet memory
    if ((retStatus=xbee_conEnd(con)) != XBEE_ENONE)
    {
        qDebug() << "Connection Terminate error " << retStatus;
    }
    if ((retStatus=xbee_pktFree(pkt)) != XBEE_ENONE)
    {
        qDebug() << "Release Packet error " << retStatus;
    }
    return retStatus;
}

//-----------------------------------------------------------------------------
/** @brief Send a Remote AT command and retrieve data

This takes a command and sends it, retrieving the returned data as a QString.
If the GUI sets a remote node, then this is worked in by retrieving the address.
*/
xbee_err XbeeApTool::sendRemoteATCommand(QString command, QString *packetData)
{
    xbee_err retStatus;
    uchar txRet;
    int pktRemaining;
    struct xbee_pkt *pkt;
    QString x;
    QString nodeAddress;
    QString addressParm = XbeeApFormUi.commandEntry->text().toUpper();
    bool ok;
// Do we have a remote AT? If so, then we need to grab the node address.
    if (XbeeApFormUi.atCommandRemote->isChecked())
    {
// If we are specifying a node ID, then first query for the address with a local AT DN command.
        if (XbeeApFormUi.addrNodeId->isChecked())
        {
            QString packetData;
            QString commandDN = "DN" + addressParm;
            sendLocalATCommand(commandDN, &packetData);
            nodeAddress = convertNum(packetData,2,8,16);
            XbeeApFormUi.addrNode64->setChecked(true);
            XbeeApFormUi.commandEntry->setText(nodeAddress);
            nodeAddressDefault = nodeAddress;
        }
        else
        {
// Pull address from the text box
            nodeAddress = addressParm;
        }
        qDebug() << "Node Address " << nodeAddress;
// Get the address converted from the QString
        struct xbee_conAddress address;
        memset(&address, 0, sizeof(address));
        address.addr64_enabled = 1;
        for (uint i = 0; i < 8; i++)
        {
            address.addr64[i] = nodeAddress.mid(2*i, 2).toInt(&ok,16);
            if (! ok)
            {   qDebug() << "Not an address "; return XBEE_EUNKNOWN; }
        }

        retStatus = xbee_conNew(xbee, &con, "Remote AT", &address);
    }
    else
        retStatus = xbee_conNew(xbee, &con, "Local AT", NULL);
    if (retStatus != XBEE_ENONE)
    {
        qDebug() << "New Remote Connection error " << retStatus;
        return retStatus;
    }
    qDebug() << "Command " << convertNum(command, 0, command.size(), 16);
     if ((retStatus = xbee_conTx(con, &txRet, command.toAscii().data())) != XBEE_ENONE)
    {
        qDebug() << "Tx error " << retStatus;
    }
     if ((retStatus = xbee_conRxWait(con, &pkt, &pktRemaining)) != XBEE_ENONE)
    {
        qDebug() << "Rx error " << retStatus;
    }
    else
    {
        uint responseSize = (pkt)->dataLen;
        for (uint i = 0; i < responseSize; ++i)
            x[i] = (pkt)->data[i];
        *packetData = x;
    }
// Terminate the connection and free packet memory
    if ((retStatus=xbee_conEnd(con)) != XBEE_ENONE)
    {
        qDebug() << "Connection Terminate error " << retStatus;
    }
    if ((retStatus=xbee_pktFree(pkt)) != XBEE_ENONE)
    {
        qDebug() << "Release Packet error " << retStatus;
    }
    return retStatus;
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

