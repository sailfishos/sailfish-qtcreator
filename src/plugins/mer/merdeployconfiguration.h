/****************************************************************************
**
** Copyright (C) 2012-2014,2017-2019 Jolla Ltd.
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

template<class Configuration>
class MerDeployConfigurationFactory : public ProjectExplorer::DeployConfigurationFactory
{
    Q_DECLARE_TR_FUNCTIONS(Mer::Internal::MerDeployConfigurationFactory)

public:
    MerDeployConfigurationFactory()
    {
        setConfigBaseId(Configuration::configurationId());
        addSupportedTargetDeviceType(Constants::MER_DEVICE_TYPE);
        setDefaultDisplayName(Configuration::displayName());
        setConfigWidgetCreator([](ProjectExplorer::Target *target) {
            return new ProjectExplorer::DeploymentDataView(target);
        });
    }
};

class MerRpmDeployConfigurationFactory
    : public MerDeployConfigurationFactory<MerRpmDeployConfigurationFactory>
{
public:
    MerRpmDeployConfigurationFactory();

    static QString displayName();
    static Core::Id configurationId();
};

class MerRsyncDeployConfigurationFactory
    : public MerDeployConfigurationFactory<MerRsyncDeployConfigurationFactory>
{
public:
    MerRsyncDeployConfigurationFactory();

    static QString displayName();
    static Core::Id configurationId();
};

//TODO: Hack
class MerMb2RpmBuildConfigurationFactory
    : public MerDeployConfigurationFactory<MerMb2RpmBuildConfigurationFactory>
{
public:
    MerMb2RpmBuildConfigurationFactory();

    static QString displayName();
    static Core::Id configurationId();
};

class MerAddRemoveSpecialDeployStepsProjectListener : public MerProjectListener
{
    Q_OBJECT

public:
    using MerProjectListener::MerProjectListener;

protected:
    bool handleProject(QmakeProjectManager::QmakeProject *project) override;
    bool forgetProject(ProjectExplorer::Project *project) override;

protected:
    void timerEvent(QTimerEvent *event) override;

private slots:
    void scheduleProjectUpdate();

private:
    void updateProject(QmakeProjectManager::QmakeProject *project);
    static bool isAmbienceProject(QmakeProjectManager::QmakeProject *project);
    static void removeStep(ProjectExplorer::BuildStepList *stepList, Core::Id stepId);

private:
    QQueue<QmakeProjectManager::QmakeProject *> m_updateProjectsQueue;
    QBasicTimer m_updateProjectsTimer;
};

} // namespace Internal
} // namespace Mer

#endif // MERDEPLOYCONFIGURATION_H
