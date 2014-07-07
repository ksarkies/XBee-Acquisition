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
#define _TTY_POSIX_         // Need to tell qextserialport we are in POSIX

#include "ui_xbee-ap.h"
#include "serialport.h"
#include <QThread>
#include <QListWidgetItem>
#include <QDialog>
#include <QCloseEvent>

//-----------------------------------------------------------------------------
/** @brief Serial Debug Tool Control Window.

*/

class XbeeApTool : public QDialog
{
    Q_OBJECT
public:
    XbeeApTool(SerialPort*, QWidget* parent = 0);
    ~XbeeApTool();
    bool success();
    QString error();
private slots:
    void on_sendButton_clicked();
    void on_clearButton_clicked();
    void closeEvent(QCloseEvent*);
    void on_baudRate_currentIndexChanged(int index);
    void on_atCommandList_currentIndexChanged(int index);
private:
// User Interface object instance
    Ui::XbeeApDialog XbeeApFormUi;
// methods
    QString getSerialData();
    QString convertASCII(QString);
    void hideWidgets();
    void buildPacket();
//Variables
    uchar packet[256];
    uchar frameID;              // Frame ID changes on each packet
    uint length;                // packet length
    uint line;
    uint atCommand;
    uint baudrate;              // Start baudrate off at 38400
    bool synchronized;
    QString errorMessage;
    SerialPort* port;           //!< Serial port object pointer
};

#endif
