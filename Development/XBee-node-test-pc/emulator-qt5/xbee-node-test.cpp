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
#include <stdio.h>
#include <cstdlib>
#include <unistd.h>
#include <iostream>
#include "xbee-node-test.h"

extern int mainprog();
extern void mainprogInit();
FILE *fp;                   /* File for results */
/* The serial port defined here is created and opened, shared with serial.cpp */
QSerialPort* port;
const qint32 bauds[8] = {1200,2400,4800,9600,19200,38400,57600,115200};

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

After initialisation of the timer the tick function is called to update the
counter. The ISR is called in that function when the limit is reached.
*/
void XbeeNodeTest::codeRun()
{
    while (running)
    {
        mainprogInit();                 // Initialization section

        while (running)
        {
            qApp->processEvents();      // Allow other processes a look in

/* Ensure time ticks over for calls to an emulated ISR, and call the ISR */
            _timerTick();

/* Place test code to be run here. Any of the QT serial access functions should
include a call to processEvents to ensure that the desired action is taken.
If the main program returns a false condition, restart from the beginning. */
        
            if (! mainprog()) break;
        }

    }
    reject();
}

//*****************************************************************************
/** Constructor

This creates the interface settings to communicate with the XBee, and
initialises some GUI elements.

@param[in] QString* p: pointer to serial port name
@param[in] uint initialBaudrate: index to baudrate array
@param[in] bool commandLine: use command line I/O only
@param[in] bool debug: print debug messages
@param[in] QString* f: pointer to log file name
@param[in] QWidget* parent: Parent widget.
*/

XbeeNodeTest::XbeeNodeTest(QString* p, uint initialBaudrate, bool commandLine,
                    bool debug, char* logFileName, QWidget* parent): QDialog(parent)
{
    baudrate = initialBaudrate;
    if (! openSerialPort(*p, initialBaudrate))
    {
        qDebug() << "Unable to open serial port" << *p;
        return;
    }
/* Attempt to open a log file */
    fp = fopen(logFileName, "a");
    if (fp == NULL)
    {
        qDebug() << "Cannot open new results file";
    }
    commandLineOnly = commandLine;
    debugMode = debug;
    if (debugMode) qDebug() << "Default Debug Mode";
// Set up the GUI if we are not using the command line
    if (success())
    {
        running = true;
        if (commandLineOnly)
        {
            if (debugMode) qDebug() << "Running, baud rate"<< bauds[initialBaudrate];
            codeRun();
        }
        else
        {
            running = false;
// Build the User Interface display from the Ui class in ui_mainwindowform.h
            xbeeNodeTestFormUi.setupUi(this);
            xbeeNodeTestFormUi.debugModeCheckBox->setChecked(debugMode);
            xbeeNodeTestFormUi.errorMessage->setVisible(false);
            setComboBoxes(initialBaudrate);
            debugMode = xbeeNodeTestFormUi.debugModeCheckBox->isChecked();
        }
    }
}

XbeeNodeTest::~XbeeNodeTest()
{
    port->close();
}

//*****************************************************************************
/** @brief Create Serial Port Object

@param[in] QString serialPort: device name for serial port.
*/
bool XbeeNodeTest::openSerialPort(QString serialPort, int baudrate)
{
    if (port != NULL) delete port;
    port = new QSerialPort(serialPort);
    bool ok = port->open(QIODevice::ReadWrite);
    if (ok)
    {
        port->setBaudRate(bauds[baudrate]);
        port->setDataBits(QSerialPort::Data8);
        port->setParity(QSerialPort::NoParity);
        port->setStopBits(QSerialPort::OneStop);
        port->setFlowControl(QSerialPort::NoFlowControl);
    }
    return ok;
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
/** @brief Setup Serial ComboBoxes

Test existence of serial ports (ACM and USB) and build both combobox entries
with ttyS0 for machines with a serial port.
*/

void XbeeNodeTest::setComboBoxes(uint initialBaudrate)
{
    QString serialPort;
    xbeeNodeTestFormUi.serialComboBox->clear();
    serialPort = "/dev/ttyS0";
    xbeeNodeTestFormUi.serialComboBox->insertItem(0,serialPort);
    for (int i=3; i>=0; i--)
    {
        serialPort = "/dev/ttyUSB"+QString::number(i);
        QFileInfo checkUSBFile(serialPort);
        if (checkUSBFile.exists())
            xbeeNodeTestFormUi.serialComboBox->insertItem(0,serialPort);
    }
    for (int i=3; i>=0; i--)
    {
        serialPort = "/dev/ttyACM"+QString::number(i);
        QFileInfo checkACMFile(serialPort);
        if (checkACMFile.exists())
            xbeeNodeTestFormUi.serialComboBox->insertItem(0,serialPort);
    }
    xbeeNodeTestFormUi.serialComboBox->setCurrentIndex(0);

    QStringList baudrates;
    baudrates << "1200" <<"2400" << "4800" << "9600" << "19200" << "38400"
              << "57600" << "115200";
    xbeeNodeTestFormUi.baudrateComboBox->addItems(baudrates);
    xbeeNodeTestFormUi.baudrateComboBox->setCurrentIndex(initialBaudrate);
}

//*****************************************************************************
/** @brief Change debug mode when button is clicked

*/
void XbeeNodeTest::on_debugModeCheckBox_clicked()
{
    debugMode = xbeeNodeTestFormUi.debugModeCheckBox->isChecked();
}

//*****************************************************************************
/** @brief Change Serial Port.

*/

void XbeeNodeTest::on_serialComboBox_activated(int index)
{
    openSerialPort(xbeeNodeTestFormUi.serialComboBox->currentText(),
                   xbeeNodeTestFormUi.baudrateComboBox->currentIndex());
}

//*****************************************************************************
/** @brief Change Baud Rate.

*/

void XbeeNodeTest::on_baudrateComboBox_activated(int newBaudrateIndex)
{
    port->setBaudRate(bauds[newBaudrateIndex]);
    if (debugMode) qDebug() << "Changed baudrate to: " << port->baudRate();
}

//*****************************************************************************
/** @brief Run the XBee code loop.

*/

void XbeeNodeTest::on_runButton_clicked()
{
    if (debugMode) qDebug() << "Running, baudrate: " << port->baudRate();
    running = true;;
    codeRun();
}

//*****************************************************************************
/** @brief Close when "Quit" is activated.

No further action is taken.
*/

void XbeeNodeTest::on_quitButton_clicked()
{
    running = false;
}

/****************************************************************************/
/* @brief Timer initialization

This function is only called in mainprog.cpp if needed for timer initialization.

@param[in] timerTrigger. Count at which a defined ISR is triggered.
*/

void timerInit(unsigned int timerTrigger)
{
    timerTickMs = 0;
    timerTickCount = timerTrigger;
}

/****************************************************************************/
/* brief Timer tick

This counts off a number of milliseconds until the selected timer count
has been completed, then calls the ISR as would happen with a hardware timer.

The number of milliseconds delay must be a multiple of 55 as the POSIX clock has
this granularity.

This is called in the outer loop of the test code in codeRun() above. However if
test code has inner loops it must be called there as well to ensure that the
timer continues to tick over.
*/

void _timerTick()
{
    usleep(55000);
    if (timerTickMs++ > timerTickCount/55)
    {
        timerTickMs = 0;
        timerISR();
    }
}

//*****************************************************************************

