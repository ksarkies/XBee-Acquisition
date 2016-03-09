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

#include "xbee-ap.h"
#include <QApplication>
#include <QMessageBox>

//-----------------------------------------------------------------------------
/** @brief XBee Tool Main Program

*/

int main(int argc,char ** argv)
{
    QApplication application(argc,argv);
    XbeeApTool xbeeApTool;
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
