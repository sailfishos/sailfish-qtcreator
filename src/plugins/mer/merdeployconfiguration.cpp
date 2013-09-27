/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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

#include "merdeployconfiguration.h"
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <coreplugin/idocument.h>
#include <QVBoxLayout>
#include <QLabel>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerDeployConfiguration::MerDeployConfiguration(Target *parent, const Core::Id id,const QString& displayName)
    : ProjectExplorer::DeployConfiguration(parent, id)
{
    setDisplayName(displayName);
    setDefaultDisplayName(displayName);
}

MerDeployConfiguration::MerDeployConfiguration(ProjectExplorer::Target *target, MerDeployConfiguration *source)
    : ProjectExplorer::DeployConfiguration(target, source)
{
    cloneSteps(source);
}

////////////////////////////////////////////////////////////////////////////////////

MerRpmDeployConfiguration::MerRpmDeployConfiguration(Target *parent, const Core::Id id)
    : MerDeployConfiguration(parent, id,displayName())
{
    init();
}

MerRpmDeployConfiguration::MerRpmDeployConfiguration(Target *target, MerRpmDeployConfiguration *source)
    : MerDeployConfiguration(target, source)
{
    init();
}

void MerRpmDeployConfiguration::init()
{
}

QString MerRpmDeployConfiguration::displayName()
{
    return tr("Deploy As RPM Package");
}

Core::Id MerRpmDeployConfiguration::configurationId()
{
    return Core::Id("Qt4ProjectManager.MerRpmDeployConfiguration");
}

////////////////////////////////////////////////////////////////////////////////////

MerRsyncDeployConfiguration::MerRsyncDeployConfiguration(Target *parent, const Core::Id id)
    : MerDeployConfiguration(parent, id,displayName())
{
}

MerRsyncDeployConfiguration::MerRsyncDeployConfiguration(Target *target, MerRsyncDeployConfiguration *source)
    : MerDeployConfiguration(target, source)
{
}

QString MerRsyncDeployConfiguration::displayName()
{
    return tr("Deploy By Copying Binaries");
}

Core::Id MerRsyncDeployConfiguration::configurationId()
{
    return Core::Id("Qt4ProjectManager.MerRSyncDeployConfiguration");
}

/////////////////////////////////////////////////////////////////////////////////////
//TODO:HACK
MerRpmBuildConfiguration::MerRpmBuildConfiguration(Target *parent, const Core::Id id)
    : MerDeployConfiguration(parent, id,displayName())
{
}

MerRpmBuildConfiguration::MerRpmBuildConfiguration(Target *target, MerRpmBuildConfiguration *source)
    : MerDeployConfiguration(target, source)
{
}

QString MerRpmBuildConfiguration::displayName()
{
    return tr("Deploy As RPM Package");
}

Core::Id MerRpmBuildConfiguration::configurationId()
{
    return Core::Id("Qt4ProjectManager.MerRpmBuildConfiguration");
}

} // namespace Internal
} // namespace Mer
