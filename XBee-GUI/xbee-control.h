/*
Title:    XBee Control and Display GUI Tool
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

#ifndef XBEE_CONTROL_TOOL_H
#define XBEE_CONTROL_TOOL_H

#define DEBUG   1

#include "ui_xbee-control.h"
#include <QTcpSocket>
#include <QString>
#include <QStandardItemModel>
#include <QFile>

#define DEFAULT_TCP_PORT    58532        // port for the external command I/F
#define DEFAULT_TCP_ADDRESS "127.0.0.1"

#ifdef WIN32
#include <Windows.h>
#define millisleep(a) Sleep(a)
#else
#define millisleep(a) usleep(a*1000)
#endif

//-----------------------------------------------------------------------------
/** @brief XBee Control Window.

*/

class XbeeControlTool : public QWidget
{
    Q_OBJECT
public:
    XbeeControlTool(QString tcpAddress, uint tcpPort, QWidget* parent = 0);
    ~XbeeControlTool();
    bool success();
    QString error();
private slots:
    void on_tcpConnectButton_clicked();
    bool on_refreshListButton_clicked();
    void on_removeNodeButton_clicked();
    void on_queryNodeButton_clicked();
    int on_firmwareButton_clicked();
    void readXbeeProcess();
    void displayError(QAbstractSocket::SocketError socketError);
    void on_configButton_clicked();
private:
// User Interface object instance
    Ui::XBeeControlWidget XbeeControlFormUi;
// Methods
    void configDialogDone(int row);
    bool findNode();
    bool setNodeAwake();
    bool validTcpSocket();
    void closeEvent(QCloseEvent*);
    int sendAtCommand(QByteArray *atCommand, QTcpSocket *tcpSocket,
                      int row, bool remote, int countMax);
    int loadHexGUI(QFile* file, int row);
// Variables
    QTcpSocket *tcpSocket;
    QString errorMessage;
    QByteArray replyBuffer;
    quint16 blockSize;
    char deviceType;
    int row;
    int timeout;
    char comCommand;
// Variables for the remote process
    int tableLength;
    QStandardItemModel *table;
    char response;
};

#endif
