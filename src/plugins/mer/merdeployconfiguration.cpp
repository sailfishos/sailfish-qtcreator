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

#include "merdeployconfiguration.h"

#include <coreplugin/idocument.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>

#include <QLabel>
#include <QVBoxLayout>

using namespace ProjectExplorer;
using namespace RemoteLinux;

namespace Mer {
namespace Internal {

MerDeployConfiguration::MerDeployConfiguration(Target *parent, Core::Id id,const QString& displayName)
    : RemoteLinuxDeployConfiguration(parent, id, displayName)
{
    setDisplayName(displayName);
    setDefaultDisplayName(displayName);
}

MerDeployConfiguration::MerDeployConfiguration(Target *target, MerDeployConfiguration *source)
    : RemoteLinuxDeployConfiguration(target, source)
{
    cloneSteps(source);
}

////////////////////////////////////////////////////////////////////////////////////

MerRpmDeployConfiguration::MerRpmDeployConfiguration(Target *parent, Core::Id id)
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
    return Core::Id("QmakeProjectManager.MerRpmDeployConfiguration");
}

////////////////////////////////////////////////////////////////////////////////////////////

MerRsyncDeployConfiguration::MerRsyncDeployConfiguration(Target *parent, Core::Id id)
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
    return Core::Id("QmakeProjectManager.MerRSyncDeployConfiguration");
}

/////////////////////////////////////////////////////////////////////////////////////
//TODO:HACK
MerMb2RpmBuildConfiguration::MerMb2RpmBuildConfiguration(Target *parent, Core::Id id)
    : MerDeployConfiguration(parent, id,displayName())
{
}

MerMb2RpmBuildConfiguration::MerMb2RpmBuildConfiguration(Target *target, MerMb2RpmBuildConfiguration *source)
    : MerDeployConfiguration(target, source)
{
}

QString MerMb2RpmBuildConfiguration::displayName()
{
    return tr("Deploy By Building An RPM Package");
}

Core::Id MerMb2RpmBuildConfiguration::configurationId()
{
    return Core::Id("QmakeProjectManager.MerMb2RpmBuildConfiguration");
}

////////////////////////////////////////////////////////////////////////////////////

MerRpmBuildDeployConfiguration::MerRpmBuildDeployConfiguration(Target *parent, Core::Id id)
    : MerDeployConfiguration(parent, id, displayName())
{

}

MerRpmBuildDeployConfiguration::MerRpmBuildDeployConfiguration(Target *target, MerRpmBuildDeployConfiguration *source)
    : MerDeployConfiguration(target, source)
{

}

QString MerRpmBuildDeployConfiguration::displayName()
{
    return tr("Deploy As RPM Package (RPMBUILD)");
}

Core::Id MerRpmBuildDeployConfiguration::configurationId()
{
    return Core::Id("QmakeProjectManager.MerRpmLocalDeployConfiguration");
}

} // namespace Internal
} // namespace Mer
