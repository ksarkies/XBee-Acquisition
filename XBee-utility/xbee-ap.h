/*
Title:    Atmel Microcontroller Serial Port FLASH loader
*/

/****************************************************************************
 *   Copyright (C) 2007 by Ken Sarkies                                      *
 *   ksarkies@trinity.asn.au                                                *
 *                                                                          *
 *   This file is part of avr-serial-prog                                   *
 *                                                                          *
 *   avr-serial-prog is free software; you can redistribute it and/or modify*
 *   it under the terms of the GNU General Public License as published by   *
 *   the Free Software Foundation; either version 2 of the License, or      *
 *   (at your option) any later version.                                    *
 *                                                                          *
 *   avr-serial-prog is distributed in the hope that it will be useful,     *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU General Public License for more details.                           *
 *                                                                          *
 *   You should have received a copy of the GNU General Public License      *
 *   along with avr-serial-prog if not, write to the                        *
 *   Free Software Foundation, Inc.,                                        *
 *   51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.              *
 ***************************************************************************/

#ifndef SERIAL_DEBUG_TOOL_H
#define SERIAL_DEBUG_TOOL_H

#include "xbeep.h"
#include "ui_xbee-ap.h"
#include <QThread>
#include <QListWidgetItem>
#include <QDialog>
#include <QCloseEvent>

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
