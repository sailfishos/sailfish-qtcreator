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

#ifndef MERABSTRACTVMSTARTSTEP_H
#define MERABSTRACTVMSTARTSTEP_H

#include <projectexplorer/buildstep.h>
#include <QPointer>

QT_BEGIN_NAMESPACE
class QMessageBox;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

class MerConnection;

class MerAbstractVmStartStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    explicit MerAbstractVmStartStep(ProjectExplorer::BuildStepList *bsl, const Core::Id id);
    MerAbstractVmStartStep(ProjectExplorer::BuildStepList *bsl, MerAbstractVmStartStep *bs);

    bool init();
    void run(QFutureInterface<bool> &fi);
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget();
    bool immutable() const;
    bool runInGuiThread() const;

    MerConnection *connection() const;

protected:
    void setConnection(MerConnection *connection);

private:
    void beginConnect();

private slots:
    void onStateChanged();
    void onQuestionBoxFinished();
    void checkForCancel();

private:
    QPointer<MerConnection> m_connection;
    QFutureInterface<bool> *m_futureInterface;
    QMessageBox *m_questionBox;
    QTimer *m_checkForCancelTimer;
};

} // namespace Internal
} // namespace Mer

#endif // MERABSTRACTVMSTARTSTEP_H
