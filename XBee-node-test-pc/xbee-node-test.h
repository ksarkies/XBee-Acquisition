/*
Title:    XBee Node Test
*/

/****************************************************************************
 *   Copyright (C) 2016 by Ken Sarkies ksarkies@internode.on.net            *
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

#ifndef XBEE_NODE_TEST_H
#define XBEE_NODE_TEST_H

#include <QDialog>
#include <QCloseEvent>
#include <QSerialPort>
#include "ui_xbee-node-test.h"

/* Port defaults 38400 baud */
#define BAUDRATE    5
#define SERIAL_PORT "/dev/ttyUSB0"

//-----------------------------------------------------------------------------
/* External variable needed for access by serial emulation code */
extern QSerialPort* port;          //!< Serial port object pointer

//-----------------------------------------------------------------------------
/** @brief 
*/

class XbeeNodeTest : public QDialog
{
    Q_OBJECT
public:
    XbeeNodeTest(QString*, uint initialBaudrate,bool commandLine,
                        bool debug,QWidget* parent = 0);
    ~XbeeNodeTest();
    bool success();
    QString error();
private slots:
    void on_quitButton_clicked();
    void on_runButton_clicked();
private:
// User Interface object
    Ui::XbeeNodeTestDialog xbeeNodeTestFormUi;
    void setComboBoxes();
    void codeRun();             // This is where the actual test code is run

    QString errorMessage;       //!< Messages for the calling application
    bool commandLineOnly;
    bool debugMode;
    bool running;
};

#endif
