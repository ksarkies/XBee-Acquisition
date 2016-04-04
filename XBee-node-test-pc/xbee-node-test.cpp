/** @brief        XBee Node Test

@detail Implementation of main dialogue class and application code.

This emulates the environment in which the test code would normally run, and
calls user defined functions from mainprog.cpp that provide the code to
be tested.

A timer is emulated allowing a timer to be initialised in the test code and the
ISR called.

mainprogInit() provides the initialization part
mainprog() provides the operational part that would appear in an infinite loop

NOTE: gcc required to provide function override for timerISR.
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

// Specify an intercharacter timeout when receiving incoming communications
#define TIMEOUTCOUNT 50

#include <QApplication>
#include <QString>
#include <QLabel>
#include <QMessageBox>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QCloseEvent>
#include <QFileInfo>
#include <QDebug>
#include <QBasicTimer>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include "xbee-node-test.h"

extern void mainprog();
extern void mainprogInit();

/*---------------------------------------------------------------------------*/
/* Timer ISR to be simulated */
void _timerTick();
static unsigned int timerTickMs;
static unsigned int timerTickCount;

// The timer ISR is defined in mainprog.cpp, or is substituted with a null call
__attribute__((weak)) void timerISR() {}

//*****************************************************************************
/** @brief Code Run

This is where the actual test code is run. The test code is split to
an initialisation part and an operational part which falls within an (almost)
infinite loop that is emulated here.
*/
void XbeeNodeTest::codeRun()
{
    mainprogInit();                 // Initialization section

    while (running)
    {
        qApp->processEvents();      // Allow other processes a look in

/* Ensure time ticks over for calls to an emulated ISR */
        _timerTick();

/* Place test code to be run here. Any of the QT serial access functions should
include a call to processEvents to ensure that the desired action is taken. */
        
        mainprog();

    }
    reject();
}

//*****************************************************************************
/** Constructor

@param[in] p Serial Port object pointer
@param[in] parent Parent widget.
*/

XbeeNodeTest::XbeeNodeTest(QString* p, uint initialBaudrate,bool commandLine,
                              bool debug,QWidget* parent): QDialog(parent)
{
    port = new QSerialPort(*p);
    port->open(QIODevice::ReadWrite);
    commandLineOnly = commandLine;
    debugMode = debug;
    if (debugMode) qDebug() << "Debug Mode";
// Set up the GUI if we are not using the command line
    if (success())
    {
        running = true;
        if (commandLineOnly) codeRun();
        else
        {
            running = false;
    // Build the User Interface display from the Ui class in ui_mainwindowform.h
            xbeeNodeTestFormUi.setupUi(this);
            xbeeNodeTestFormUi.debugModeCheckBox->setChecked(debugMode);
            xbeeNodeTestFormUi.errorMessage->setVisible(false);
            setComboBoxes();
            debugMode = xbeeNodeTestFormUi.debugModeCheckBox->isChecked();
        }
    }
}

XbeeNodeTest::~XbeeNodeTest()
{
    port->close();
}

//*****************************************************************************
/** @brief Successful Port Open

@returns true if the serial port was opened.
*/
bool XbeeNodeTest::success()
{
    return (port->error() == QSerialPort::NoError);
}
//*****************************************************************************
/** @brief Error Message

@returns a message when the device didn't respond properly.
*/
QString XbeeNodeTest::error()
{
    switch (port->error())
    {
        case QSerialPort::DeviceNotFoundError:
            errorMessage = "No such device";
            break;
        default:
            errorMessage = "Unknown Error";
    }
    return errorMessage;
}

//*****************************************************************************
/** @brief Close when "Quit" is activated.

No further action is taken.
*/

void XbeeNodeTest::on_quitButton_clicked()
{
    running = false;
}

//*****************************************************************************
/** @brief Setup Serial ComboBoxes

Test existence of serial ports (ACM and USB) and build both combobox entries
with ttyS0 for machines with a serial port.
*/

void XbeeNodeTest::setComboBoxes()
{
    QString port;
    xbeeNodeTestFormUi.serialComboBox->clear();
    port = "/dev/ttyS0";
    xbeeNodeTestFormUi.serialComboBox->insertItem(0,port);
    for (int i=3; i>=0; i--)
    {
        port = "/dev/ttyUSB"+QString::number(i);
        QFileInfo checkUSBFile(port);
        if (checkUSBFile.exists())
            xbeeNodeTestFormUi.serialComboBox->insertItem(0,port);
    }
    for (int i=3; i>=0; i--)
    {
        port = "/dev/ttyACM"+QString::number(i);
        QFileInfo checkACMFile(port);
        if (checkACMFile.exists())
            xbeeNodeTestFormUi.serialComboBox->insertItem(0,port);
    }
    xbeeNodeTestFormUi.serialComboBox->setCurrentIndex(0);

    QStringList baudrates;
    baudrates << "1200" <<"2400" << "4800" << "9600" << "19200" << "38400"
              << "57600" << "115200";
    xbeeNodeTestFormUi.baudrateComboBox->addItems(baudrates);
    xbeeNodeTestFormUi.baudrateComboBox->setCurrentIndex(BAUDRATE);
}

//*****************************************************************************
/** @brief Run the XBee code loop.

*/

void XbeeNodeTest::on_runButton_clicked()
{
    qDebug() << "Running";
    running = true;
    codeRun();
}

//*****************************************************************************
/** @brief Send a single character command to the serial port.

@param[in] const char command. A single character.
*/

void XbeeNodeTest::sendCommand(const char command)
{
    port->putChar(command);
    qApp->processEvents();          // Allow send and receive to occur
}

/****************************************************************************/
/* Timer initialization

This function is only called in mainprog.cpp if needed for timer initialization.

@param[in] timerTrigger. Count at which a defined ISR is triggered.
*/

void timerInit(unsigned int timerTrigger)
{
    timerTickMs = 0;
    timerTickCount = timerTrigger;
}

/****************************************************************************/
/* Timer tick

This counts off a number of milliseconds until the selected timer count
has been completed, then calls the ISR as would happen with a hardware timer.

It must be called in the main program loop.
*/

void _timerTick()
{
    usleep(1000);
    if (timerTickMs++ == timerTickCount)
    {
        timerTickMs = 0;
        timerISR();
    }
}

//*****************************************************************************

