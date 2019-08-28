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

#pragma once

#include <sfdk/virtualmachine.h>

#include <QPointer>

QT_BEGIN_NAMESPACE
class QMessageBox;
class QProgressDialog;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

class MerVmConnectionUi : public Sfdk::VirtualMachine::ConnectionUi
{
    Q_OBJECT

public:
    using Sfdk::VirtualMachine::ConnectionUi::ConnectionUi;

    void warn(Warning which) override;
    void dismissWarning(Warning which) override;

    bool shouldAsk(Question which) const override;
    void ask(Question which, std::function<void()> onStatusChanged) override;
    void dismissQuestion(Question which) override;
    QuestionStatus status(Question which) const override;

private:
    QMessageBox *openWarningBox(const QString &title, const QString &text);
    QMessageBox *openQuestionBox(std::function<void()> onStatusChanged,
            const QString &title, const QString &text,
            const QString &informativeText = QString(),
            std::function<void()> setDoNotAskAgain = nullptr);
    QProgressDialog *openProgressDialog(std::function<void()> onStatusChanged,
            const QString &title, const QString &text);
    template<class Dialog>
    void deleteDialog(QPointer<Dialog> &dialog);
    QuestionStatus status(QMessageBox *box) const;
    QuestionStatus status(QProgressDialog *dialog) const;

private:
    QPointer<QMessageBox> m_unableToCloseVmWarningBox;
    QPointer<QMessageBox> m_startVmQuestionBox;
    QPointer<QMessageBox> m_resetVmQuestionBox;
    QPointer<QMessageBox> m_closeVmQuestionBox;
    QPointer<QProgressDialog> m_connectingProgressDialog;
    QPointer<QProgressDialog> m_lockingDownProgressDialog;
};

} // namespace Internal
} // namespace Mer
