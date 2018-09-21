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

#include "winrtrunconfiguration.h"
#include "winrtrunconfigurationwidget.h"
#include "winrtconstants.h"

#include <coreplugin/icore.h>

#include <projectexplorer/target.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/runnables.h>

#include <qmakeprojectmanager/qmakeproject.h>

namespace WinRt {
namespace Internal {

static const char uninstallAfterStopIdC[] = "WinRtRunConfigurationUninstallAfterStopId";

WinRtRunConfiguration::WinRtRunConfiguration(ProjectExplorer::Target *target)
    : RunConfiguration(target, Constants::WINRT_RC_PREFIX)
{
    setDisplayName(tr("Run App Package"));
    addExtraAspect(new ProjectExplorer::ArgumentsAspect(this, "WinRtRunConfigurationArgumentsId"));
}

QString WinRtRunConfiguration::extraId() const
{
    return m_proFilePath;
}

QWidget *WinRtRunConfiguration::createConfigurationWidget()
{
    return new WinRtRunConfigurationWidget(this);
}

QVariantMap WinRtRunConfiguration::toMap() const
{
    QVariantMap map = RunConfiguration::toMap();
    map.insert(QLatin1String(uninstallAfterStopIdC), m_uninstallAfterStop);
    return map;
}

bool WinRtRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    setUninstallAfterStop(map.value(QLatin1String(uninstallAfterStopIdC)).toBool());
    m_proFilePath = ProjectExplorer::idFromMap(map).suffixAfter(id());
    return true;
}

QString WinRtRunConfiguration::arguments() const
{
    return extraAspect<ProjectExplorer::ArgumentsAspect>()->arguments();
}

void WinRtRunConfiguration::setUninstallAfterStop(bool b)
{
    m_uninstallAfterStop = b;
    emit uninstallAfterStopChanged(m_uninstallAfterStop);
}

QString WinRtRunConfiguration::buildSystemTarget() const
{
    return m_proFilePath;
}

ProjectExplorer::Runnable WinRtRunConfiguration::runnable() const
{
    ProjectExplorer::StandardRunnable r;
    r.executable = executable();
    r.commandLineArguments = arguments();
    return r;
}

QString WinRtRunConfiguration::executable() const
{
    QmakeProjectManager::QmakeProject *project
            = static_cast<QmakeProjectManager::QmakeProject *>(target()->project());
    if (!project)
        return QString();

    QmakeProjectManager::QmakeProFile *rootProFile = project->rootProFile();
    if (!rootProFile)
        return QString();

    const QmakeProjectManager::QmakeProFile *pro
            = rootProFile->findProFile(Utils::FileName::fromString(m_proFilePath));
    if (!pro)
        return QString();

    QmakeProjectManager::TargetInformation ti = pro->targetInformation();
    if (!ti.valid)
        return QString();

    QString destDir = ti.destDir.toString();
    if (destDir.isEmpty())
        destDir = ti.buildDir.toString();
    else if (QDir::isRelativePath(destDir))
        destDir = QDir::cleanPath(ti.buildDir.toString() + '/' + destDir);

    QString executable = QDir::cleanPath(destDir + '/' + ti.target);
    executable = Utils::HostOsInfo::withExecutableSuffix(executable);
    return executable;
}

} // namespace Internal
} // namespace WinRt
