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

#include <coreplugin/icore.h>
#include <utils/checkablemessagebox.h>
#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QMessageBox>
#include <QProgressDialog>
#include <QTimer>

using namespace Core;
using namespace Sfdk;
using Utils::CheckableMessageBox;

namespace Mer {
namespace Internal {

namespace {
const int DISMISS_MESSAGE_BOX_DELAY = 2000;
}

void MerVmConnectionUi::warn(Warning which)
{
    switch (which) {
    case AlreadyConnecting:
        openWarningBox(tr("Already Connecting to Virtual Machine"),
                tr("Already connecting to the \"%1\" virtual machine - please repeat later."))
            ->setAttribute(Qt::WA_DeleteOnClose);
        break;
    case AlreadyDisconnecting:
        openWarningBox(tr("Already Disconnecting from Virtual Machine"),
                tr("Already disconnecting from the \"%1\" virtual machine - please repeat later."))
            ->setAttribute(Qt::WA_DeleteOnClose);
        break;
    case UnableToCloseVm:
        QTC_CHECK(!m_unableToCloseVmWarningBox);
        m_unableToCloseVmWarningBox = openWarningBox(
                tr("Unable to Close Virtual Machine"),
                tr("Timeout waiting for the \"%1\" virtual machine to close."));
        break;
    case VmNotRegistered:
        openWarningBox(tr("Virtual Machine Not Found"),
                tr("No virtual machine with the name \"%1\" found. Check your installation."))
            ->setAttribute(Qt::WA_DeleteOnClose);
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

void MerVmConnectionUi::ask(Question which, OnStatusChanged onStatusChanged)
{
    switch (which) {
    case StartVm:
        QTC_CHECK(!m_startVmQuestionBox);
        m_startVmQuestionBox = openQuestionBox(
                onStatusChanged,
                tr("Start Virtual Machine"),
                tr("The \"%1\" virtual machine is not running. Do you want to start it now?"),
                QString(),
                MerSettings::isAskBeforeStartingVmEnabled()
                    ? [] { MerSettings::setAskBeforeStartingVmEnabled(false); }
                    : std::function<void()>());
        break;
    case ResetVm:
        QTC_CHECK(!m_resetVmQuestionBox);
        bool startedOutside;
        connection()->isVirtualMachineOff(0, &startedOutside);
        m_resetVmQuestionBox = openQuestionBox(
                onStatusChanged,
                tr("Reset Virtual Machine"),
                tr("Connection to the \"%1\" virtual machine failed recently. "
                    "Do you want to reset the virtual machine first?"),
                startedOutside ?
                    tr("This virtual machine has been started outside of this Qt Creator session.")
                    : QString());
        break;
    case CloseVm:
        QTC_CHECK(!m_closeVmQuestionBox);
        m_closeVmQuestionBox = openQuestionBox(
                onStatusChanged,
                tr("Close Virtual Machine"),
                tr("Do you really want to close the \"%1\" virtual machine?"),
                tr("This virtual machine has been started outside of this Qt Creator session. "
                    "Answer \"No\" to disconnect and leave the virtual machine running."));
        break;
    case CancelConnecting:
        QTC_CHECK(!m_connectingProgressDialog);
        m_connectingProgressDialog = openProgressDialog(
                onStatusChanged,
                tr("Connecting to Virtual Machine"),
                tr("Connecting to the \"%1\" virtual machine…"));
        break;
    case CancelLockingDown:
        QTC_CHECK(!m_lockingDownProgressDialog);
        m_lockingDownProgressDialog = openProgressDialog(
                onStatusChanged,
                tr("Closing Virtual Machine"),
                tr("Waiting for the \"%1\" virtual machine to close…"));
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
        deleteDialog(m_connectingProgressDialog);
        break;
    case CancelLockingDown:
        deleteDialog(m_lockingDownProgressDialog);
        break;
    }
}

VmConnection::Ui::QuestionStatus MerVmConnectionUi::status(Question which) const
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

QMessageBox *MerVmConnectionUi::openWarningBox(const QString &title, const QString &text)
{
    QMessageBox *box = new QMessageBox(
            QMessageBox::Warning,
            title,
            text.arg(connection()->virtualMachine()),
            QMessageBox::Ok,
            ICore::mainWindow());
    box->show();
    box->raise();
    return box;
}

QMessageBox *MerVmConnectionUi::openQuestionBox(OnStatusChanged onStatusChanged,
        const QString &title, const QString &text, const QString &informativeText,
        std::function<void()> setDoNotAskAgain)
{
    QMessageBox *box = new QMessageBox(
            QMessageBox::Question,
            title,
            text.arg(connection()->virtualMachine()),
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
            connection(), onStatusChanged);
    box->show();
    box->raise();
    return box;
}

QProgressDialog *MerVmConnectionUi::openProgressDialog(OnStatusChanged onStatusChanged,
        const QString &title, const QString &text)
{
    QProgressDialog *dialog = new QProgressDialog(ICore::mainWindow());
    dialog->setMaximum(0);
    dialog->setWindowTitle(title);
    dialog->setLabelText(text.arg(connection()->virtualMachine()));
    connect(dialog, &QDialog::finished,
            connection(), onStatusChanged);
    dialog->show();
    dialog->raise();
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

VmConnection::Ui::QuestionStatus MerVmConnectionUi::status(QMessageBox *box) const
{
    if (!box)
        return NotAsked;
    else if (QAbstractButton *button = box->clickedButton())
        return button == box->button(QMessageBox::Yes) ? Yes : No;
    else
        return Asked;
}

VmConnection::Ui::QuestionStatus MerVmConnectionUi::status(QProgressDialog *dialog) const
{
    if (!dialog)
        return NotAsked;
    else if (!dialog->isVisible())
        return dialog->wasCanceled() ? Yes : No;
    else
        return Asked;
}

} // namespace Internal
} // namespace Mer
