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

using namespace Core;
using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

namespace {
const int CHECK_FOR_CANCEL_INTERVAL = 2000;
const int DISMISS_MESSAGE_BOX_DELAY = 2000;
}

class MerAbstractVmStartStepConfigWidget : public BuildStepConfigWidget
{
    Q_OBJECT

public:
    MerAbstractVmStartStepConfigWidget(MerAbstractVmStartStep *step)
        : BuildStepConfigWidget(step)
    {
        setSummaryText(QString::fromLatin1("<b>%1:</b> %2")
                .arg(displayName())
                .arg(tr("Starts the virtual machine, if necessary.")));
    }
};

MerAbstractVmStartStep::MerAbstractVmStartStep(BuildStepList *bsl, Core::Id id)
    : BuildStep(bsl, id)
    , m_connection(0)
{
    setWidgetExpandedByDefault(false);
}

bool MerAbstractVmStartStep::init()
{
    QTC_ASSERT(m_connection, return false);

    return true;
}

void MerAbstractVmStartStep::doRun()
{
    if (!m_connection) {
        emit addOutput(tr("%1: Internal error.").arg(displayName()), OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    if (m_connection->state() == MerConnection::Connected) {
        emit addOutput(tr("%1: The \"%2\" virtual machine is already running. Nothing to do.")
            .arg(displayName()).arg(m_connection->virtualMachine()), OutputFormat::NormalMessage);
        emit finished(true);
    } else {
        emit addOutput(tr("%1: Starting \"%2\" virtual machine...")
                .arg(displayName()).arg(m_connection->virtualMachine()), OutputFormat::NormalMessage);

        connect(m_connection.data(), &MerConnection::stateChanged,
                this, &MerAbstractVmStartStep::onStateChanged);
        m_connection->connectTo(MerConnection::AskStartVm);
    }
}

void MerAbstractVmStartStep::doCancel()
{
    QTC_ASSERT(m_connection, return);
    m_connection->disconnect(this);
    m_connection = 0;
}

BuildStepConfigWidget *MerAbstractVmStartStep::createConfigWidget()
{
    return new MerAbstractVmStartStepConfigWidget(this);
}

MerConnection *MerAbstractVmStartStep::connection() const
{
    return m_connection;
}

void MerAbstractVmStartStep::setConnection(MerConnection *connection)
{
    m_connection = connection;
}

void MerAbstractVmStartStep::onStateChanged()
{
    bool result = false;
    switch (m_connection->state()) {
    case MerConnection::Disconnected:
    case MerConnection::Error:
        break;

    case MerConnection::Connected:
        result = true;
        break;

    default:
        return;
    }

    m_connection->disconnect(this);
    m_connection = 0;
    emit finished(result);
}

} // namespace Internal
} // namespace Mer

#include "merabstractvmstartstep.moc"
