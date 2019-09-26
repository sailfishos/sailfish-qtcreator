/****************************************************************************
**
** Copyright (C) 2014-2015,2017-2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "merbuildsteps.h"

#include "mersdkkitinformation.h"
#include "mer/merconstants.h"

#include <sfdk/buildengine.h>

#include <projectexplorer/target.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Sfdk;

namespace Mer {
namespace Internal {

MerSdkStartStep::MerSdkStartStep(BuildStepList *bsl)
    : MerAbstractVmStartStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}

bool MerSdkStartStep::init()
{
    const BuildEngine *const engine = MerSdkKitInformation::buildEngine(target()->kit());
    if (!engine) {
        addOutput(tr("Cannot start SDK: Missing %1 build-engine information in the kit").arg(QCoreApplication::translate("Mer", Mer::Constants::MER_OS_NAME)),
                OutputFormat::ErrorMessage);
        return false;
    }

    setVirtualMachine(engine->virtualMachine());

    return MerAbstractVmStartStep::init();
}

Core::Id MerSdkStartStep::stepId()
{
    return Core::Id("Mer.MerSdkStartStep");
}

QString MerSdkStartStep::displayName()
{
    return tr("Start Build Engine");
}

} // namespace Internal
} // namespace Mer
