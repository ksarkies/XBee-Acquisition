/*       XBee Configuration Dialogue GUI Tool

This provides a GUI interface to an XBee coordinator process running on the
local or remote Internet connected PC or Linux based controller for the XBee
acquisition network. This file manages the configuration of the coordinator
or remote XBee through reading and writing parameters via the XBee API.
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
#include <QString>
#include <QLabel>
#include <QMessageBox>
#include <QCloseEvent>
#include <QDebug>
#include <QBasicTimer>
#include <QtNetwork>
#include <QTcpSocket>
#include <QTextStream>
#include <QCloseEvent>
#include <unistd.h>
#include <cstdlib>
#include <iostream>
#include "xbee-dialog.h"
#include "xbee-control.h"
#include "xbee-gui-libs.h"

//-----------------------------------------------------------------------------
/** Constructor

@parameter int row. The table row number for the remote node.
@parameter bool remote. True if a remote node, false if local XBee.
@parameter parent Parent widget.
*/

XBeeConfigWidget::XBeeConfigWidget(QString tcpAddress, uint tcpPort,
                                   int nodeRow, bool remoteNode, int timeout,
                                   QWidget* parent)
                    : QWidget(parent), row(nodeRow), remote(remoteNode)
{
// Create the TCP socket to the internet process
    tcpSocket = new QTcpSocket(this);
// Setup QT signal/slots for reading and error handling
// The readyRead signal from the QAbstractSocket is linked to the readXbeeProcess slot
    connect(tcpSocket, SIGNAL(readyRead()), this, SLOT(readXbeeProcess()));
    connect(tcpSocket, SIGNAL(error(QAbstractSocket::SocketError)),
             this, SLOT(displayError(QAbstractSocket::SocketError)));
// Connect to the host
    tcpSocket->abort();
    tcpSocket->connectToHost(tcpAddress, tcpPort);
// Pull in all the information table
    if (! tcpSocket->waitForConnected(10000)) exit(1);
// Ask for information about the remote node as stored in the base station.
    QByteArray rowGetCommand;
    rowGetCommand.append('I');
    rowGetCommand.append(char(row));
    comCommand = rowGetCommand.at(0);
    sendCommand(&rowGetCommand, tcpSocket);

/* Remote End Device nodes must be kept awake. Send an instruction to the AVR
to keep the XBee awake. */
    if (deviceType == 2)
    {
        QMessageBox msgBox;
        msgBox.open();
        msgBox.setText("Node being contacted. Please wait.");
        qApp->processEvents();
        QByteArray stayAwakeCommand;
        stayAwakeCommand.clear();
        stayAwakeCommand.append("RDW");
        if (sendAtCommand(&stayAwakeCommand, tcpSocket, row, remote, timeout) > 0)
        {
#ifdef DEBUG
            qDebug() << "Timeout accessing remote node sleep mode";
#endif
            return;
        }
        msgBox.close();
    }
// Build the User Interface display from the Ui class
    XBeeConfigWidgetFormUi.setupUi(this);
// Setup the display with information from the addressed XBee
    XBeeConfigWidgetFormUi.perSampSpinBox->setMaximum(65);
    setIOBoxes();
    on_refreshDisplay_clicked();
}

XBeeConfigWidget::~XBeeConfigWidget()
{
}

//-----------------------------------------------------------------------------
/** @brief Quit the window.

This closes the window from the quit button and restores the sleep setting if
it was changed.
*/
void XBeeConfigWidget::on_closeButton_clicked()
{
    accept();
    close();
}

/** Also deal the same way with the close window button */
void XBeeConfigWidget::closeEvent(QCloseEvent *event)
{
    accept();
    QWidget::closeEvent(event);
}

//-----------------------------------------------------------------------------
/** @brief Quit the dialogue window.

This closes the dialogue window and restores the sleep setting if it was changed.
*/
void XBeeConfigWidget::accept()
{
    if (deviceType == 2)        // Coordinator and routers never change sleep mode
    {
/* get the currently set sleep mode. */
#ifdef DEBUG
        qDebug() << "Getting current sleep mode";
#endif
        QByteArray sleepModeCommand;
        sleepModeCommand.clear();
        sleepModeCommand.append("SM");
        if (sendAtCommand(&sleepModeCommand, tcpSocket, row, remote,500) > 0) return;
        char currentSleepMode = replyBuffer[0];

/* Change back to the original version if currently different and if no change was made
to the setting. */
        char userSleepMode = XBeeConfigWidgetFormUi.sleepModeComboBox->currentIndex();
#ifdef DEBUG
        qDebug() << "Current" << (int)currentSleepMode << "Setting" << (int)userSleepMode
                 << "Old" << (int)oldSleepMode;
#endif
        if ((userSleepMode == oldSleepMode) && (currentSleepMode != oldSleepMode))
        {
#ifdef DEBUG
            qDebug() << "Revert to old";
#endif
            sleepModeCommand.clear();
            sleepModeCommand.append("SM");
            sleepModeCommand.append(oldSleepMode);
            sendAtCommand(&sleepModeCommand, tcpSocket, row, remote,10);
        }
/* Change to the user version if it is different from both the current and the original. */
        else if ((userSleepMode != oldSleepMode) && (userSleepMode != currentSleepMode))
        {
#ifdef DEBUG
            qDebug() << "Change to user setting";
#endif
            sleepModeCommand.clear();
            sleepModeCommand.append("SM");
            sleepModeCommand.append(userSleepMode);
            sendAtCommand(&sleepModeCommand, tcpSocket, row, remote,10);
// Finally WR command to write the change to permanent memory
            QByteArray wrCommand;
            wrCommand.clear();
            wrCommand.append("WR");
            sendAtCommand(&wrCommand, tcpSocket, row, remote,10);
        }
    }
/* Emit the "terminated" signal with the row number so that the row status can
be restored */
    emit terminated(row);
}

//-----------------------------------------------------------------------------
/** @brief Reset the network Parameters.

*/
void XBeeConfigWidget::on_netResetButton_clicked()
{
    QByteArray atCommand;
    atCommand.append("NR");      // NR command to perform network reset
    atCommand.append('\0');     // This just resets the addressed XBee
    sendAtCommand(&atCommand, tcpSocket, row, remote,10);
}

//-----------------------------------------------------------------------------
/** @brief Reset the Device Software.

*/
void XBeeConfigWidget::on_softResetButton_clicked()
{
    QByteArray atCommand;
    atCommand.append("FR");      // FR command to perform software reset
    sendAtCommand(&atCommand, tcpSocket, row, remote,10);
}

//-----------------------------------------------------------------------------
/** @brief Disassociate from the network and reassociate.
*/
void XBeeConfigWidget::on_disassociateButton_clicked()
{
    QByteArray atCommand;
    atCommand.append("DA");      // DA command to disassociate
    sendAtCommand(&atCommand, tcpSocket, row, remote,10);
}

//-----------------------------------------------------------------------------
/** @brief Write newly entered values to the device memory.

If an entry widget was modified, write the new value and set in node permanent
memory.
*/
void XBeeConfigWidget::on_writeValuesButton_clicked()
{
    bool ok;
    bool changes = false;
// ID command to set the PAN ID
    QByteArray atCommand;
    if (XBeeConfigWidgetFormUi.pan->isModified())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("ID");
        QString value = XBeeConfigWidgetFormUi.pan->text();
        for (int i=0; i<16; i+=2) atCommand.append(value.mid(i,2).toUShort(&ok, 16));
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// NI command to set the node identifier
    if (XBeeConfigWidgetFormUi.nodeIdent->isModified())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("NI");
        atCommand.append(XBeeConfigWidgetFormUi.nodeIdent->text());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Join Notification Checkbox changed
    if (deviceType != 0)        // Routers and end devices only
    {
        if (joinNotificationCurrent != XBeeConfigWidgetFormUi.joinNotificationCheckBox->checkState())
        {
            changes = true;
            atCommand.clear();
            atCommand.append("JN");
            atCommand.append(XBeeConfigWidgetFormUi.joinNotificationCheckBox->isChecked() ? 1 : 0);
            sendAtCommand(&atCommand, tcpSocket, row, remote,10);
        }
    }
// Channel Verification Checkbox changed
    if (deviceType == 1)        // Routers only
    {
// NW command to set the network watchdog timer value
        if (XBeeConfigWidgetFormUi.watchdogTime->isModified())
        {
            changes = true;
            atCommand.clear();
            atCommand.append("NW");
            int watchdogTime = XBeeConfigWidgetFormUi.watchdogTime->text().toInt();
            atCommand.append((watchdogTime >> 8) & 0xFF);   // Upper byte
            atCommand.append(watchdogTime & 0xFF);          // Lower byte
            sendAtCommand(&atCommand, tcpSocket, row, remote,10);
        }
        if (channelVerifyCurrent != XBeeConfigWidgetFormUi.channelVerifyCheckBox->checkState())
        {
            changes = true;
            atCommand.clear();
            atCommand.append("JV");
            atCommand.append(XBeeConfigWidgetFormUi.channelVerifyCheckBox->isChecked() ? 1 : 0);
            sendAtCommand(&atCommand, tcpSocket, row, remote,10);
        }
    }
// Set sleep period
    if (XBeeConfigWidgetFormUi.sleepPeriod->isModified())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("SP");
        int sleepPeriod = XBeeConfigWidgetFormUi.sleepPeriod->text().toInt();
        atCommand.append((sleepPeriod >> 8) & 0xFF);   // Upper byte
        atCommand.append(sleepPeriod & 0xFF);          // Lower byte
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set sleep number of periods
    if (XBeeConfigWidgetFormUi.sleepNumber->isModified())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("SN");
        int sleepNumber = XBeeConfigWidgetFormUi.sleepNumber->text().toInt();
        atCommand.append((sleepNumber >> 8) & 0xFF);   // Upper byte
        atCommand.append(sleepNumber & 0xFF);          // Lower byte
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set sleep holdoff time
    if (deviceType == 2)
    {
        if (XBeeConfigWidgetFormUi.sleepHoldoff->isModified())
        {
            changes = true;
            atCommand.clear();
            atCommand.append("ST");
            int sleepHoldoff = XBeeConfigWidgetFormUi.sleepHoldoff->text().toInt();
            atCommand.append((sleepHoldoff >> 8) & 0xFF);   // Upper byte
            atCommand.append(sleepHoldoff & 0xFF);          // Lower byte
            sendAtCommand(&atCommand, tcpSocket, row, remote,10);
        }
    }
// Set wake delay before sending characters to allow host to receive.
    if (XBeeConfigWidgetFormUi.wakeHostDelay->isModified())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("WH");
        int wakeHostDelay = XBeeConfigWidgetFormUi.wakeHostDelay->text().toInt();
        atCommand.append((wakeHostDelay >> 8) & 0xFF);   // Upper byte
        atCommand.append(wakeHostDelay & 0xFF);          // Lower byte
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set D0
    if (d0Index != XBeeConfigWidgetFormUi.d0ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D0");
        atCommand.append(XBeeConfigWidgetFormUi.d0ComboBox->currentIndex());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set D1
    if (d1Index != XBeeConfigWidgetFormUi.d1ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D1");
        atCommand.append(XBeeConfigWidgetFormUi.d1ComboBox->currentIndex());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set D2
    if (d2Index != XBeeConfigWidgetFormUi.d2ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D2");
        atCommand.append(XBeeConfigWidgetFormUi.d2ComboBox->currentIndex());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set D3
    if (d3Index != XBeeConfigWidgetFormUi.d3ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D3");
        atCommand.append(XBeeConfigWidgetFormUi.d3ComboBox->currentIndex());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set D4
    if (d4Index != XBeeConfigWidgetFormUi.d4ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D4");
        atCommand.append(XBeeConfigWidgetFormUi.d4ComboBox->currentIndex());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set D5
    if (d5Index != XBeeConfigWidgetFormUi.d5ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D5");
        atCommand.append(XBeeConfigWidgetFormUi.d5ComboBox->currentIndex());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set D6
    if (d6Index != XBeeConfigWidgetFormUi.d6ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D6");
        atCommand.append(XBeeConfigWidgetFormUi.d6ComboBox->currentIndex());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set D7
    if (d7Index != XBeeConfigWidgetFormUi.d7ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D7");
        atCommand.append(XBeeConfigWidgetFormUi.d7ComboBox->currentIndex());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set D10
    if (d10Index != XBeeConfigWidgetFormUi.d10ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("P0");
        atCommand.append(XBeeConfigWidgetFormUi.d10ComboBox->currentIndex());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set D11
    if (d11Index != XBeeConfigWidgetFormUi.d11ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("P1");
        atCommand.append(XBeeConfigWidgetFormUi.d11ComboBox->currentIndex());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set D12
    if (d12Index != XBeeConfigWidgetFormUi.d12ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("P2");
        atCommand.append(XBeeConfigWidgetFormUi.d12ComboBox->currentIndex());
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Set sample period spinbox
    if (samplePeriod != XBeeConfigWidgetFormUi.perSampSpinBox->value()*1000)
    {
        samplePeriod = XBeeConfigWidgetFormUi.perSampSpinBox->value()*1000;
        changes = true;
        atCommand.clear();
        atCommand.append("IR");
        atCommand.append((samplePeriod >> 8) & 0xFF);   // Upper byte
        atCommand.append(samplePeriod & 0xFF);          // Lower byte
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
// Finally WR command to write to permanent memory
    if (changes)
    {
        atCommand.clear();
        atCommand.append("WR");
        sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    }
}

//-----------------------------------------------------------------------------
/** @brief Refresh parameter display.

Send some AT commands to pull in data about the local XBee settings.
The deviceType must be determined first by querying the coordinator process.

If errors occur, typically timeouts, continue the process to the end.
The user can try again.
*/
void XBeeConfigWidget::on_refreshDisplay_clicked()
{
    QByteArray atCommand;
    atCommand.append("ID");      // ID command to get PAN ID
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        XBeeConfigWidgetFormUi.pan->setText(replyBuffer.toHex());
    }
    else
        XBeeConfigWidgetFormUi.pan->setText("Error");
    atCommand.clear();
    atCommand.append("AI");      // AI command to get association indication
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        XBeeConfigWidgetFormUi.association->setText(replyBuffer.toHex());
    }
    else
        XBeeConfigWidgetFormUi.association->setText("Error");
    atCommand.clear();
    atCommand.append("NI");      // NI command to get association indication
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        XBeeConfigWidgetFormUi.nodeIdent->setText(replyBuffer);
    }
    else
        XBeeConfigWidgetFormUi.nodeIdent->setText("Error");
    atCommand.clear();
    atCommand.append("SH");      // SH command to get high serial number
    sendAtCommand(&atCommand, tcpSocket, row, remote,10);
    QString serialHigh = replyBuffer.toHex().toUpper();
    atCommand.clear();
    atCommand.append("SL");      // SL command to get low serial number
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        XBeeConfigWidgetFormUi.serial->setText(serialHigh+' '+replyBuffer.toHex().toUpper());
    }
    else
        XBeeConfigWidgetFormUi.serial->setText("Error");
    XBeeConfigWidgetFormUi.children->setEnabled(false);
    XBeeConfigWidgetFormUi.childrenLabel->setEnabled(false);
    if (deviceType != 2)            // Not end devices
    {
        atCommand.clear();
        atCommand.append("NC");      // NC command to get remaining children
        if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
        {
            XBeeConfigWidgetFormUi.children->setEnabled(true);
            XBeeConfigWidgetFormUi.childrenLabel->setEnabled(true);
            XBeeConfigWidgetFormUi.children->setText(QString("%1").arg((int)replyBuffer[0]));
        }
    }
    XBeeConfigWidgetFormUi.channelVerifyCheckBox->setEnabled(false);
    XBeeConfigWidgetFormUi.channelVerifyLabel->setEnabled(false);
    XBeeConfigWidgetFormUi.watchdogTime->setEnabled(false);
    XBeeConfigWidgetFormUi.watchdogTimeLabel->setEnabled(false);
    if (deviceType == 1)            // Routers only
    {
        channelVerifyCurrent = XBeeConfigWidgetFormUi.channelVerifyCheckBox->checkState();
        atCommand.clear();
        atCommand.append("JV");      // JV command to get channel verify setting
        if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
        {
            XBeeConfigWidgetFormUi.channelVerifyCheckBox->setEnabled(true);
            XBeeConfigWidgetFormUi.channelVerifyLabel->setEnabled(true);
            if (replyBuffer[0] == '\0')
                XBeeConfigWidgetFormUi.channelVerifyCheckBox->setCheckState(Qt::Unchecked);
            else XBeeConfigWidgetFormUi.channelVerifyCheckBox->setCheckState(Qt::Checked);
            channelVerifyCurrent = XBeeConfigWidgetFormUi.channelVerifyCheckBox->checkState();
        }
        atCommand.clear();
        atCommand.append("NW");      // NW command to get network watchdog timeout
        if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
        {
            QString networkWatchdog = QString("%1")
                    .arg((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)),0,10);
            XBeeConfigWidgetFormUi.watchdogTime->setEnabled(true);
            XBeeConfigWidgetFormUi.watchdogTimeLabel->setEnabled(true);
            XBeeConfigWidgetFormUi.watchdogTime->setText(networkWatchdog);
        }
    }
    XBeeConfigWidgetFormUi.joinNotificationCheckBox->setEnabled(false);
    XBeeConfigWidgetFormUi.joinNotificationLabel->setEnabled(false);
    XBeeConfigWidgetFormUi.sleepModeComboBox->setEnabled(false);
    XBeeConfigWidgetFormUi.sleepModeLabel->setEnabled(false);
    if (deviceType != 0)                // Not coordinator or router. Leave options disabled
    {
        atCommand.clear();
        atCommand.append("JN");         // JN command to get Join Notification
        if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
        {
            XBeeConfigWidgetFormUi.joinNotificationCheckBox->setEnabled(true);
            XBeeConfigWidgetFormUi.joinNotificationLabel->setEnabled(true);
            if (replyBuffer[0] == '\0')
                XBeeConfigWidgetFormUi.joinNotificationCheckBox->setCheckState(Qt::Unchecked);
            else XBeeConfigWidgetFormUi.joinNotificationCheckBox->setCheckState(Qt::Checked);
            joinNotificationCurrent = XBeeConfigWidgetFormUi.joinNotificationCheckBox->checkState();
        }
    }
    if (deviceType == 2)                // Not coordinator or router. Leave options disabled
    {
// For routers and end devices, the sleep mode has already been read in as oldSleepMode.
        XBeeConfigWidgetFormUi.sleepModeComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.sleepModeLabel->setEnabled(true);
        XBeeConfigWidgetFormUi.sleepModeComboBox->setCurrentIndex(oldSleepMode);
    }
    atCommand.clear();
    atCommand.append("SP");      // SP command to get sleep period
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        QString sleepPeriod = QString("%1")
                    .arg((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)),0,10);
        XBeeConfigWidgetFormUi.sleepPeriod->setText(sleepPeriod);
    }
    atCommand.clear();
    atCommand.append("SN");      // SP command to get sleep number
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        QString sleepNumber = QString("%1")
                    .arg((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)),0,10);
        XBeeConfigWidgetFormUi.sleepNumber->setText(sleepNumber);
    }
    atCommand.clear();
    atCommand.append("ST");      // ST command to get sleep holdoff time
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        QString sleepHoldoff = QString("%1")
                    .arg((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)),0,10);
        XBeeConfigWidgetFormUi.sleepHoldoff->setText(sleepHoldoff);
    }
    atCommand.clear();
    atCommand.append("WH");      // SP command to get wake delay time
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        QString wakeHostDelay = QString("%1")
                    .arg((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)),0,10);
        XBeeConfigWidgetFormUi.wakeHostDelay->setText(wakeHostDelay);
    }
    atCommand.clear();
    atCommand.append("OP");      // OP command to get operational PAN ID
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        XBeeConfigWidgetFormUi.opPan->setText(replyBuffer.toHex());
    }
    else
        XBeeConfigWidgetFormUi.opPan->setText("Error");
    atCommand.clear();
    atCommand.append("PL");      // PL command to get power level
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        XBeeConfigWidgetFormUi.powerLevel->setText(replyBuffer.toHex());
    }
    else
        XBeeConfigWidgetFormUi.powerLevel->setText("Error");
    atCommand.clear();
    atCommand.append("PM");      // PM command to get power boost
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        if (replyBuffer.at(0) == 1)
            XBeeConfigWidgetFormUi.powerBoost->setText("Boost");
        else
            XBeeConfigWidgetFormUi.powerBoost->setText("");
    }
    atCommand.clear();
    atCommand.append("DB");      // DB command to get RSS
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        XBeeConfigWidgetFormUi.rxLevel->setText(QString("-%1 dBm").arg((int)replyBuffer[0]));
    }
    else
        XBeeConfigWidgetFormUi.rxLevel->setText("Error");
    atCommand.clear();
    atCommand.append("CH");      // CH command to get Channel
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        XBeeConfigWidgetFormUi.channel->setText(replyBuffer.toHex().toUpper());
    }
    else
        XBeeConfigWidgetFormUi.channel->setText("Error");
    atCommand.clear();
    atCommand.append("D0");      // D0 command to get GPIO port 0 setting
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        d0Index = (int)replyBuffer[0];
        XBeeConfigWidgetFormUi.d0ComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.d0ComboBox->setCurrentIndex(d0Index);
    }
    else
    {
        XBeeConfigWidgetFormUi.d0ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D1");      // D1 command to get GPIO port 0 setting
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        d1Index = (int)replyBuffer[0];
        XBeeConfigWidgetFormUi.d1ComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.d1ComboBox->setCurrentIndex(d1Index);
    }
    else
    {
        XBeeConfigWidgetFormUi.d1ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D2");      // D2 command to get GPIO port 0 setting
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        d2Index = (int)replyBuffer[0];
        XBeeConfigWidgetFormUi.d2ComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.d2ComboBox->setCurrentIndex(d2Index);
    }
    else
    {
        XBeeConfigWidgetFormUi.d2ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D3");      // D3 command to get GPIO port 0 setting
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        d3Index = (int)replyBuffer[0];
        XBeeConfigWidgetFormUi.d3ComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.d3ComboBox->setCurrentIndex(d3Index);
    }
    else
    {
        XBeeConfigWidgetFormUi.d3ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D4");      // D4 command to get GPIO port 0 setting
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        d4Index = (int)replyBuffer[0];
        XBeeConfigWidgetFormUi.d4ComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.d4ComboBox->setCurrentIndex(d4Index);
    }
    else
    {
        XBeeConfigWidgetFormUi.d4ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D5");      // D5 command to get GPIO port 0 setting
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        d5Index = (int)replyBuffer[0];
        XBeeConfigWidgetFormUi.d5ComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.d5ComboBox->setCurrentIndex(d5Index);
    }
    else
    {
        XBeeConfigWidgetFormUi.d5ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D6");      // D6 command to get GPIO port 0 setting
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        d6Index = (int)replyBuffer[0];
        XBeeConfigWidgetFormUi.d6ComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.d6ComboBox->setCurrentIndex(d6Index);
    }
    else
    {
        XBeeConfigWidgetFormUi.d6ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D7");      // D7 command to get GPIO port 0 setting
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        d7Index = (int)replyBuffer[0];
        XBeeConfigWidgetFormUi.d7ComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.d7ComboBox->setCurrentIndex(d7Index);
    }
    else
    {
        XBeeConfigWidgetFormUi.d7ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("P0");      // D10 command to get GPIO port 0 setting
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        d10Index = (int)replyBuffer[0];
        XBeeConfigWidgetFormUi.d10ComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.d10ComboBox->setCurrentIndex(d10Index);
    }
    else
    {
        XBeeConfigWidgetFormUi.d10ComboBox->setEnabled(false);
        XBeeConfigWidgetFormUi.d10ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("P1");      // D11 command to get GPIO port 0 setting
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        d11Index = (int)replyBuffer[0];
        XBeeConfigWidgetFormUi.d11ComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.d11ComboBox->setCurrentIndex(d11Index);
    }
    else
    {
        XBeeConfigWidgetFormUi.d11ComboBox->setEnabled(false);
        XBeeConfigWidgetFormUi.d11ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("P2");      // D12 command to get GPIO port 0 setting
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        d12Index = (int)replyBuffer[0];
        XBeeConfigWidgetFormUi.d12ComboBox->setEnabled(true);
        XBeeConfigWidgetFormUi.d12ComboBox->setCurrentIndex(d12Index);
    }
    else
    {
        XBeeConfigWidgetFormUi.d12ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("IR");      // IR command to get sample period
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        samplePeriod = ((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF);
        XBeeConfigWidgetFormUi.perSampSpinBox->setEnabled(true);
        XBeeConfigWidgetFormUi.perSampSpinBox->setValue(samplePeriod/1000);
    }
    else
    {
        XBeeConfigWidgetFormUi.perSampSpinBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("IC");      // IC command to get change detection
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        changeDetect = ((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)) << 1);
        XBeeConfigWidgetFormUi.checkBoxC0->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxC1->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxC2->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxC3->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxC4->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxC5->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxC6->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxC7->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxC10->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxC11->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxC12->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxC0->setChecked((changeDetect <<= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxC1->setChecked((changeDetect <<= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxC2->setChecked((changeDetect <<= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxC3->setChecked((changeDetect <<= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxC4->setChecked((changeDetect <<= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxC5->setChecked((changeDetect <<= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxC6->setChecked((changeDetect <<= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxC7->setChecked((changeDetect <<= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxC10->setChecked((changeDetect <<= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxC11->setChecked((changeDetect <<= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxC12->setChecked((changeDetect <<= 1) & 0x01);
    }
    else
    {
        XBeeConfigWidgetFormUi.checkBoxC0->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxC1->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxC2->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxC3->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxC4->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxC5->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxC6->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxC7->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxC10->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxC11->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxC12->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("PR");      // PR command to get pullup resistor
    if (sendAtCommand(&atCommand, tcpSocket, row, remote,10) == 0)
    {
        pullUp = ((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)) << 1);
        XBeeConfigWidgetFormUi.checkBoxP0->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP1->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP2->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP3->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP4->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP5->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP6->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP7->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP8->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP9->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP10->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP11->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP12->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP13->setEnabled(true);
        XBeeConfigWidgetFormUi.checkBoxP4->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP3->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP2->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP1->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP0->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP6->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP8->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP13->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP5->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP9->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP12->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP10->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP11->setChecked((pullUp >>= 1) & 0x01);
        XBeeConfigWidgetFormUi.checkBoxP7->setChecked((pullUp >>= 1) & 0x01);
    }
    else
    {
        XBeeConfigWidgetFormUi.checkBoxP0->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP1->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP2->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP3->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP4->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP5->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP6->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP7->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP8->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP9->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP10->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP11->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP12->setEnabled(false);
        XBeeConfigWidgetFormUi.checkBoxP13->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
/** @brief Pull in the return data from the Internet Process.

This interprets the echoed commands and performs most of the processing.
*/
void XBeeConfigWidget::readXbeeProcess()
{
    QByteArray reply = tcpSocket->readAll();
    int length = reply[0];
    char command = reply[1];
    int status = reply[2];
    while (length > reply.size())
    {
// Wait for any more data up to 5 seconds, then bomb out if none
        if (tcpSocket->waitForReadyRead(5000))
            reply.append(tcpSocket->readAll());
        else
        {
            setComStatus(comError);
            return;
        }
    }
#ifdef DEBUG
    if (command != 'r')             // Don't print remote check mesasges
    {
        if (reply.size() > 0)
            qDebug() << "Config sendCommand response received: length" << length
                     << "Status" << status << "Command" << command;
        else qDebug() << "Config sendCommand Null Response";
    }
#endif
// The command sent should match the received echo.
    if (command != comCommand)
    {
        setComStatus(comError);
        return;
    }
// Start command interpretation
    switch (command)
    {
        case 'l':
        case 'r':
            response = reply[2];
            for (int i = 3; i < reply.size(); i++)
            {
                replyBuffer[i-3] = reply[i];
            }
            break;
        case 'I':
            deviceType = reply[13];
            break;
    }
    setComStatus(comReceived);
}

//-----------------------------------------------------------------------------
/** @brief Notify of connection failure.

This is a slot.
*/

void XBeeConfigWidget::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError) {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, "XBee GUI",
                                 "The host was not found.");
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, "XBee GUI",
                                 "The connection was refused by the peer.");
        break;
    default:
        QMessageBox::information(this, QString("XBee GUI"),
                                 QString("The following error occurred: %1.")
                                 .arg(tcpSocket->errorString()));
    }
}

//-----------------------------------------------------------------------------
/** @brief Send an AT command over TCP connection and wait for response.

The command must be formatted as for the XBee AT commands. When a response is
checked, the result is returned in "response" with more detail provided in
"replyBuffer".

Globals: row, response, replyBuffer

@parameter  atCommand-> The AT command string as a QByteArray to send
@parameter  remote. True if the node is a remote node
@parameter  countMax. Number of 100ms delays in the wait loop.
@return 0 no error
        1 socket error sending command (usually timeout)
        2 timeout waiting for response
        3 socket error sending response (usually timeout)
*/

int XBeeConfigWidget::sendAtCommand(QByteArray *atCommand, QTcpSocket *tcpSocket,
                                    int row, bool remote, int countMax)
{
    int errorCode = 0;
    if (remote)
    {
        atCommand->prepend(row);     // row
        atCommand->prepend('R');
    }
    else
    {
        atCommand->prepend('\0');     // Dummy "row" value
        atCommand->prepend('L');
    }
    comCommand = atCommand->at(0);
    if (sendCommand(atCommand,tcpSocket) > 0) errorCode = 1;
    else
    {
/* Ask for a confirmation or error code */
        replyBuffer.clear();
        atCommand->clear();
        if (remote)
            atCommand->append('r');
        else
            atCommand->append('l');
        atCommand->append('\0');         // Dummy "row" value
        int count = 0;
        response = 0;
/* Query node every 100ms until response is received or an error occurs. */
        QMessageBox msgBox;
        msgBox.open();
        msgBox.setText("Node being contacted. Please wait.");
        while((response == 0) && (errorCode == 0))
        {
            if ((countMax > 0) && (count++ > countMax)) errorCode = 2;  //Timeout
            comCommand = atCommand->at(0);
            if (sendCommand(atCommand,tcpSocket) > 0) errorCode = 3;
            if (remote) ssleep(1);
        }
        msgBox.close();
    }
    return errorCode;
}

//-----------------------------------------------------------------------------
/** @brief Setup I/O configuration comboboxes.

*/
void XBeeConfigWidget::setIOBoxes()
{
    XBeeConfigWidgetFormUi.d0ComboBox->addItem("Disabled");
    XBeeConfigWidgetFormUi.d0ComboBox->addItem("Commission");
    XBeeConfigWidgetFormUi.d0ComboBox->addItem("Analogue In");
    XBeeConfigWidgetFormUi.d0ComboBox->addItem("Digital In");
    XBeeConfigWidgetFormUi.d0ComboBox->addItem("Digital Low");
    XBeeConfigWidgetFormUi.d0ComboBox->addItem("Digital High");
    XBeeConfigWidgetFormUi.d0ComboBox->setCurrentIndex((int)replyBuffer[0]);
    XBeeConfigWidgetFormUi.d1ComboBox->addItem("Disabled");
    XBeeConfigWidgetFormUi.d1ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d1ComboBox->addItem("Analogue In");
    XBeeConfigWidgetFormUi.d1ComboBox->addItem("Digital In");
    XBeeConfigWidgetFormUi.d1ComboBox->addItem("Digital Low");
    XBeeConfigWidgetFormUi.d1ComboBox->addItem("Digital High");
    XBeeConfigWidgetFormUi.d1ComboBox->setCurrentIndex((int)replyBuffer[0]);
    XBeeConfigWidgetFormUi.d2ComboBox->addItem("Disabled");
    XBeeConfigWidgetFormUi.d2ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d2ComboBox->addItem("Analogue In");
    XBeeConfigWidgetFormUi.d2ComboBox->addItem("Digital In");
    XBeeConfigWidgetFormUi.d2ComboBox->addItem("Digital Low");
    XBeeConfigWidgetFormUi.d2ComboBox->addItem("Digital High");
    XBeeConfigWidgetFormUi.d2ComboBox->setCurrentIndex((int)replyBuffer[0]);
    XBeeConfigWidgetFormUi.d3ComboBox->addItem("Disabled");
    XBeeConfigWidgetFormUi.d3ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d3ComboBox->addItem("Analogue In");
    XBeeConfigWidgetFormUi.d3ComboBox->addItem("Digital In");
    XBeeConfigWidgetFormUi.d3ComboBox->addItem("Digital Low");
    XBeeConfigWidgetFormUi.d3ComboBox->addItem("Digital High");
    XBeeConfigWidgetFormUi.d3ComboBox->setCurrentIndex((int)replyBuffer[0]);
    XBeeConfigWidgetFormUi.d4ComboBox->addItem("Disabled");
    XBeeConfigWidgetFormUi.d4ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d4ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d4ComboBox->addItem("Digital In");
    XBeeConfigWidgetFormUi.d4ComboBox->addItem("Digital Low");
    XBeeConfigWidgetFormUi.d4ComboBox->addItem("Digital High");
    XBeeConfigWidgetFormUi.d4ComboBox->setCurrentIndex((int)replyBuffer[0]);
    XBeeConfigWidgetFormUi.d5ComboBox->addItem("Disabled");
    XBeeConfigWidgetFormUi.d5ComboBox->addItem("Association");
    XBeeConfigWidgetFormUi.d5ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d5ComboBox->addItem("Digital In");
    XBeeConfigWidgetFormUi.d5ComboBox->addItem("Digital Low");
    XBeeConfigWidgetFormUi.d5ComboBox->addItem("Digital High");
    XBeeConfigWidgetFormUi.d5ComboBox->setCurrentIndex((int)replyBuffer[0]);
    XBeeConfigWidgetFormUi.d6ComboBox->addItem("Disabled");
    XBeeConfigWidgetFormUi.d6ComboBox->addItem("RTS Flow");
    XBeeConfigWidgetFormUi.d6ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d6ComboBox->addItem("Digital In");
    XBeeConfigWidgetFormUi.d6ComboBox->addItem("Digital Low");
    XBeeConfigWidgetFormUi.d6ComboBox->addItem("Digital High");
    XBeeConfigWidgetFormUi.d6ComboBox->setCurrentIndex((int)replyBuffer[0]);
    XBeeConfigWidgetFormUi.d7ComboBox->addItem("Disabled");
    XBeeConfigWidgetFormUi.d7ComboBox->addItem("CTS Flow");
    XBeeConfigWidgetFormUi.d7ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d7ComboBox->addItem("Digital In");
    XBeeConfigWidgetFormUi.d7ComboBox->addItem("Digital Low");
    XBeeConfigWidgetFormUi.d7ComboBox->addItem("Digital High");
    XBeeConfigWidgetFormUi.d7ComboBox->addItem("485 Tx Low En");
    XBeeConfigWidgetFormUi.d7ComboBox->addItem("485 Tx High En");
    XBeeConfigWidgetFormUi.d7ComboBox->setCurrentIndex((int)replyBuffer[0]);
    XBeeConfigWidgetFormUi.d10ComboBox->addItem("Disabled");
    XBeeConfigWidgetFormUi.d10ComboBox->addItem("RSSI");
    XBeeConfigWidgetFormUi.d10ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d10ComboBox->addItem("Digital In Mon");
    XBeeConfigWidgetFormUi.d10ComboBox->addItem("Digital Low");
    XBeeConfigWidgetFormUi.d10ComboBox->addItem("Digital High");
    XBeeConfigWidgetFormUi.d10ComboBox->setCurrentIndex((int)replyBuffer[0]);
    XBeeConfigWidgetFormUi.d11ComboBox->addItem("Digital In Unmon");
    XBeeConfigWidgetFormUi.d11ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d11ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d11ComboBox->addItem("Digital In Mon");
    XBeeConfigWidgetFormUi.d11ComboBox->addItem("Digital Low");
    XBeeConfigWidgetFormUi.d11ComboBox->addItem("Digital High");
    XBeeConfigWidgetFormUi.d11ComboBox->setCurrentIndex((int)replyBuffer[0]);
    XBeeConfigWidgetFormUi.d12ComboBox->addItem("Digital In Unmon");
    XBeeConfigWidgetFormUi.d12ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d12ComboBox->addItem("Reserved");
    XBeeConfigWidgetFormUi.d12ComboBox->addItem("Digital In Mon");
    XBeeConfigWidgetFormUi.d12ComboBox->addItem("Digital Low");
    XBeeConfigWidgetFormUi.d12ComboBox->addItem("Digital High");
    XBeeConfigWidgetFormUi.d12ComboBox->setCurrentIndex((int)replyBuffer[0]);
}

