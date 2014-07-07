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

//-----------------------------------------------------------------------------
/** XBee-AP-UI Initialiser

*/

XbeeApTool::XbeeApTool(SerialPort* p, QWidget* parent) : QDialog(parent)
{
    port = p;
    synchronized =false;
    baudrate = 2;
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
                    "Check the connections and power.);
    return;
// Setup the AP Command selector    
}

