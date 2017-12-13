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

#include "remotelinuxplugin.h"

#include "embeddedlinuxqtversionfactory.h"
#include "genericlinuxdeviceconfigurationfactory.h"
#include "genericremotelinuxdeploystepfactory.h"
#include "remotelinuxanalyzesupport.h"
#include "remotelinuxcustomrunconfiguration.h"
#include "remotelinuxdebugsupport.h"
#include "remotelinuxdeployconfigurationfactory.h"
#include "remotelinuxrunconfiguration.h"
#include "remotelinuxrunconfigurationfactory.h"

#include <QtPlugin>

namespace RemoteLinux {
namespace Internal {

RemoteLinuxPlugin::RemoteLinuxPlugin()
{
    setObjectName(QLatin1String("RemoteLinuxPlugin"));
}

bool RemoteLinuxPlugin::initialize(const QStringList &arguments,
    QString *errorMessage)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorMessage)

    using namespace ProjectExplorer;
    using namespace ProjectExplorer::Constants;

    auto constraint = [](RunConfiguration *runConfig) {
        const Core::Id id = runConfig->id();
        return id == RemoteLinuxCustomRunConfiguration::runConfigId()
            || id.name().startsWith(RemoteLinuxRunConfiguration::IdPrefix);
    };

    RunControl::registerWorker<SimpleTargetRunner>(NORMAL_RUN_MODE, constraint);
    RunControl::registerWorker<LinuxDeviceDebugSupport>(DEBUG_RUN_MODE, constraint);
    RunControl::registerWorker<RemoteLinuxQmlProfilerSupport>(QML_PROFILER_RUN_MODE, constraint);
    //RunControl::registerWorker<RemoteLinuxPerfSupport>(PERFPROFILER_RUN_MODE, constraint);

    addAutoReleasedObject(new GenericLinuxDeviceConfigurationFactory);
    addAutoReleasedObject(new RemoteLinuxRunConfigurationFactory);
    addAutoReleasedObject(new RemoteLinuxDeployConfigurationFactory);
    addAutoReleasedObject(new GenericRemoteLinuxDeployStepFactory);

    addAutoReleasedObject(new EmbeddedLinuxQtVersionFactory);

    return true;
}

RemoteLinuxPlugin::~RemoteLinuxPlugin()
{
}

void RemoteLinuxPlugin::extensionsInitialized()
{
}

} // namespace Internal
} // namespace RemoteLinux
