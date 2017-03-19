/*
Title:    XBee Node Test
*/

/****************************************************************************
 *   Copyright (C) 2016 by Ken Sarkies (www.jiggerjuice.info)               *
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

#ifndef XBEE_NODE_TEST_H_QT5
#define XBEE_NODE_TEST_H_QT5

#include <QDialog>
#include <QCloseEvent>
#include <QSerialPort>
#include "ui_xbee-node-test.h"

/* Port defaults 38400 baud */
#define INITIAL_BAUDRATE    5
#define SERIAL_PORT         "/dev/ttyUSB0"

#define LOG_FILE            "../xbee-node-test.dat"

/* Choose whether to use hardware flow control for serial comms. */
//#define USE_HARDWARE_FLOW

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
    XbeeNodeTest(QString* p, uint initialBaudrate, bool commandLine,
                        bool debug, char* logFileName, QWidget* parent = 0);
    ~XbeeNodeTest();
    bool success();
    QString error();
private slots:
    void on_debugModeCheckBox_clicked();
    void on_baudrateComboBox_activated(int newBaudrate);
    void on_serialComboBox_activated(int newBaudrate);
    void on_runButton_clicked();
    void on_quitButton_clicked();
private:
// User Interface object
    Ui::XbeeNodeTestDialog xbeeNodeTestFormUi;
    bool openSerialPort(QString serialPort, int baudrate);
    void setComboBoxes(uint initialBaudrate);
    void codeRun();             // This is where the actual test code is run

    QString errorMessage;       //!< Messages for the calling application
    bool commandLineOnly;
    bool debugMode;
    bool running;
    int baudrate;
};

#endif
