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

#ifndef MEREMULATORSTARTSTEP_H
#define MEREMULATORSTARTSTEP_H

#include <remotelinux/abstractremotelinuxdeploystep.h>

namespace Mer {
namespace Internal {

class MerEmulatorStartService;
class MerEmulatorStartStep : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
    Q_OBJECT

public:
    MerEmulatorStartStep(ProjectExplorer::BuildStepList *bsl);
    MerEmulatorStartStep(ProjectExplorer::BuildStepList *bsl, MerEmulatorStartStep *other);

    static Core::Id stepId();
    static QString stepDisplayName();

private:
    void ctor();

    RemoteLinux::AbstractRemoteLinuxDeployService *deployService() const;
    bool initInternal(QString *error = 0);

    MerEmulatorStartService *m_service;
};

} // Internal
} // Mer

#endif // MEREMULATORSTARTSTEP_H
