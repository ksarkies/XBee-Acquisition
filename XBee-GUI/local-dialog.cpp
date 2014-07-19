/*       XBee Control and Display GUI Tool

This provides a GUI interface to an XBee acquisition process running on the local
or remote Internet connected PC or Linux based controller for the XBee acquisition
network.
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

#include "local-dialog.h"
#include "xbee-control.h"
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

//-----------------------------------------------------------------------------
/** Constructor

@parameter int row. The table row number for the remote node.
@parameter bool remote. True if a remote node, false if local XBee.
@parameter parent Parent widget.
*/

LocalDialog::LocalDialog(QString address, int nodeRow, bool remoteNode, QWidget* parent)
                    : QDialog(parent), row(nodeRow), remote(remoteNode)
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
    tcpSocket->connectToHost(address, PORT);
// Pull in all the information table
    if (! tcpSocket->waitForConnected(10000)) exit(1);
// Ask for information about the remote node.
    QByteArray rowGetCommand;
    rowGetCommand.append('I');
    rowGetCommand.append(char(row));
    sendCommand(rowGetCommand);

/* Remote End Device nodes must be kept awake. The best way to do this
is to read the sleep mode, then if it is greater that 1, change it back
to 1 (pin sleep, as 0 is not possible in end devices). */
    if (deviceType == 2)                // Coordinator and routers are never changed
    {
/* First recover the actual setting. This requires an extra long delay
and we setup a message box to warn of this. */
        QByteArray sleepModeCommand;
        sleepModeCommand.clear();
        sleepModeCommand.append("SM");
        if (sendAtCommand(sleepModeCommand, remote,3000) > 0)
        {
#ifdef DEBUG
            qDebug() << "Timeout accessing remote node sleep mode";
#endif
        }
        else
        {
// Keep old value.
            oldSleepMode = replyBuffer[0];
#ifdef DEBUG
            qDebug() << "Original sleep mode" << (int)oldSleepMode << "changing to pin sleep";
#endif
// Set the sleep mode to 1 for the duration
            sleepModeCommand.clear();
            sleepModeCommand.append("SM");
            sleepModeCommand.append("\1");
            if (sendAtCommand(sleepModeCommand, remote,3000) > 0)
            {
#ifdef DEBUG
                qDebug() << "Timeout setting remote node sleep mode";
#endif
            }
        }
    }
// Build the User Interface display from the Ui class
    localDialogFormUi.setupUi(this);
// Setup the display with information from the addressed XBee
    localDialogFormUi.perSampSpinBox->setMaximum(65);
    setIOBoxes();
    on_refreshDisplay_clicked();
}

LocalDialog::~LocalDialog()
{
}

//-----------------------------------------------------------------------------
/** @brief Quit the dialogue window.

This closes the dialogue window and restores the sleep setting if it was changed.
*/
void LocalDialog::closeEvent(QCloseEvent *event)
{
    accept();
    QDialog::closeEvent(event);
}

//-----------------------------------------------------------------------------
/** @brief Quit the dialogue window.

This closes the dialogue window and restores the sleep setting if it was changed.
*/
void LocalDialog::accept()
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
        if (sendAtCommand(sleepModeCommand, remote,500) > 0) return;
        char currentSleepMode = replyBuffer[0];

/* Change back to the original version if currently different and if no change was made
to the setting. */
        char userSleepMode = localDialogFormUi.sleepModeComboBox->currentIndex();
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
            sendAtCommand(sleepModeCommand,remote,10);
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
            sendAtCommand(sleepModeCommand,remote,10);
// Finally WR command to write the change to permanent memory
            QByteArray wrCommand;
            wrCommand.clear();
            wrCommand.append("WR");
            sendAtCommand(wrCommand,remote,10);
        }
    }
}

//-----------------------------------------------------------------------------
/** @brief Reset the network Parameters.

*/
void LocalDialog::on_netResetButton_clicked()
{
    QByteArray atCommand;
    atCommand.append("NR");      // NR command to perform network reset
    atCommand.append('\0');     // This just resets the addressed XBee
    sendAtCommand(atCommand,remote,10);
}

//-----------------------------------------------------------------------------
/** @brief Reset the Device Software.

*/
void LocalDialog::on_softResetButton_clicked()
{
    QByteArray atCommand;
    atCommand.append("FR");      // FR command to perform software reset
    sendAtCommand(atCommand,remote,10);
}

//-----------------------------------------------------------------------------
/** @brief Disassociate from the network and reassociate.
*/
void LocalDialog::on_disassociateButton_clicked()
{
    QByteArray atCommand;
    atCommand.append("DA");      // DA command to disassociate
    sendAtCommand(atCommand,remote,10);
}

//-----------------------------------------------------------------------------
/** @brief Write newly entered values to the device memory.

If an entry widget was modified, write the new value and set in node permanent
memory.
*/
void LocalDialog::on_writeValuesButton_clicked()
{
    bool ok;
    bool changes = false;
// ID command to set the PAN ID
    QByteArray atCommand;
    if (localDialogFormUi.pan->isModified())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("ID");
        QString value = localDialogFormUi.pan->text();
        for (int i=0; i<16; i+=2) atCommand.append(value.mid(i,2).toUShort(&ok, 16));
        sendAtCommand(atCommand,remote,10);
    }
// NI command to set the node identifier
    if (localDialogFormUi.nodeIdent->isModified())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("NI");
        atCommand.append(localDialogFormUi.nodeIdent->text());
        sendAtCommand(atCommand,remote,10);
    }
// Join Notification Checkbox changed
    if (deviceType != 0)        // Routers and end devices only
    {
        if (joinNotificationCurrent != localDialogFormUi.joinNotificationCheckBox->checkState())
        {
            changes = true;
            atCommand.clear();
            atCommand.append("JN");
            atCommand.append(localDialogFormUi.joinNotificationCheckBox->isChecked() ? 1 : 0);
            sendAtCommand(atCommand,remote,10);
        }
    }
// Channel Verification Checkbox changed
    if (deviceType == 1)        // Routers only
    {
// NW command to set the network watchdog timer value
        if (localDialogFormUi.watchdogTime->isModified())
        {
            changes = true;
            atCommand.clear();
            atCommand.append("NW");
            int watchdogTime = localDialogFormUi.watchdogTime->text().toInt();
            atCommand.append((watchdogTime >> 8) & 0xFF);   // Upper byte
            atCommand.append(watchdogTime & 0xFF);          // Lower byte
            sendAtCommand(atCommand,remote,10);
        }
        if (channelVerifyCurrent != localDialogFormUi.channelVerifyCheckBox->checkState())
        {
            changes = true;
            atCommand.clear();
            atCommand.append("JV");
            atCommand.append(localDialogFormUi.channelVerifyCheckBox->isChecked() ? 1 : 0);
            sendAtCommand(atCommand,remote,10);
        }
    }
// Set sleep period
    if (localDialogFormUi.sleepPeriod->isModified())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("SP");
        int sleepPeriod = localDialogFormUi.sleepPeriod->text().toInt();
        atCommand.append((sleepPeriod >> 8) & 0xFF);   // Upper byte
        atCommand.append(sleepPeriod & 0xFF);          // Lower byte
        sendAtCommand(atCommand,remote,10);
    }
// Set sleep holdoff time
    if (deviceType == 2)
    {
        if (localDialogFormUi.sleepHoldoff->isModified())
        {
            changes = true;
            atCommand.clear();
            atCommand.append("ST");
            int sleepHoldoff = localDialogFormUi.sleepHoldoff->text().toInt();
            atCommand.append((sleepHoldoff >> 8) & 0xFF);   // Upper byte
            atCommand.append(sleepHoldoff & 0xFF);          // Lower byte
            sendAtCommand(atCommand,remote,10);
        }
    }
// Set D0
    if (d0Index != localDialogFormUi.d0ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D0");
        atCommand.append(localDialogFormUi.d0ComboBox->currentIndex());
        sendAtCommand(atCommand,remote,10);
    }
// Set D1
    if (d1Index != localDialogFormUi.d1ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D1");
        atCommand.append(localDialogFormUi.d1ComboBox->currentIndex());
        sendAtCommand(atCommand,remote,10);
    }
// Set D2
    if (d2Index != localDialogFormUi.d2ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D2");
        atCommand.append(localDialogFormUi.d2ComboBox->currentIndex());
        sendAtCommand(atCommand,remote,10);
    }
// Set D3
    if (d3Index != localDialogFormUi.d3ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D3");
        atCommand.append(localDialogFormUi.d3ComboBox->currentIndex());
        sendAtCommand(atCommand,remote,10);
    }
// Set D4
    if (d4Index != localDialogFormUi.d4ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D4");
        atCommand.append(localDialogFormUi.d4ComboBox->currentIndex());
        sendAtCommand(atCommand,remote,10);
    }
// Set D5
    if (d5Index != localDialogFormUi.d5ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D5");
        atCommand.append(localDialogFormUi.d5ComboBox->currentIndex());
        sendAtCommand(atCommand,remote,10);
    }
// Set D6
    if (d6Index != localDialogFormUi.d6ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D6");
        atCommand.append(localDialogFormUi.d6ComboBox->currentIndex());
        sendAtCommand(atCommand,remote,10);
    }
// Set D7
    if (d7Index != localDialogFormUi.d7ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("D7");
        atCommand.append(localDialogFormUi.d7ComboBox->currentIndex());
        sendAtCommand(atCommand,remote,10);
    }
// Set D10
    if (d10Index != localDialogFormUi.d10ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("P0");
        atCommand.append(localDialogFormUi.d10ComboBox->currentIndex());
        sendAtCommand(atCommand,remote,10);
    }
// Set D11
    if (d11Index != localDialogFormUi.d11ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("P1");
        atCommand.append(localDialogFormUi.d11ComboBox->currentIndex());
        sendAtCommand(atCommand,remote,10);
    }
// Set D12
    if (d12Index != localDialogFormUi.d12ComboBox->currentIndex())
    {
        changes = true;
        atCommand.clear();
        atCommand.append("P2");
        atCommand.append(localDialogFormUi.d12ComboBox->currentIndex());
        sendAtCommand(atCommand,remote,10);
    }
// Set sample period spinbox
    if (samplePeriod != localDialogFormUi.perSampSpinBox->value()*1000)
    {
        samplePeriod = localDialogFormUi.perSampSpinBox->value()*1000;
        changes = true;
        atCommand.clear();
        atCommand.append("IR");
        atCommand.append((samplePeriod >> 8) & 0xFF);   // Upper byte
        atCommand.append(samplePeriod & 0xFF);          // Lower byte
        sendAtCommand(atCommand,remote,10);
    }
// Finally WR command to write to permanent memory
    if (changes)
    {
        atCommand.clear();
        atCommand.append("WR");
        sendAtCommand(atCommand,remote,10);
    }
}

//-----------------------------------------------------------------------------
/** @brief Refresh parameter display.

Send some AT commands to pull in data about the local XBee settings.
The deviceType must be determined first by querying the coordinator process.

If errors occur, typically timeouts, continue the process to the end.
The user can try again.
*/
void LocalDialog::on_refreshDisplay_clicked()
{
    QByteArray atCommand;
    atCommand.append("ID");      // ID command to get PAN ID
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        localDialogFormUi.pan->setText(replyBuffer.toHex());
    }
    else
        localDialogFormUi.pan->setText("Error");
    atCommand.clear();
    atCommand.append("AI");      // AI command to get association indication
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        localDialogFormUi.association->setText(replyBuffer.toHex());
    }
    else
        localDialogFormUi.association->setText("Error");
    atCommand.clear();
    atCommand.append("NI");      // NI command to get association indication
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        localDialogFormUi.nodeIdent->setText(replyBuffer);
    }
    else
        localDialogFormUi.nodeIdent->setText("Error");
    atCommand.clear();
    atCommand.append("SH");      // SH command to get high serial number
    sendAtCommand(atCommand,remote,10);
    QString serialHigh = replyBuffer.toHex().toUpper();
    atCommand.clear();
    atCommand.append("SL");      // SL command to get low serial number
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        localDialogFormUi.serial->setText(serialHigh+' '+replyBuffer.toHex().toUpper());
    }
    else
        localDialogFormUi.serial->setText("Error");
    localDialogFormUi.children->setEnabled(false);
    localDialogFormUi.childrenLabel->setEnabled(false);
    if (deviceType != 2)            // Not end devices
    {
        atCommand.clear();
        atCommand.append("NC");      // NC command to get remaining children
        if (sendAtCommand(atCommand,remote,10) == 0)
        {
            localDialogFormUi.children->setEnabled(true);
            localDialogFormUi.childrenLabel->setEnabled(true);
            localDialogFormUi.children->setText(QString("%1").arg((int)replyBuffer[0]));
        }
    }
    localDialogFormUi.channelVerifyCheckBox->setEnabled(false);
    localDialogFormUi.channelVerifyLabel->setEnabled(false);
    localDialogFormUi.watchdogTime->setEnabled(false);
    localDialogFormUi.watchdogTimeLabel->setEnabled(false);
    if (deviceType == 1)            // Routers only
    {
        channelVerifyCurrent = localDialogFormUi.channelVerifyCheckBox->checkState();
        atCommand.clear();
        atCommand.append("JV");      // JV command to get channel verify setting
        if (sendAtCommand(atCommand,remote,10) == 0)
        {
            localDialogFormUi.channelVerifyCheckBox->setEnabled(true);
            localDialogFormUi.channelVerifyLabel->setEnabled(true);
            if (replyBuffer[0] == '\0')
                localDialogFormUi.channelVerifyCheckBox->setCheckState(Qt::Unchecked);
            else localDialogFormUi.channelVerifyCheckBox->setCheckState(Qt::Checked);
            channelVerifyCurrent = localDialogFormUi.channelVerifyCheckBox->checkState();
        }
        atCommand.clear();
        atCommand.append("NW");      // NW command to get network watchdog timeout
        if (sendAtCommand(atCommand,remote,10) == 0)
        {
            QString networkWatchdog = QString("%1")
                    .arg((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)),0,10);
            localDialogFormUi.watchdogTime->setEnabled(true);
            localDialogFormUi.watchdogTimeLabel->setEnabled(true);
            localDialogFormUi.watchdogTime->setText(networkWatchdog);
        }
    }
    localDialogFormUi.joinNotificationCheckBox->setEnabled(false);
    localDialogFormUi.joinNotificationLabel->setEnabled(false);
    localDialogFormUi.sleepModeComboBox->setEnabled(false);
    localDialogFormUi.sleepModeLabel->setEnabled(false);
    if (deviceType != 0)                // Not coordinator or router. Leave options disabled
    {
        atCommand.clear();
        atCommand.append("JN");         // JN command to get Join Notification
        if (sendAtCommand(atCommand,remote,10) == 0)
        {
            localDialogFormUi.joinNotificationCheckBox->setEnabled(true);
            localDialogFormUi.joinNotificationLabel->setEnabled(true);
            if (replyBuffer[0] == '\0')
                localDialogFormUi.joinNotificationCheckBox->setCheckState(Qt::Unchecked);
            else localDialogFormUi.joinNotificationCheckBox->setCheckState(Qt::Checked);
            joinNotificationCurrent = localDialogFormUi.joinNotificationCheckBox->checkState();
        }
    }
    if (deviceType == 2)                // Not coordinator or router. Leave options disabled
    {
// For routers and end devices, the sleep mode has already been read in as oldSleepMode.
        localDialogFormUi.sleepModeComboBox->setEnabled(true);
        localDialogFormUi.sleepModeLabel->setEnabled(true);
        localDialogFormUi.sleepModeComboBox->setCurrentIndex(oldSleepMode);
    }
    atCommand.clear();
    atCommand.append("SP");      // SP command to get sleep period
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        QString sleepPeriod = QString("%1")
                    .arg((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)),0,10);
        localDialogFormUi.sleepPeriod->setText(sleepPeriod);
    }
    atCommand.clear();
    atCommand.append("ST");      // ST command to get sleep holdoff time
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        QString sleepHoldoff = QString("%1")
                    .arg((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)),0,10);
        localDialogFormUi.sleepHoldoff->setText(sleepHoldoff);
    }
    atCommand.clear();
    atCommand.append("OP");      // OP command to get operational PAN ID
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        localDialogFormUi.opPan->setText(replyBuffer.toHex());
    }
    else
        localDialogFormUi.opPan->setText("Error");
    atCommand.clear();
    atCommand.append("PL");      // PL command to get power level
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        localDialogFormUi.powerLevel->setText(replyBuffer.toHex());
    }
    else
        localDialogFormUi.powerLevel->setText("Error");
    atCommand.clear();
    atCommand.append("PM");      // PM command to get power boost
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        if (replyBuffer.at(0) == 1)
            localDialogFormUi.powerBoost->setText("Boost");
        else
            localDialogFormUi.powerBoost->setText("");
    }
    atCommand.clear();
    atCommand.append("DB");      // DB command to get RSS
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        localDialogFormUi.rxLevel->setText(QString("-%1 dBm").arg((int)replyBuffer[0]));
    }
    else
        localDialogFormUi.rxLevel->setText("Error");
    atCommand.clear();
    atCommand.append("CH");      // CH command to get Channel
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        localDialogFormUi.channel->setText(replyBuffer.toHex().toUpper());
    }
    else
        localDialogFormUi.channel->setText("Error");
    atCommand.clear();
    atCommand.append("D0");      // D0 command to get GPIO port 0 setting
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        d0Index = (int)replyBuffer[0];
        localDialogFormUi.d0ComboBox->setEnabled(true);
        localDialogFormUi.d0ComboBox->setCurrentIndex(d0Index);
    }
    else
    {
        localDialogFormUi.d0ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D1");      // D1 command to get GPIO port 0 setting
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        d1Index = (int)replyBuffer[0];
        localDialogFormUi.d1ComboBox->setEnabled(true);
        localDialogFormUi.d1ComboBox->setCurrentIndex(d1Index);
    }
    else
    {
        localDialogFormUi.d1ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D2");      // D2 command to get GPIO port 0 setting
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        d2Index = (int)replyBuffer[0];
        localDialogFormUi.d2ComboBox->setEnabled(true);
        localDialogFormUi.d2ComboBox->setCurrentIndex(d2Index);
    }
    else
    {
        localDialogFormUi.d2ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D3");      // D3 command to get GPIO port 0 setting
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        d3Index = (int)replyBuffer[0];
        localDialogFormUi.d3ComboBox->setEnabled(true);
        localDialogFormUi.d3ComboBox->setCurrentIndex(d3Index);
    }
    else
    {
        localDialogFormUi.d3ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D4");      // D4 command to get GPIO port 0 setting
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        d4Index = (int)replyBuffer[0];
        localDialogFormUi.d4ComboBox->setEnabled(true);
        localDialogFormUi.d4ComboBox->setCurrentIndex(d4Index);
    }
    else
    {
        localDialogFormUi.d4ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D5");      // D5 command to get GPIO port 0 setting
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        d5Index = (int)replyBuffer[0];
        localDialogFormUi.d5ComboBox->setEnabled(true);
        localDialogFormUi.d5ComboBox->setCurrentIndex(d5Index);
    }
    else
    {
        localDialogFormUi.d5ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D6");      // D6 command to get GPIO port 0 setting
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        d6Index = (int)replyBuffer[0];
        localDialogFormUi.d6ComboBox->setEnabled(true);
        localDialogFormUi.d6ComboBox->setCurrentIndex(d6Index);
    }
    else
    {
        localDialogFormUi.d6ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("D7");      // D7 command to get GPIO port 0 setting
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        d7Index = (int)replyBuffer[0];
        localDialogFormUi.d7ComboBox->setEnabled(true);
        localDialogFormUi.d7ComboBox->setCurrentIndex(d7Index);
    }
    else
    {
        localDialogFormUi.d7ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("P0");      // D10 command to get GPIO port 0 setting
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        d10Index = (int)replyBuffer[0];
        localDialogFormUi.d10ComboBox->setEnabled(true);
        localDialogFormUi.d10ComboBox->setCurrentIndex(d10Index);
    }
    else
    {
        localDialogFormUi.d10ComboBox->setEnabled(false);
        localDialogFormUi.d10ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("P1");      // D11 command to get GPIO port 0 setting
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        d11Index = (int)replyBuffer[0];
        localDialogFormUi.d11ComboBox->setEnabled(true);
        localDialogFormUi.d11ComboBox->setCurrentIndex(d11Index);
    }
    else
    {
        localDialogFormUi.d11ComboBox->setEnabled(false);
        localDialogFormUi.d11ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("P2");      // D12 command to get GPIO port 0 setting
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        d12Index = (int)replyBuffer[0];
        localDialogFormUi.d12ComboBox->setEnabled(true);
        localDialogFormUi.d12ComboBox->setCurrentIndex(d12Index);
    }
    else
    {
        localDialogFormUi.d12ComboBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("IR");      // IR command to get sample period
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        samplePeriod = ((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF);
        localDialogFormUi.perSampSpinBox->setEnabled(true);
        localDialogFormUi.perSampSpinBox->setValue(samplePeriod/1000);
    }
    else
    {
        localDialogFormUi.perSampSpinBox->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("IC");      // IC command to get change detection
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        changeDetect = ((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)) << 1);
        localDialogFormUi.checkBoxC0->setEnabled(true);
        localDialogFormUi.checkBoxC1->setEnabled(true);
        localDialogFormUi.checkBoxC2->setEnabled(true);
        localDialogFormUi.checkBoxC3->setEnabled(true);
        localDialogFormUi.checkBoxC4->setEnabled(true);
        localDialogFormUi.checkBoxC5->setEnabled(true);
        localDialogFormUi.checkBoxC6->setEnabled(true);
        localDialogFormUi.checkBoxC7->setEnabled(true);
        localDialogFormUi.checkBoxC10->setEnabled(true);
        localDialogFormUi.checkBoxC11->setEnabled(true);
        localDialogFormUi.checkBoxC12->setEnabled(true);
        localDialogFormUi.checkBoxC0->setChecked((changeDetect <<= 1) & 0x01);
        localDialogFormUi.checkBoxC1->setChecked((changeDetect <<= 1) & 0x01);
        localDialogFormUi.checkBoxC2->setChecked((changeDetect <<= 1) & 0x01);
        localDialogFormUi.checkBoxC3->setChecked((changeDetect <<= 1) & 0x01);
        localDialogFormUi.checkBoxC4->setChecked((changeDetect <<= 1) & 0x01);
        localDialogFormUi.checkBoxC5->setChecked((changeDetect <<= 1) & 0x01);
        localDialogFormUi.checkBoxC6->setChecked((changeDetect <<= 1) & 0x01);
        localDialogFormUi.checkBoxC7->setChecked((changeDetect <<= 1) & 0x01);
        localDialogFormUi.checkBoxC10->setChecked((changeDetect <<= 1) & 0x01);
        localDialogFormUi.checkBoxC11->setChecked((changeDetect <<= 1) & 0x01);
        localDialogFormUi.checkBoxC12->setChecked((changeDetect <<= 1) & 0x01);
    }
    else
    {
        localDialogFormUi.checkBoxC0->setEnabled(false);
        localDialogFormUi.checkBoxC1->setEnabled(false);
        localDialogFormUi.checkBoxC2->setEnabled(false);
        localDialogFormUi.checkBoxC3->setEnabled(false);
        localDialogFormUi.checkBoxC4->setEnabled(false);
        localDialogFormUi.checkBoxC5->setEnabled(false);
        localDialogFormUi.checkBoxC6->setEnabled(false);
        localDialogFormUi.checkBoxC7->setEnabled(false);
        localDialogFormUi.checkBoxC10->setEnabled(false);
        localDialogFormUi.checkBoxC11->setEnabled(false);
        localDialogFormUi.checkBoxC12->setEnabled(false);
    }
    atCommand.clear();
    atCommand.append("PR");      // PR command to get pullup resistor
    if (sendAtCommand(atCommand,remote,10) == 0)
    {
        pullUp = ((((uint)replyBuffer[0] << 8) + ((uint)replyBuffer[1] & 0xFF)) << 1);
        localDialogFormUi.checkBoxP0->setEnabled(true);
        localDialogFormUi.checkBoxP1->setEnabled(true);
        localDialogFormUi.checkBoxP2->setEnabled(true);
        localDialogFormUi.checkBoxP3->setEnabled(true);
        localDialogFormUi.checkBoxP4->setEnabled(true);
        localDialogFormUi.checkBoxP5->setEnabled(true);
        localDialogFormUi.checkBoxP6->setEnabled(true);
        localDialogFormUi.checkBoxP7->setEnabled(true);
        localDialogFormUi.checkBoxP8->setEnabled(true);
        localDialogFormUi.checkBoxP9->setEnabled(true);
        localDialogFormUi.checkBoxP10->setEnabled(true);
        localDialogFormUi.checkBoxP11->setEnabled(true);
        localDialogFormUi.checkBoxP12->setEnabled(true);
        localDialogFormUi.checkBoxP13->setEnabled(true);
        localDialogFormUi.checkBoxP4->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP3->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP2->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP1->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP0->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP6->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP8->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP13->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP5->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP9->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP12->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP10->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP11->setChecked((pullUp >>= 1) & 0x01);
        localDialogFormUi.checkBoxP7->setChecked((pullUp >>= 1) & 0x01);
    }
    else
    {
        localDialogFormUi.checkBoxP0->setEnabled(false);
        localDialogFormUi.checkBoxP1->setEnabled(false);
        localDialogFormUi.checkBoxP2->setEnabled(false);
        localDialogFormUi.checkBoxP3->setEnabled(false);
        localDialogFormUi.checkBoxP4->setEnabled(false);
        localDialogFormUi.checkBoxP5->setEnabled(false);
        localDialogFormUi.checkBoxP6->setEnabled(false);
        localDialogFormUi.checkBoxP7->setEnabled(false);
        localDialogFormUi.checkBoxP8->setEnabled(false);
        localDialogFormUi.checkBoxP9->setEnabled(false);
        localDialogFormUi.checkBoxP10->setEnabled(false);
        localDialogFormUi.checkBoxP11->setEnabled(false);
        localDialogFormUi.checkBoxP12->setEnabled(false);
        localDialogFormUi.checkBoxP13->setEnabled(false);
    }
}

//-----------------------------------------------------------------------------
/** @brief Setup I/O configuration comboboxes.

*/
void LocalDialog::setIOBoxes()
{
    localDialogFormUi.d0ComboBox->addItem("Disabled");
    localDialogFormUi.d0ComboBox->addItem("Commission");
    localDialogFormUi.d0ComboBox->addItem("Analogue In");
    localDialogFormUi.d0ComboBox->addItem("Digital In");
    localDialogFormUi.d0ComboBox->addItem("Digital Low");
    localDialogFormUi.d0ComboBox->addItem("Digital High");
    localDialogFormUi.d0ComboBox->setCurrentIndex((int)replyBuffer[0]);
    localDialogFormUi.d1ComboBox->addItem("Disabled");
    localDialogFormUi.d1ComboBox->addItem("Reserved");
    localDialogFormUi.d1ComboBox->addItem("Analogue In");
    localDialogFormUi.d1ComboBox->addItem("Digital In");
    localDialogFormUi.d1ComboBox->addItem("Digital Low");
    localDialogFormUi.d1ComboBox->addItem("Digital High");
    localDialogFormUi.d1ComboBox->setCurrentIndex((int)replyBuffer[0]);
    localDialogFormUi.d2ComboBox->addItem("Disabled");
    localDialogFormUi.d2ComboBox->addItem("Reserved");
    localDialogFormUi.d2ComboBox->addItem("Analogue In");
    localDialogFormUi.d2ComboBox->addItem("Digital In");
    localDialogFormUi.d2ComboBox->addItem("Digital Low");
    localDialogFormUi.d2ComboBox->addItem("Digital High");
    localDialogFormUi.d2ComboBox->setCurrentIndex((int)replyBuffer[0]);
    localDialogFormUi.d3ComboBox->addItem("Disabled");
    localDialogFormUi.d3ComboBox->addItem("Reserved");
    localDialogFormUi.d3ComboBox->addItem("Analogue In");
    localDialogFormUi.d3ComboBox->addItem("Digital In");
    localDialogFormUi.d3ComboBox->addItem("Digital Low");
    localDialogFormUi.d3ComboBox->addItem("Digital High");
    localDialogFormUi.d3ComboBox->setCurrentIndex((int)replyBuffer[0]);
    localDialogFormUi.d4ComboBox->addItem("Disabled");
    localDialogFormUi.d4ComboBox->addItem("Reserved");
    localDialogFormUi.d4ComboBox->addItem("Reserved");
    localDialogFormUi.d4ComboBox->addItem("Digital In");
    localDialogFormUi.d4ComboBox->addItem("Digital Low");
    localDialogFormUi.d4ComboBox->addItem("Digital High");
    localDialogFormUi.d4ComboBox->setCurrentIndex((int)replyBuffer[0]);
    localDialogFormUi.d5ComboBox->addItem("Disabled");
    localDialogFormUi.d5ComboBox->addItem("Association");
    localDialogFormUi.d5ComboBox->addItem("Reserved");
    localDialogFormUi.d5ComboBox->addItem("Digital In");
    localDialogFormUi.d5ComboBox->addItem("Digital Low");
    localDialogFormUi.d5ComboBox->addItem("Digital High");
    localDialogFormUi.d5ComboBox->setCurrentIndex((int)replyBuffer[0]);
    localDialogFormUi.d6ComboBox->addItem("Disabled");
    localDialogFormUi.d6ComboBox->addItem("RTS Flow");
    localDialogFormUi.d6ComboBox->addItem("Reserved");
    localDialogFormUi.d6ComboBox->addItem("Digital In");
    localDialogFormUi.d6ComboBox->addItem("Digital Low");
    localDialogFormUi.d6ComboBox->addItem("Digital High");
    localDialogFormUi.d6ComboBox->setCurrentIndex((int)replyBuffer[0]);
    localDialogFormUi.d7ComboBox->addItem("Disabled");
    localDialogFormUi.d7ComboBox->addItem("CTS Flow");
    localDialogFormUi.d7ComboBox->addItem("Reserved");
    localDialogFormUi.d7ComboBox->addItem("Digital In");
    localDialogFormUi.d7ComboBox->addItem("Digital Low");
    localDialogFormUi.d7ComboBox->addItem("Digital High");
    localDialogFormUi.d7ComboBox->addItem("485 Tx Low En");
    localDialogFormUi.d7ComboBox->addItem("485 Tx High En");
    localDialogFormUi.d7ComboBox->setCurrentIndex((int)replyBuffer[0]);
    localDialogFormUi.d10ComboBox->addItem("Disabled");
    localDialogFormUi.d10ComboBox->addItem("RSSI");
    localDialogFormUi.d10ComboBox->addItem("Reserved");
    localDialogFormUi.d10ComboBox->addItem("Digital In Mon");
    localDialogFormUi.d10ComboBox->addItem("Digital Low");
    localDialogFormUi.d10ComboBox->addItem("Digital High");
    localDialogFormUi.d10ComboBox->setCurrentIndex((int)replyBuffer[0]);
    localDialogFormUi.d11ComboBox->addItem("Digital In Unmon");
    localDialogFormUi.d11ComboBox->addItem("Reserved");
    localDialogFormUi.d11ComboBox->addItem("Reserved");
    localDialogFormUi.d11ComboBox->addItem("Digital In Mon");
    localDialogFormUi.d11ComboBox->addItem("Digital Low");
    localDialogFormUi.d11ComboBox->addItem("Digital High");
    localDialogFormUi.d11ComboBox->setCurrentIndex((int)replyBuffer[0]);
    localDialogFormUi.d12ComboBox->addItem("Digital In Unmon");
    localDialogFormUi.d12ComboBox->addItem("Reserved");
    localDialogFormUi.d12ComboBox->addItem("Reserved");
    localDialogFormUi.d12ComboBox->addItem("Digital In Mon");
    localDialogFormUi.d12ComboBox->addItem("Digital Low");
    localDialogFormUi.d12ComboBox->addItem("Digital High");
    localDialogFormUi.d12ComboBox->setCurrentIndex((int)replyBuffer[0]);
}

//-----------------------------------------------------------------------------
/** @brief Send a command via the Internet Socket.

This will compute and fill the length, set the sync bytes, send the command and wait
for a reply up to 5s. If no reply it will return false.

@parameter  QByteArray command: An array to send with length, command and any parameters.
@returns    0 if no error occurred
            3 timeout waiting for response
*/
int LocalDialog::sendCommand(QByteArray command)
{
    int errorCode = 0;
    command.prepend(command.size()+1);
    comStatus = comSent;
    comCommand = command[1];
#ifdef DEBUG
    if (comCommand != 'r')          // Don't print remote check messages
        qDebug() << "sendCommand command sent:" << comCommand
                 << "string" << command.right(command.size()-3);
#endif
    command.append('\0');                  // Terminate the string
    tcpSocket->write(command);
// Wait up to 5s for a response. Should pass over to receiver function.
    if (! tcpSocket->waitForReadyRead(5000)) errorCode = 3;
    else
    {
// The receiver function should pick up and change away from sent status
// We need this to synchronize across the network and to avoid overload
        while (comStatus == comSent);
    }
    return errorCode;
}

//-----------------------------------------------------------------------------
/** @brief Pull in the return data from the Internet Process.

This interprets the echoed commands and performs most of the processing.
*/
void LocalDialog::readXbeeProcess()
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
            comStatus = comError;
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
        comStatus = comError;
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
    comStatus = comReceived;
}

//-----------------------------------------------------------------------------
/** @brief Notify of connection failure.

*/
void LocalDialog::displayError(QAbstractSocket::SocketError socketError)
{
    switch (socketError)
    {
    case QAbstractSocket::RemoteHostClosedError:
        break;
    case QAbstractSocket::HostNotFoundError:
        QMessageBox::information(this, tr("XBee GUI"),
                                 tr("The host was not found."));
        break;
    case QAbstractSocket::ConnectionRefusedError:
        QMessageBox::information(this, tr("XBee GUI"),
                                 tr("The connection was refused by the peer."));
        break;
    default:
        QMessageBox::information(this, tr("XBee GUI"),
                                 tr("The following error occurred: %1.")
                                 .arg(tcpSocket->errorString()));
    }
}

//-----------------------------------------------------------------------------
/** @brief Send an AT command and wait for response.

The command must be formatted as for the XBee AT commands.
When a response is checked, the result is returned in "response"
with more detail provided in "replyBuffer".

@return 0 no error
        1 socket command error (usually timeout)
        2 timeout waiting for response
        3 socket response error (usually timeout)
*/
int LocalDialog::sendAtCommand(QByteArray atCommand, bool remote, int countMax)
{
    int errorCode = 0;
    if (remote)
    {
        atCommand.prepend(row);     // row
        atCommand.prepend('R');
    }
    else
    {
        atCommand.prepend('\0');     // Dummy "row" value
        atCommand.prepend('L');
    }
    if (sendCommand(atCommand) > 0) errorCode = 1;
    else
    {
/* Ask for a confirmation or error code */
        replyBuffer.clear();
        atCommand.clear();
        if (remote)
            atCommand.append('r');
        else
            atCommand.append('l');
        atCommand.append(row);
        int count = 0;
        response = 0;
        while((response == 0) && (errorCode == 0))
        {
            if (count++ > countMax) errorCode = 2;
            if (sendCommand(atCommand) > 0) errorCode = 3;
            if (remote) millisleep(10);
            qApp->processEvents();
        }
    }
    return errorCode;
}

