/*       Node Configuration Dialogue GUI Tool

This provides a GUI interface to an XBee coordinator process running on the
local or remote Internet connected PC or Linux based controller for the XBee
acquisition network. This file manages the configuration of the remote node
through the inbuilt command interface in the node MCU.
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

#ifndef NODE_CONFIG_H
#define NODE_CONFIG_H

#include "ui/ui_node-config.h"
#include <QTcpSocket>
#include <QDialog>
#include <QCloseEvent>

#ifdef WIN32
#include <Windows.h>
#define millisleep(a) Sleep(a)
#else
#define millisleep(a) usleep(a*1000)
#endif

//-----------------------------------------------------------------------------
/** @brief Local XBee configuration dialogue

This class provides a dialogue window
*/

class NodeConfigWidget : public QWidget
{
    Q_OBJECT
public:
    NodeConfigWidget(QString address, uint tcpPort, int row,
                     int timeout, QWidget* parent = 0);
    ~NodeConfigWidget();
private slots:
    void on_closeButton_clicked();
    void closeEvent(QCloseEvent *event);
    void accept();
    void on_wakeButton_clicked();
    void on_commandButton_clicked();
    void on_sleepButton_clicked();
    void readXbeeProcess();
    void displayError(QAbstractSocket::SocketError socketError);
signals:
    void terminated(int row);
private:
// Methods
    Ui::NodeConfigWidget NodeConfigFormUi;
    int sendAtCommand(QByteArray *atCommand, QTcpSocket *tcpSocket,
                      int row, bool remote, int countMax);
    int sendString(QByteArray *command, QTcpSocket *tcpSocket,
                      int row, int timeout);
// Variables
    QTcpSocket *tcpSocket;
    int row;
    int timeout;
    QString deviceTypeString;
    QString snL;
    QString snH;
    QString nodeID;
    char oldSleepMode;
    char deviceType;
    char comCommand;
    QByteArray replyBuffer;
    char response;
};

#endif
