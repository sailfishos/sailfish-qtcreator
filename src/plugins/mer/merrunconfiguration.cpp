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

#include "merrunconfiguration.h"

#include <projectexplorer/target.h>

namespace Mer {
namespace Internal {

MerRunConfiguration::MerRunConfiguration(ProjectExplorer::Target *parent, const Core::Id id,
                                         const QString &proFilePath)
    : RemoteLinux::RemoteLinuxRunConfiguration(parent, id, proFilePath)
{
}

MerRunConfiguration::MerRunConfiguration(ProjectExplorer::Target *parent,
                                         MerRunConfiguration *source)
    : RemoteLinux::RemoteLinuxRunConfiguration(parent, source)
{
}

bool MerRunConfiguration::isEnabled() const
{
    if (!target()->activeDeployConfiguration())
        return false;

    return RemoteLinuxRunConfiguration::isEnabled();
}

} // Internal
} // Mer
