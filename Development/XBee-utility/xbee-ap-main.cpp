/**
@mainpage XBee AP Mode Command Tool
@version 1.0
@author Ken Sarkies (www.jiggerjuice.net)
@date 16 December 2012
@adte 9 March 2016

A utility to assemble and send API commands to an XBee on a serial or
USB-serial port.

The responses are collected and interpreted.

@note
Compiler: gcc
@note
Uses: Qt version 5
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
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "xbee-ap.h"

//-----------------------------------------------------------------------------
/** @brief XBee Tool Main Program

*/

int main(int argc,char ** argv)
{
    QString serialPort = SERIAL_PORT;
    int c;
    uint initialBaudrate = BAUD_RATE; //!< Baudrate index to start searching
    int baudParm;
    QString filename;

    opterr = 0;
    while ((c = getopt (argc, argv, "P:b:")) != -1)
    {
        switch (c)
        {
        case 'P':
            serialPort = optarg;
            break;
        case 'b':
            baudParm = atoi(optarg);
            switch (baudParm)
            {
            case 1200: initialBaudrate=0;break;
            case 2400: initialBaudrate=1;break;
            case 4800: initialBaudrate=2;break;
            case 9600: initialBaudrate=3;break;
            case 19200: initialBaudrate=4;break;
            case 38400: initialBaudrate=5;break;
            case 57600: initialBaudrate=6;break;
            case 115200: initialBaudrate=7;break;
            default:
                fprintf (stderr, "Invalid Baudrate %i.\n", baudParm);
                return false;
            }
            break;
        case '?':
            if ((optopt == 'P') || (optopt == 'b'))
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
            default: return false;
        }
    }

    QApplication application(argc,argv);
    XbeeApTool xbeeApTool(&serialPort,initialBaudrate);
    if (xbeeApTool.success())
    {
        xbeeApTool.show();
        return application.exec();
    }
    else
        QMessageBox::critical(0,"Serial Port Problem",
              QString("%1").arg(xbeeApTool.error()));
    return false;
}
