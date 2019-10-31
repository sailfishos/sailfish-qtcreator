/****************************************************************************
**
** Copyright (C) 2014-2015,2019 Jolla Ltd.
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

#ifndef MERABSTRACTVMSTARTSTEP_H
#define MERABSTRACTVMSTARTSTEP_H

#include <projectexplorer/buildstep.h>

#include <QPointer>

namespace Sfdk {
class VirtualMachine;
}

namespace Mer {
namespace Internal {

class MerAbstractVmStartStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    explicit MerAbstractVmStartStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);

    bool init() override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;

    Sfdk::VirtualMachine *virtualMachine() const;

protected:
    void doRun() override;
    void doCancel() override;
    void setVirtualMachine(Sfdk::VirtualMachine *virtualMachine);

private slots:
    void onStateChanged();

private:
    QPointer<Sfdk::VirtualMachine> m_virtualMachine;
};

} // namespace Internal
} // namespace Mer

#endif // MERABSTRACTVMSTARTSTEP_H
