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

#define PORT 58532        // port for the external command I/F
#define ADDRESS "127.0.0.1"

#ifdef WIN32
#include <Windows.h>
#define millisleep(a) Sleep(a)
#else
#define millisleep(a) usleep(a*1000)
#endif

//-----------------------------------------------------------------------------
/** @brief XBee Control Window.

*/

class XbeeControlTool : public QDialog
{
    Q_OBJECT
public:
    XbeeControlTool(QWidget* parent = 0);
    ~XbeeControlTool();
    bool success();
    QString error();
private slots:
    void readXbeeProcess();
    void displayError(QAbstractSocket::SocketError socketError);
    bool on_refreshListButton_clicked();
    int on_firmwareButton_clicked();
    void closeEvent(QCloseEvent*);
    void on_configButton_clicked();
    void on_connectButton_clicked();
    void on_removeNodeButton_clicked();
    void on_queryNodeButton_clicked();
private:
// User Interface object instance
    Ui::XbeeControlDialog XbeeControlFormUi;
// Methods
    int sendCommand(QByteArray command);
    QString convertASCII(QString);
    void hideWidgets();
    int loadHexGUI(QFile* file, int row);
    void updateProgress(int progress);
    int sendAtCommand(QByteArray atCommand, bool remote, int countMax);
    int validTcpSocket();
    bool findNode();
    void ssleep(int seconds);
    void popup(QString message);
// Variables
    QTcpSocket *tcpSocket;
    QString errorMessage;
    quint16 blockSize;
    char deviceType;
    int row;
// The commStatus is used to signal when data has arrived and what happened
    enum {comIdle, comSent, comReceived, comError, comXbeeError} comStatus;
    char comCommand;
// Variables for the remote process
    int tableLength;
    QStandardItemModel *table;
    char response;
};

#endif
