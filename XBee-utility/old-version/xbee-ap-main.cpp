/**
@mainpage XBee AP Mode Command Tool
@version 1.0
@author Ken Sarkies (www.jiggerjuice.net)
@date 16 December 2012

A utility to assemble and send API commands to an XBee on a serial or
USB-serial port.

The responses are collected and interpreted.

@note
Compiler: gcc (Ubuntu 4.4.1-4ubuntu9) 4.4.1
@note
Uses: Qt version 4.5.2
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

#include "xbee-ap.h"
#include "serialport.h"
#include <QApplication>
#include <QMessageBox>

// Default serial port
#define SERIAL_PORT "/dev/ttyUSB0"

//-----------------------------------------------------------------------------
/** @brief Serial Debug Tool Main Program

This creates a serial port object and a programming window object.
*/

int main(int argc,char ** argv)
{
    QApplication application(argc,argv);
    SerialPort port(SERIAL_PORT);
    XbeeApTool xbeeApTool(&port);
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
