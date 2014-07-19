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

