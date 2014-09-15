/****************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
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

#include "merabstractvmstartstep.h"
#include "merconnection.h"
#include "merconnectionmanager.h"
#include "merconstants.h"
#include "mersdk.h"
#include "mersdkkitinformation.h"
#include "mervirtualboxmanager.h"

#include <coreplugin/icore.h>
#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

#include <QMessageBox>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

namespace {
const int CHECK_FOR_CANCEL_INTERVAL = 2000;
const int DISMISS_MESSAGE_BOX_DELAY = 2000;
}

class MerAbstractVmStartStepConfigWidget : public ProjectExplorer::SimpleBuildStepConfigWidget
{
    Q_OBJECT

public:
    MerAbstractVmStartStepConfigWidget(MerAbstractVmStartStep *step)
        : SimpleBuildStepConfigWidget(step)
    {
    }

    QString summaryText() const
    {
        return QString::fromLatin1("<b>%1:</b> %2")
            .arg(displayName())
            .arg(tr("Starts the virtual machine, if necessary."));
    }
};

MerAbstractVmStartStep::MerAbstractVmStartStep(BuildStepList *bsl, const Core::Id id)
    : BuildStep(bsl, id)
    , m_connection(0)
    , m_futureInterface(0)
    , m_questionBox(0)
    , m_checkForCancelTimer(0)
{
}

MerAbstractVmStartStep::MerAbstractVmStartStep(BuildStepList *bsl, MerAbstractVmStartStep *bs)
    : BuildStep(bsl, bs)
    , m_connection(bs->m_connection)
    , m_futureInterface(0)
    , m_questionBox(0)
    , m_checkForCancelTimer(0)
{
}

bool MerAbstractVmStartStep::init()
{
    QTC_ASSERT(m_connection, return false);

    return true;
}

void MerAbstractVmStartStep::run(QFutureInterface<bool> &fi)
{
    if (!m_connection) {
        emit addOutput(tr("%1: Internal error.").arg(displayName()), ErrorMessageOutput);
        fi.reportResult(false);
        emit finished();
        return;
    }

    if (m_connection->state() == MerConnection::Connected) {
        emit addOutput(tr("%1: The \"%2\" virtual machine is already running. Nothing to do.")
            .arg(displayName()).arg(m_connection->virtualMachine()), MessageOutput);
        fi.reportResult(true);
        emit finished();
    } else {
        emit addOutput(tr("%1: Starting \"%2\" virtual machine...")
                .arg(displayName()).arg(m_connection->virtualMachine()), MessageOutput);
        connect(m_connection, SIGNAL(stateChanged()), this, SLOT(onStateChanged()));
        m_futureInterface = &fi;
        if (m_connection->isVirtualMachineOff()) {
            m_questionBox = new QMessageBox(
                    QMessageBox::Question,
                    displayName(),
                    tr("The \"%1\" virtual machine is not running. Do you want to start it now?")
                    .arg(m_connection->virtualMachine()),
                    QMessageBox::Yes | QMessageBox::No,
                    Core::ICore::mainWindow());
            m_questionBox->setEscapeButton(QMessageBox::No);
            connect(m_questionBox, SIGNAL(finished(int)), this, SLOT(onQuestionBoxFinished()));
            m_questionBox->show();
            m_questionBox->raise();
        } else {
            beginConnect();
        }
    }
}

ProjectExplorer::BuildStepConfigWidget *MerAbstractVmStartStep::createConfigWidget()
{
    return new MerAbstractVmStartStepConfigWidget(this);
}

bool MerAbstractVmStartStep::immutable() const
{
    return false;
}

bool MerAbstractVmStartStep::runInGuiThread() const
{
    return true;
}

MerConnection *MerAbstractVmStartStep::connection() const
{
    return m_connection;
}

void MerAbstractVmStartStep::setConnection(MerConnection *connection)
{
    m_connection = connection;
}

void MerAbstractVmStartStep::beginConnect()
{
    m_checkForCancelTimer = new QTimer(this);
    connect(m_checkForCancelTimer, SIGNAL(timeout()), this, SLOT(checkForCancel()));
    m_checkForCancelTimer->start(CHECK_FOR_CANCEL_INTERVAL);
    m_connection->connectTo();
}

void MerAbstractVmStartStep::onStateChanged()
{
    switch (m_connection->state()) {
    case MerConnection::Disconnected:
    case MerConnection::Error:
        m_futureInterface->reportResult(false);
        break;

    case MerConnection::Connected:
        m_futureInterface->reportResult(true);
        break;

    default:
        return;
    }

    if (m_questionBox) {
        m_questionBox->setEnabled(false);
        QTimer::singleShot(DISMISS_MESSAGE_BOX_DELAY, m_questionBox, SLOT(deleteLater()));
        m_questionBox->disconnect(this);
        m_questionBox = 0;
    }

    m_connection->disconnect(this);
    m_connection = 0;
    m_futureInterface = 0;
    delete m_checkForCancelTimer, m_checkForCancelTimer = 0;
    emit finished();
}

void MerAbstractVmStartStep::onQuestionBoxFinished()
{
    QAbstractButton *button = m_questionBox->clickedButton();

    if (button == m_questionBox->button(QMessageBox::Yes)) {
        beginConnect();
    } else {
        m_connection->disconnect(this);
        m_connection = 0;
        m_futureInterface->reportResult(false);
        m_futureInterface = 0;
        emit finished();
    }

    m_questionBox->deleteLater(); // it's the sender
    m_questionBox = 0;
}

void MerAbstractVmStartStep::checkForCancel()
{
    if (m_futureInterface->isCanceled()) {
        m_connection->disconnect(this);
        m_connection = 0;
        m_futureInterface->reportResult(false);
        m_futureInterface = 0;
        delete m_checkForCancelTimer, m_checkForCancelTimer = 0;
        emit finished();
    }
}

} // namespace Internal
} // namespace Mer

#include "merabstractvmstartstep.moc"
