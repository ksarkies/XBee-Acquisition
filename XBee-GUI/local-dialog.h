/*       XBee Control and Display GUI Tool

This provides a GUI interface to an XBee acquisition process running on the local
or remote Internet connected PC or Linux based controller for the XBee acquisition
network.
*/
/****************************************************************************
 *   Copyright (C) 2013 by Ken Sarkies ksarkies@internode.on.net            *
 *                                                                          *
 *   This file is part of xbee-control                                      *
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

#ifndef LOCAL_DIALOG_H
#define LOCAL_DIALOG_H

#include "ui/ui_local-dialog.h"
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

class LocalDialog : public QDialog
{
    Q_OBJECT
public:
    LocalDialog(QString address, int row, bool remote, QWidget* parent = 0);
    ~LocalDialog();
private slots:
    void readXbeeProcess();
    void displayError(QAbstractSocket::SocketError socketError);
    int sendCommand(QByteArray command);
    int sendAtCommand(QByteArray atCommand, bool remote, int count);
    void on_refreshDisplay_clicked();
    void on_netResetButton_clicked();
    void on_softResetButton_clicked();
    void on_disassociateButton_clicked();
    void on_writeValuesButton_clicked();
    void closeEvent(QCloseEvent *event);
    void accept();
private:
// Methods
    Ui::LocalDialog localDialogFormUi;
    void setIOBoxes();
// Variables
    QTcpSocket *tcpSocket;
    int row;
    bool remote;
    Qt::CheckState channelVerifyCurrent;
    Qt::CheckState joinNotificationCurrent;
    char oldSleepMode;
    char deviceType;
// The commStatus is used to signal when data has arrived and what happened
    enum {comIdle, comSent, comReceived, comError, comXbeeError} comStatus;
    char comCommand;
    char response;
    QByteArray replyBuffer;
    int d0Index;
    int d1Index;
    int d2Index;
    int d3Index;
    int d4Index;
    int d5Index;
    int d6Index;
    int d7Index;
    int d10Index;
    int d11Index;
    int d12Index;
    int samplePeriod;
    uint changeDetect;
    uint pullUp;
};

#endif
