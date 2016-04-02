/**
@mainpage XBee AVR Node Test
@version 0.0.0
@author Ken Sarkies (www.jiggerjuice.info)
@date 2 April 2016
@brief Code for a POSIX system with an XBee on a serial port 

This code forms the core of an interface between an XBee networking device
using ZigBee stack, and a data acquisition unit making a variety of
measurements for communication to a base controller.

This is designed to run on a POSIX system with an XBee on a serial port. Delays
are implemented directly in the code.

The purpose of this is to provide a testbed for code debug.

The program can be run without a GUI from the command line with the -n option.
A debug mode is available for extended printout.

xbee-node-test -d -P /dev/ttyUSB0 -b 38400 -n

In command line mode, the program can only be stopped by ctl-C or process kill.
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
#include <QMessageBox>
#include <QDebug>
#include <unistd.h>
#include "xbee-node-test.h"

//-----------------------------------------------------------------------------
/** @brief Main Program

This creates a serial port object and a programming window object.
*/

int main(int argc,char ** argv)
{
    QString serialPort = SERIAL_PORT;
    int c;
    uint initialBaudrate = 5; //!< Baudrate index to start searching
    int baudParm;
    bool commandLineOnly = false;
    bool debug = false;
    bool ok;
    QString filename;

    opterr = 0;
    while ((c = getopt (argc, argv, "w:r:s:e:P:ndvxb:")) != -1)
    {
        switch (c)
        {
        case 'P':
            serialPort = optarg;
            break;
        case 'n':
            commandLineOnly = true;
            break;
        case 'd':
            debug = true;
            break;
        case 'b':
            baudParm = atoi(optarg);
            switch (baudParm)
            {
            case 2400: initialBaudrate=0;break;
            case 4800: initialBaudrate=1;break;
            case 9600: initialBaudrate=2;break;
            case 19200: initialBaudrate=3;break;
            case 38400: initialBaudrate=4;break;
            case 57600: initialBaudrate=5;break;
            case 115200: initialBaudrate=6;break;
            default:
                fprintf (stderr, "Invalid Baudrate %i.\n", baudParm);
                return false;
            }
            break;
        case '?':
            if (optopt == 'P')
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
            default: return false;
        }
    }

    QApplication application(argc,argv);
    XbeeNodeTest xbeeNodeTest(&serialPort,initialBaudrate,commandLineOnly,debug);
    if (! commandLineOnly)
    {
        if (xbeeNodeTest.success())
        {
            xbeeNodeTest.show();
            return application.exec();
        }
        else
        {
            QMessageBox::critical(0,"Serial Port",
                  QString("Could not open Serial Port\n%1")
                                      .arg(xbeeNodeTest.error()));
             return true;
        }
    }
/* Set additional command line only actions, if any. */
    return true;

}
