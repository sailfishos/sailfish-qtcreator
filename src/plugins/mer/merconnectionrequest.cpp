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

#include "merconnectionrequest.h"
#include "merconnectionmanager.h"
#include <QMessageBox>
#include <QTimer>

//This ungly code fixes the isse of showing the messagebox, while QProcess
//form build stps ends. Since messagebox reenters event loop
//QProcess emits finnished singal and cleans up. Meanwhile users closes box end continues
//with invalid m_process pointer.

namespace Mer {
namespace Internal {


MerConnectionRequest::MerConnectionRequest(const QString& vm):
    m_vm(vm)
{
    QTimer::singleShot(0,this,SLOT(prompt()));
}

void MerConnectionRequest::prompt()
{
    const QMessageBox::StandardButton response =
        QMessageBox::question(0, tr("Start Virtual Machine"),
                              tr("Virtual Machine '%1' is not running! Please start the "
                                 "Virtual Machine and retry after the Virtual Machine is "
                                 "running.\n\n"
                                 "Start Virtual Machine now?").arg(m_vm),
                              QMessageBox::No | QMessageBox::Yes, QMessageBox::Yes);
    if (response == QMessageBox::Yes) {
        MerConnectionManager::instance()->connectTo(m_vm);
    }
    this->deleteLater();
}

}
}
