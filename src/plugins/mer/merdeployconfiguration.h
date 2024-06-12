/****************************************************************************
**
** Copyright (C) 2012-2014,2017-2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef MERDEPLOYCONFIGURATION_H
#define MERDEPLOYCONFIGURATION_H

#include "merconstants.h"

#include "merprojectlistener.h"

#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/deploymentdataview.h>

#include <QBasicTimer>
#include <QQueue>

namespace CMakeProjectManager {
class CMakeProject;
}

namespace QmakeProjectManager {
class QmakeProject;
}

namespace Mer {
namespace Internal {

template<class T>
T *earlierBuildStep(ProjectExplorer::DeployConfiguration *deployConfiguration,
        const ProjectExplorer::BuildStep *laterBuildStep)
{
    for (ProjectExplorer::BuildStep *step : deployConfiguration->stepList()->steps()) {
        if (step == laterBuildStep)
            break;
        if (T *const step_ = dynamic_cast<T *>(step))
            return step_;
    }
    return nullptr;
}

class MerRpmDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_DECLARE_TR_FUNCTIONS(Mer::Internal::MerRpmDeployConfigurationFactory)

public:
    MerRpmDeployConfigurationFactory();

    static QString displayName();
    static Utils::Id configurationId();
};

class MerRsyncDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_DECLARE_TR_FUNCTIONS(Mer::Internal::MerRsyncDeployConfigurationFactory)

public:
    MerRsyncDeployConfigurationFactory();

    static QString displayName();
    static Utils::Id configurationId();
};

//TODO: Hack
class MerMb2RpmBuildConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_DECLARE_TR_FUNCTIONS(Mer::Internal::MerMb2DeployConfigurationFactory)

public:
    MerMb2RpmBuildConfigurationFactory();

    static QString displayName();
    static Utils::Id configurationId();
};

class MerAddRemoveSpecialDeployStepsProjectListener : public MerProjectListener
{
    Q_OBJECT

public:
    using MerProjectListener::MerProjectListener;

protected:
    bool handleProject(ProjectExplorer::Project *project) override;
    bool forgetProject(ProjectExplorer::Project *project) override;

protected:
    void timerEvent(QTimerEvent *event) override;

private slots:
    void scheduleTargetUpdate();

private:
    void updateTarget(ProjectExplorer::Target *target);
    static bool isAmbienceProject(ProjectExplorer::Target *target);
    static void removeStep(ProjectExplorer::BuildStepList *stepList, Utils::Id stepId);

private:
    QQueue<QPointer<ProjectExplorer::Target>> m_updateTargetsQueue;
    QBasicTimer m_updateTargetsTimer;
};

} // namespace Internal
} // namespace Mer

#endif // MERDEPLOYCONFIGURATION_H
