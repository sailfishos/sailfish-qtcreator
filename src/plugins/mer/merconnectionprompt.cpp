/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include "merconnectionprompt.h"
#include "merconnection.h"
#include "mersdkmanager.h"
#include "mervirtualboxmanager.h"

#include <QMessageBox>
#include <QTimer>

//This ungly code fixes the isse of showing the messagebox, while QProcess
//form build stps ends. Since messagebox reenters event loop
//QProcess emits finnished singal and cleans up. Meanwhile users closes box end continues
//with invalid m_process pointer.

namespace Mer {
namespace Internal {

MerConnectionPrompt::MerConnectionPrompt(MerConnection *connection, const MerPlugin *merplugin):
    m_connection(connection)
  ,m_merplugin(merplugin)
{
}

void MerConnectionPrompt::prompt(const MerConnectionPrompt::PromptRequest pr)
{
    switch(pr) {
    case MerConnectionPrompt::Start:
        QTimer::singleShot(0,this,SLOT(startPrompt()));
        break;
    case MerConnectionPrompt::Close:
        QTimer::singleShot(0,this,SLOT(closePrompt()));
        break;
    }
}

void MerConnectionPrompt::startPrompt()
{
    const QMessageBox::StandardButton response =
        QMessageBox::question(0, tr("Start Virtual Machine"),
                              tr("Virtual Machine '%1' is not running! Please start the "
                                 "Virtual Machine and retry after the Virtual Machine is "
                                 "running.\n\n"
                                 "Start Virtual Machine now?").arg(m_connection->virtualMachine()),
                              QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (response == QMessageBox::Yes) {
        m_connection->connectTo();
    }

    this->deleteLater();
}

void MerConnectionPrompt::closePrompt()
{
    QList<MerSdk*> sdks = MerSdkManager::instance()->sdks();
    foreach(const MerSdk* sdk, sdks) {
        if(sdk->isHeadless()) {
            const QString& vm = sdk->virtualMachineName();
            if (sdk->connection()->state() == MerConnection::Connected) {
                const QMessageBox::StandardButton response =
                        QMessageBox::question(0, tr("Stop Virtual Machine"),
                        tr("The headless virtual machine '%1' is still running!\n\n"
                        "Stop Virtual Machine now?").arg(vm),
                        QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);

                if (response == QMessageBox::Yes) {
                    sdk->connection()->disconnectFrom();
                    QTimer::singleShot(3000, const_cast<Mer::Internal::MerPlugin*> (m_merplugin), SIGNAL(asynchronousShutdownFinished()));
                }
            }
        }
    }

    this->deleteLater();
}

}
}
