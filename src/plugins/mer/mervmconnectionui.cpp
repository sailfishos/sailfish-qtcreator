/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "mervmconnectionui.h"

#include "mersettings.h"

#include <sfdk/sdk.h>

#include <coreplugin/icore.h>
#include <ssh/sshconnection.h>
#include <utils/checkablemessagebox.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QCloseEvent>
#include <QMessageBox>
#include <QProgressDialog>
#include <QPushButton>
#include <QTimer>

using namespace Core;
using namespace Sfdk;
using Utils::CheckableMessageBox;

namespace Mer {
namespace Internal {

namespace {
const int PROGRESS_MINIMUM_DURATION_PROLONGED = 20000; // aligned with VmConnection timeouts
const int DISMISS_MESSAGE_BOX_DELAY = 2000;
}

class MerVmConnectionUi::ProgressDialog : public QProgressDialog
{
    Q_OBJECT

public:
    ProgressDialog(QWidget *parent)
        : QProgressDialog(parent)
    {
        m_cancelButton = new QPushButton(tr("Cancel"), this);
        m_cancelButton->setEnabled(false);
        setCancelButton(m_cancelButton);
        connect(this, &QDialog::finished, [=]() {
            if (m_cancelHandler)
                m_cancelHandler();
        });
    }

    void enableCancel(std::function<void()> handler)
    {
        m_cancelHandler = handler;
        m_cancelButton->setEnabled(true);
    }

    void disableCancel()
    {
        m_cancelButton->setEnabled(false);
        m_cancelHandler = {};
    }

    bool isCancelEnabled() const
    {
        return m_cancelButton->isEnabled();
    }

protected:
    void closeEvent(QCloseEvent *event) override
    {
        event->ignore();
    }

private:
    QPointer<QPushButton> m_cancelButton;
    std::function<void()> m_cancelHandler;
};

void MerVmConnectionUi::setVirtualMachine(VirtualMachine *virtualMachine)
{
    ConnectionUi::setVirtualMachine(virtualMachine);

    connect(virtualMachine, &VirtualMachine::stateChanged,
            this, &MerVmConnectionUi::onVmStateChanged);
}

void MerVmConnectionUi::warn(Warning which)
{
    switch (which) {
    case UnableToCloseVm:
        QTC_CHECK(!m_unableToCloseVmWarningBox);
        m_unableToCloseVmWarningBox = openWarningBox(which);
        break;
    case VmNotRegistered:
        openWarningBox(which)->setAttribute(Qt::WA_DeleteOnClose);
        break;
    case SshPortOccupied:
        openWarningBox(which)->setAttribute(Qt::WA_DeleteOnClose);
        break;
    }
}

void MerVmConnectionUi::dismissWarning(Warning which)
{
    switch (which) {
    case UnableToCloseVm:
        deleteDialog(m_unableToCloseVmWarningBox);
        break;
    default:
        QTC_CHECK(false);
    }
}

bool MerVmConnectionUi::shouldAsk(Question which) const
{
    switch (which) {
    case StartVm:
        return MerSettings::isAskBeforeStartingVmEnabled();
        break;
    default:
        QTC_CHECK(false);
        return false;
    }
}

void MerVmConnectionUi::ask(Question which, std::function<void()> onStatusChanged)
{
    switch (which) {
    case StartVm:
        QTC_CHECK(!m_startVmQuestionBox);
        m_startVmQuestionBox = openQuestionBox(which, onStatusChanged, QString(),
                MerSettings::isAskBeforeStartingVmEnabled()
                    ? [] { MerSettings::setAskBeforeStartingVmEnabled(false); }
                    : std::function<void()>());
        break;
    case ResetVm:
        QTC_CHECK(!m_resetVmQuestionBox);
        bool startedOutside;
        virtualMachine()->isOff(0, &startedOutside);
        m_resetVmQuestionBox = openQuestionBox(which, onStatusChanged,
                startedOutside ?
                    tr("The virtual machine has been started outside of this Qt Creator session.")
                    : QString());
        break;
    case CloseVm:
        QTC_CHECK(!m_closeVmQuestionBox);
        m_closeVmQuestionBox = openQuestionBox(which, onStatusChanged,
                tr("The virtual machine has been started outside of this Qt Creator session. "
                    "Answer \"No\" to disconnect and leave the virtual machine running."));
        break;
    case CancelConnecting:
        QTC_ASSERT(m_connectingProgressDialog, return);
        m_connectingProgressDialog->enableCancel(onStatusChanged);
        m_connectingProgressDialog->show();
        break;
    case CancelLockingDown:
        QTC_ASSERT(m_lockingDownProgressDialog, return);
        m_lockingDownProgressDialog->enableCancel(onStatusChanged);
        m_lockingDownProgressDialog->show();
        break;
    }
}

void MerVmConnectionUi::dismissQuestion(Question which)
{
    switch (which) {
    case StartVm:
        deleteDialog(m_startVmQuestionBox);
        break;
    case ResetVm:
        deleteDialog(m_resetVmQuestionBox);
        break;
    case CloseVm:
        deleteDialog(m_closeVmQuestionBox);
        break;
    case CancelConnecting:
        QTC_ASSERT(m_connectingProgressDialog, return);
        m_connectingProgressDialog->disableCancel();
        break;
    case CancelLockingDown:
        QTC_ASSERT(m_lockingDownProgressDialog, return);
        m_lockingDownProgressDialog->disableCancel();
        break;
    }
}

VirtualMachine::ConnectionUi::QuestionStatus MerVmConnectionUi::status(Question which) const
{
    switch (which) {
    case StartVm:
        return status(m_startVmQuestionBox);
        break;
    case ResetVm:
        return status(m_resetVmQuestionBox);
        break;
    case CloseVm:
        return status(m_closeVmQuestionBox);
        break;
    case CancelConnecting:
        return status(m_connectingProgressDialog);
        break;
    case CancelLockingDown:
        return status(m_lockingDownProgressDialog);
        break;
    }

    QTC_CHECK(false);
    return NotAsked;
}

void MerVmConnectionUi::informStateChangePending()
{
    QMessageBox::information(ICore::dialogParent(), tr("Request Pending"),
            tr("Another request pending. Try later."));
}

void MerVmConnectionUi::onVmStateChanged()
{
    switch (virtualMachine()->state()) {
    case VirtualMachine::Disconnected:
    case VirtualMachine::Error:
    case VirtualMachine::Connected:
        deleteDialog(m_connectingProgressDialog);
        deleteDialog(m_lockingDownProgressDialog);
        break;

    case VirtualMachine::Starting:
    case VirtualMachine::Connecting:
        if (!m_connectingProgressDialog) {
            m_connectingProgressDialog = openProgressDialog(ConnectionUi::CancelConnecting);
            if (ICore::dialogParent() == ICore::mainWindow())
                m_connectingProgressDialog->setMinimumDuration(PROGRESS_MINIMUM_DURATION_PROLONGED);
        }
        break;

    case VirtualMachine::Disconnecting:
        if (!m_lockingDownProgressDialog) {
            // Avoid displaying the CancelLockingDown question unless we know
            // for sure the VM is going to be closed.  E.g. when connection is
            // requested after connection failure, it will disconnect first,
            // then ask whether the VM should be also restarted, and the user
            // may decide to NOT restart it.
            if (!ICore::mainWindow()->isVisible()) {
                m_lockingDownProgressDialog = openProgressDialog(ConnectionUi::CancelLockingDown);
                m_lockingDownProgressDialog->show();
            }
        }
        break;

    case VirtualMachine::Closing:
        if (!m_lockingDownProgressDialog) {
            m_lockingDownProgressDialog = openProgressDialog(ConnectionUi::CancelLockingDown);
            if (!ICore::mainWindow()->isVisible()) {
                // Display immediately at Qt Creator shutdown
                m_lockingDownProgressDialog->show();
            } else if (ICore::dialogParent() == ICore::mainWindow()) {
                m_lockingDownProgressDialog->setMinimumDuration(PROGRESS_MINIMUM_DURATION_PROLONGED);
            }
        }
        break;
    }
}

QMessageBox *MerVmConnectionUi::openWarningBox(Warning which)
{
    QString title;
    QString text;
    formatWarning(which, &title, &text);

    QMessageBox *box = new QMessageBox(
            QMessageBox::Warning,
            title,
            text.arg(virtualMachine()->name()),
            QMessageBox::Ok,
            ICore::mainWindow());
    box->show();
    box->raise();
    return box;
}

QMessageBox *MerVmConnectionUi::openQuestionBox(Question which,
        std::function<void()> onStatusChanged, const QString &informativeText,
        std::function<void()> setDoNotAskAgain)
{
    QString title;
    QString text;
    formatQuestion(which, &title, &text);

    QMessageBox *box = new QMessageBox(
            QMessageBox::Question,
            title,
            text.arg(virtualMachine()->name()),
            QMessageBox::Yes | QMessageBox::No,
            ICore::mainWindow());
    box->setInformativeText(informativeText);
    if (setDoNotAskAgain) {
        box->setCheckBox(new QCheckBox(CheckableMessageBox::msgDoNotAskAgain()));
        connect(box, &QMessageBox::finished, [=] {
            if (box->checkBox()->isChecked())
                setDoNotAskAgain();
        });
    }
    box->setEscapeButton(QMessageBox::No);
    connect(box, &QMessageBox::finished,
            virtualMachine(), onStatusChanged);
    box->show();
    box->raise();
    return box;
}

MerVmConnectionUi::ProgressDialog *MerVmConnectionUi::openProgressDialog(Question which)
{
    QString title;
    QString text;
    formatQuestion(which, &title, &text);

    ProgressDialog *dialog = new ProgressDialog(ICore::mainWindow());
    // It is not really desired to make it modal, but it seems to be the
    // only reliable way to ensure the dialog is on top when it becomes visible.
    dialog->setWindowModality(Qt::ApplicationModal);
    dialog->setMaximum(0);
    dialog->setValue(0); // Required to make setMinimumDuration work
    dialog->setWindowTitle(title);
    dialog->setLabelText(text.arg(virtualMachine()->name()));
    return dialog;
}

template<class Dialog>
void MerVmConnectionUi::deleteDialog(QPointer<Dialog> &dialog)
{
    if (!dialog)
        return;

    if (!dialog->isVisible()) {
        delete dialog;
    } else {
        dialog->setEnabled(false);
        QTimer::singleShot(DISMISS_MESSAGE_BOX_DELAY, dialog.data(), &QObject::deleteLater);
        dialog->disconnect();
        dialog = 0;
    }
}

VirtualMachine::ConnectionUi::QuestionStatus MerVmConnectionUi::status(QMessageBox *box) const
{
    if (!box)
        return NotAsked;
    else if (QAbstractButton *button = box->clickedButton())
        return button == box->button(QMessageBox::Yes) ? Yes : No;
    else
        return Asked;
}

VirtualMachine::ConnectionUi::QuestionStatus MerVmConnectionUi::status(ProgressDialog *dialog) const
{
    if (!dialog || !dialog->isCancelEnabled())
        return NotAsked;
    else if (!dialog->isVisible())
        return dialog->wasCanceled() ? Yes : No;
    else
        return Asked;
}

/*!
 * \class MerBuildEngineVmConnectionUi
 * \internal
 */

void MerBuildEngineVmConnectionUi::formatWarning(Warning which, QString *title, QString *text)
{
    Q_ASSERT(title);
    Q_ASSERT(text);

    switch (which) {
    case UnableToCloseVm:
        *title = tr("Unable to Stop %1 Build Engine").arg(Sdk::sdkVariant());
        *text = tr("Timeout waiting for the \"%1\" virtual machine to close.");
        break;
    case VmNotRegistered:
        *title = tr("%1 Build Engine Not Found").arg(Sdk::sdkVariant());
        *text = tr("No virtual machine with the name \"%1\" found. Check your installation.");
        break;
    case SshPortOccupied:
        *title = tr("Conflicting SSH Port Configuration");
        *text = tr("Another application seems to be listening on the TCP port %1 configured as "
                "SSH port for the \"%%1\" virtual machine - choose another SSH port in options.")
            .arg(virtualMachine()->sshParameters().port());
        break;
    }
}

void MerBuildEngineVmConnectionUi::formatQuestion(Question which, QString *title, QString *text)
{
    Q_ASSERT(title);
    Q_ASSERT(text);

    switch (which) {
    case StartVm:
        *title = tr("Start %1 Build Engine").arg(Sdk::sdkVariant());
        *text = tr("The \"%1\" virtual machine is not running. Do you want to start it now?");
        break;
    case ResetVm:
        *title = tr("Reset %1 Build Engine").arg(Sdk::sdkVariant());
        *text = tr("Connection to the \"%1\" virtual machine failed recently. "
                "Do you want to reset the virtual machine first?");
        break;
    case CloseVm:
        *title = tr("Stop %1 Build Engine").arg(Sdk::sdkVariant());
        *text = tr("Do you really want to close the \"%1\" virtual machine?");
        break;
    case CancelConnecting:
        *title = tr("Connecting to %1 Build Engine").arg(Sdk::sdkVariant());
        *text = tr("Connecting to the \"%1\" virtual machine…");
        break;
    case CancelLockingDown:
        *title = tr("Stopping %1 Build Engine").arg(Sdk::sdkVariant());
        *text = tr("Waiting for the \"%1\" virtual machine to close…");
        break;
    }
}

/*!
 * \class MerEmulatorVmConnectionUi
 * \internal
 */

void MerEmulatorVmConnectionUi::formatWarning(Warning which, QString *title, QString *text)
{
    Q_ASSERT(title);
    Q_ASSERT(text);

    switch (which) {
    case UnableToCloseVm:
        *title = tr("Unable to Stop %1 Emulator").arg(Sdk::osVariant());
        *text = tr("Timeout waiting for the \"%1\" virtual machine to close.");
        break;
    case VmNotRegistered:
        *title = tr("%1 Emulator Not Found").arg(Sdk::osVariant());
        *text = tr("No virtual machine with the name \"%1\" found. Check your installation.");
        break;
    case SshPortOccupied:
        *title = tr("Conflicting SSH Port Configuration");
        *text = tr("Another application seems to be listening on the TCP port %1 configured as "
                "SSH port for the \"%%1\" emulator - choose another SSH port in options.")
            .arg(virtualMachine()->sshParameters().port());
        break;
    }
}

void MerEmulatorVmConnectionUi::formatQuestion(Question which, QString *title, QString *text)
{
    Q_ASSERT(title);
    Q_ASSERT(text);

    switch (which) {
    case StartVm:
        *title = tr("Start %1 Emulator").arg(Sdk::osVariant());
        *text = tr("The \"%1\" virtual machine is not running. Do you want to start it now?");
        break;
    case ResetVm:
        *title = tr("Reset %1 Emulator").arg(Sdk::osVariant());
        *text = tr("Connection to the \"%1\" virtual machine failed recently. "
                "Do you want to reset the virtual machine first?");
        break;
    case CloseVm:
        *title = tr("Stop %1 Emulator").arg(Sdk::osVariant());
        *text = tr("Do you really want to close the \"%1\" virtual machine?");
        break;
    case CancelConnecting:
        *title = tr("Connecting to %1 Emulator").arg(Sdk::osVariant());
        *text = tr("Connecting to the \"%1\" virtual machine…");
        break;
    case CancelLockingDown:
        *title = tr("Stopping %1 Emulator").arg(Sdk::osVariant());
        *text = tr("Waiting for the \"%1\" virtual machine to close…");
        break;
    }
}

} // namespace Internal
} // namespace Mer

#include "mervmconnectionui.moc"
