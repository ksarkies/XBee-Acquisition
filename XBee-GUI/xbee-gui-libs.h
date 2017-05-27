/*
Title:    XBee Control and Display GUI Tool Common Code Library
*/

/****************************************************************************
 *   Copyright (C) 2017 by Ken Sarkies ksarkies@internode.on.net            *
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
#include <QString>
#include <QMessageBox>
#include <QtNetwork>
#include <QTcpSocket>
#include <QTextStream>
#include <QDebug>
#include <cstdlib>
#include <iostream>
#include <unistd.h>

// The commStatus is used to signal when data has arrived and what happened
enum comStatus {comIdle, comSent, comReceived, comError, comXbeeError};

/* Prototypes */

int sendCommand(QByteArray *command, QTcpSocket *tcpSocket);
void ssleep(int seconds);
comStatus getComStatus();
void setComStatus(comStatus status);

