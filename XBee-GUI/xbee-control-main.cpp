/**
@mainpage XBee Control and Display GUI Tool
@version 1.0
@author Ken Sarkies (www.jiggerjuice.net)
@date 14 January 2013

This program provides a GUI access to a process xbee_acqcontrol which is
listening on TCP port 58532 on an Internet connected machine. This latter
process controls an XBee data acquisition network.

The XBee Control and Display GUI Tool begins by reading a table of information
for XBee nodes connected to the network. Thereafter specific nodes may be
selected and queried, configured or controlled.

@note
Compiler: gcc (Ubuntu/Linaro 4.6.3-1ubuntu5) 4.6.3
@note
Uses: Qt version 4.8.1
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

#include <unistd.h>
#include "xbee-control.h"
#include <QApplication>
#include <QMessageBox>

//-----------------------------------------------------------------------------
/** @brief XBee Control Tool Main Program

*/

int main(int argc,char ** argv)
{
/* Interpret any command line options */
    char c;
    opterr = 0;
    QString tcpAddress = DEFAULT_TCP_ADDRESS;
    uint tcpPort = DEFAULT_TCP_PORT;
    while ((c = getopt (argc, argv, "a:p:")) != -1)
    {
        switch (c)
        {
// TCP address
        case 'a':
            tcpAddress = optarg;
            break;
// TCP port number
        case 'p':
            tcpPort = atoi(optarg);
            break;
// Unknown
        case '?':
            if ((optopt == 'a') || (optopt == 'p'))
                fprintf (stderr, "Option -%c requires an argument.\n", optopt);
            else if (isprint (optopt))
                fprintf (stderr, "Unknown option `-%c'.\n", optopt);
            else
                fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
            default: return false;
        }
    }
    QApplication application(argc,argv);
    XbeeControlTool xbeeControlTool(tcpAddress,tcpPort);
    if (xbeeControlTool.success())
    {
        xbeeControlTool.show();
        return application.exec();
    }
    else
        QMessageBox::critical(0,"Unable to connect to XBee control process",
              QString("%1").arg(xbeeControlTool.error()));
    return false;
}
