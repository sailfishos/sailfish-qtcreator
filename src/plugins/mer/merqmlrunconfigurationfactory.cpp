/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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

#include "merqmlrunconfigurationfactory.h"

#include "merconstants.h"
#include "merdevicefactory.h"
#include "merqmlrunconfiguration.h"

#include <projectexplorer/buildtargetinfo.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <utils/qtcassert.h>

using namespace Mer::Constants;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace Utils;

namespace Mer {
namespace Internal {

namespace {
const char SAILFISHAPP_QML_CONFIG[] = "sailfishapp_qml";
} // anonymous namespace

MerQmlRunConfigurationFactory::MerQmlRunConfigurationFactory(QObject *parent)
    : IRunConfigurationFactory(parent)
{
    setObjectName("MerQmlRunConfigurationFactory");
    registerRunConfiguration<MerQmlRunConfiguration>(MER_QMLRUNCONFIGURATION);
    setSupportedTargetDeviceTypes({MER_DEVICE_TYPE});
    addFixedBuildTarget(tr("QML Scene (on Sailfish OS Device)"));
}

bool MerQmlRunConfigurationFactory::canCreateHelper(Target *parent, const QString &buildTarget) const
{
    Q_UNUSED(buildTarget);

    QmakeProject *qmakeProject = qobject_cast<QmakeProject *>(parent->project());
    if (!qmakeProject)
        return false;

    QmakeProFile *root = qmakeProject->rootProFile();
    if (root->projectType() != ProjectType::AuxTemplate ||
            ! root->variableValue(Variable::Config).contains(QLatin1String(SAILFISHAPP_QML_CONFIG))) {
        return false;
    }

    return true;
}

} // Internal
} // Mer
