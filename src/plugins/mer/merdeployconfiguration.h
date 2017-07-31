/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

//#include <projectexplorer/deployconfiguration.h>
#include <remotelinux/remotelinuxdeployconfiguration.h>

#include <QBasicTimer>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace ProjectExplorer {
class Project;
}

namespace Mer {
namespace Internal {

class MerDeployConfiguration : public RemoteLinux::RemoteLinuxDeployConfiguration
{
    Q_OBJECT
public:
    MerDeployConfiguration(ProjectExplorer::Target *parent, Core::Id id,const QString& displayName);
    MerDeployConfiguration(ProjectExplorer::Target *target, MerDeployConfiguration *source);

public slots:
    void addRemoveSpecialSteps();

protected:
    void timerEvent(QTimerEvent *event) override;
    virtual void doAddRemoveSpecialSteps();

    static bool isAmbienceProject(ProjectExplorer::Project *project);
    static void removeStep(ProjectExplorer::BuildStepList *stepList, Core::Id stepId);

private:
    void init();

    QBasicTimer m_addRemoveSpecialStepsTimer;
};

class MerRpmDeployConfiguration : public MerDeployConfiguration
{
    Q_OBJECT

public:
    static QString displayName();
    static Core::Id configurationId();

protected:
    void doAddRemoveSpecialSteps() override;

private:
    MerRpmDeployConfiguration(ProjectExplorer::Target *parent, Core::Id id);
    MerRpmDeployConfiguration(ProjectExplorer::Target *target, MerRpmDeployConfiguration *source);
    void init();
    friend class MerDeployConfigurationFactory;
};

class MerRsyncDeployConfiguration : public  MerDeployConfiguration
{
    Q_OBJECT

public:
    static QString displayName();
    static Core::Id configurationId();

protected:
    void doAddRemoveSpecialSteps() override;

private:
    MerRsyncDeployConfiguration(ProjectExplorer::Target *parent, Core::Id id);
    MerRsyncDeployConfiguration(ProjectExplorer::Target *target, MerRsyncDeployConfiguration *source);
    friend class MerDeployConfigurationFactory;
};
//TODO: Hack
class MerMb2RpmBuildConfiguration : public  MerDeployConfiguration
{
    Q_OBJECT

public:
    static QString displayName();
    static Core::Id configurationId();

protected:
    void doAddRemoveSpecialSteps() override;

private:
    MerMb2RpmBuildConfiguration(ProjectExplorer::Target *parent, Core::Id id);
    MerMb2RpmBuildConfiguration(ProjectExplorer::Target *target, MerMb2RpmBuildConfiguration *source);
    friend class MerDeployConfigurationFactory;
};

class MerRpmBuildDeployConfiguration : public MerDeployConfiguration
{
    Q_OBJECT

    public:
        static QString displayName();
        static Core::Id configurationId();

    private:
        MerRpmBuildDeployConfiguration(ProjectExplorer::Target *parent, Core::Id id);
        MerRpmBuildDeployConfiguration(ProjectExplorer::Target *target, MerRpmBuildDeployConfiguration *source);
        friend class MerDeployConfigurationFactory;
};

} // namespace Internal
} // namespace Mer

#endif // MERDEPLOYCONFIGURATION_H
