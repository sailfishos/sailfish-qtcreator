/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <qmakeprojectmanager/qmakebuildconfiguration.h>

namespace QmakeAndroidSupport {
namespace Internal {

class AndroidQmakeBuildConfigurationFactory : public QmakeProjectManager::QmakeBuildConfigurationFactory
{
public:
    explicit AndroidQmakeBuildConfigurationFactory(QObject *parent = 0)
        : QmakeBuildConfigurationFactory(parent)
    { }

    int priority(const ProjectExplorer::Kit *k, const QString &projectPath) const override;
    int priority(const ProjectExplorer::Target *parent) const override;

    ProjectExplorer::BuildConfiguration *create(ProjectExplorer::Target *parent,
                                                const ProjectExplorer::BuildInfo *info) const override;
    ProjectExplorer::BuildConfiguration *clone(ProjectExplorer::Target *parent, ProjectExplorer::BuildConfiguration *source) override;
    ProjectExplorer::BuildConfiguration *restore(ProjectExplorer::Target *parent, const QVariantMap &map) override;
};

class AndroidQmakeBuildConfiguration : public QmakeProjectManager::QmakeBuildConfiguration
{
    friend class AndroidQmakeBuildConfigurationFactory;
    Q_OBJECT
public:
    explicit AndroidQmakeBuildConfiguration(ProjectExplorer::Target *target);
    AndroidQmakeBuildConfiguration(ProjectExplorer::Target *target, AndroidQmakeBuildConfiguration *source);
    AndroidQmakeBuildConfiguration(ProjectExplorer::Target *target, Core::Id id);
    void addToEnvironment(Utils::Environment &env) const override;
    void manifestSaved();

    using BuildConfiguration::emitEnvironmentChanged;
private:
    mutable QString m_androidNdkPlatform;
};

} // namespace Internal
} // namespace QmakeAndroidSupport
