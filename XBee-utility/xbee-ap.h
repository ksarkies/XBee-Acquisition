/*               XBee AP Mode Command Tool
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

#ifndef XBEE_AP_TOOL_H
#define XBEE_AP_TOOL_H

#include "xbeep.h"
#include "ui_xbee-ap.h"
#include <QThread>
#include <QListWidgetItem>
#include <QDialog>
#include <QCloseEvent>

// Default remote node address
#define DEFAULT_ADDRESS         "0013A200408B4B82"
#define SERIAL_PORT             "/dev/ttyUSB0"
// Default baud rate at 38400
#define BAUD_RATE               5

// Serial Port Parameters

enum DataBitsType
{
    DATA_5 = 5,
    DATA_6 = 6,
    DATA_7 = 7,
    DATA_8 = 8
};

enum ParityType
{
    PAR_NONE,
    PAR_ODD,
    PAR_EVEN,
    PAR_SPACE
};

enum StopBitsType
{
    STOP_1,
    STOP_2
};

enum FlowType
{
    FLOW_OFF,
    FLOW_HARDWARE,
    FLOW_XONXOFF
};

// Serial Port Structure
struct serialPortType
{
    QString device;
    uchar baudRate;
    uchar flowControl;
    uchar parity;
    uchar dataBits;
    uchar stopBits;
};

//-----------------------------------------------------------------------------
/** @brief XBee Control Window.

*/

class XbeeApTool : public QDialog
{
    Q_OBJECT
public:
    XbeeApTool(QWidget* parent = 0);
    ~XbeeApTool();
    bool success();
    QString error();
private slots:
    void on_sendButton_clicked();
    void on_sendQuery_clicked();
    void on_clearButton_clicked();
    void closeEvent(QCloseEvent*);
    void on_baudRate_currentIndexChanged(int index);
    void on_atCommandList_currentIndexChanged(int index);
    void on_atQueryList_currentIndexChanged(int index);
    void on_atCommandRemote_clicked();
    void on_dataSetup_clicked();
    void on_getDataSamples_clicked();
private:
// User Interface object instance
    Ui::XbeeApDialog XbeeApFormUi;
// methods
    QString convertASCII(QString);
    void hideWidgets();
    void showResponse(struct xbee_pkt **pkt);
    xbee_err sendLocalATCommand(QString command, QString *packetData);
    xbee_err sendRemoteATCommand(QString command, QString *packetData);
//Variables
    uchar packet[256];
    uchar frameID;              // Frame ID changes on each packet
    uint length;                // packet length
    uint line;
    uint atCommand;
    uchar atListCount;
    QString errorMessage;
    QString nodeAddressDefault;
    serialPortType port;
	struct xbee *xbee;
	struct xbee_con *con;
};

#endif
