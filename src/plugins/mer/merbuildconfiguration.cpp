/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "merbuildconfiguration.h"

#include "merconnection.h"
#include "merconstants.h"
#include "merbuildsteps.h"
#include "mersettings.h"
#include "mersdk.h"
#include "mersdkkitinformation.h"

#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakestep.h>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;

namespace {

const char MER_VARIABLES_CACHE_FILENAME[] = ".mb2/qmake_variables.cache";

} // namespace anonymous

namespace Mer {
namespace Internal {

MerBuildConfiguration::MerBuildConfiguration(Target *target, Core::Id id)
    : QmakeBuildConfiguration(target, id)
{
    QmakeProject *qmakeProject = static_cast<QmakeProject *>(project());
    connect(qmakeProject, &QmakeProject::buildDirectoryInitialized,
            this, &MerBuildConfiguration::setupExtraParserArguments);

    connect(EditorManager::instance(), &EditorManager::aboutToSave,
            this, [this](IDocument *document) {
        QTC_ASSERT(project(), return);
        if (!project()->files(Project::AllFiles).contains(document->filePath())
                || !document->filePath().toString().contains(QRegExp("\\.spec$|\\.yaml$")))
            return;

        updateExtraParserArguments();
    });

    connect(MerSettings::instance(), &MerSettings::importQmakeVariablesEnabledChanged,
            [this]() {
        setupExtraParserArguments();
    });
}

void MerBuildConfiguration::initialize(const ProjectExplorer::BuildInfo &info)
{
    QmakeBuildConfiguration::initialize(info);

    BuildStepList *buildSteps = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    Q_ASSERT(buildSteps);
    buildSteps->insertStep(0, new MerSdkStartStep(buildSteps));

    BuildStepList *cleanSteps = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    Q_ASSERT(cleanSteps);
    cleanSteps->insertStep(0, new MerSdkStartStep(cleanSteps));
}

void MerBuildConfiguration::setupExtraParserArguments()
{
    if (!qmakeStep())
        return;

    QStringList args;
    if (MerSettings::isImportQmakeVariablesEnabled()) {
        QFile file(buildDirectory().toString() + "/" + MER_VARIABLES_CACHE_FILENAME);
        if (file.exists() && file.open(QIODevice::ReadOnly)) {
            args = Utils::transform(file.readAll().split('\0'),
                                    QOverload<const QByteArray &>::of(QString::fromUtf8));
        }
    }

    qmakeStep()->setExtraParserArguments(args);
}

void MerBuildConfiguration::updateExtraParserArguments() const
{
    if (!MerSettings::isImportQmakeVariablesEnabled())
        return;

    const MerSdk *const merSdk = MerSdkKitInformation::sdk(target()->kit());
    QTC_ASSERT(merSdk, return);
    QTC_ASSERT(merSdk->connection(), return);

    if (merSdk->connection()->state() != Mer::MerConnection::Connected) {
        BuildStepList *steps = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
        QTC_ASSERT(steps, return);
        Mer::Internal::MerSdkStartStep *sdkStartStep = steps->firstOfType<Mer::Internal::MerSdkStartStep>();
        QTC_ASSERT(sdkStartStep, return);

        BuildManager::appendStep(sdkStartStep, sdkStartStep->displayName());
    }

    QMakeStep *qs = qmakeStep();
    QTC_ASSERT(qs, return);
    BuildManager::appendStep(qs, tr("Updating cache"));
}

bool MerBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!QmakeBuildConfiguration::fromMap(map))
        return false;
    setupExtraParserArguments();
    return true;
}

MerBuildConfigurationFactory::MerBuildConfigurationFactory()
{
    registerBuildConfiguration<MerBuildConfiguration>(QmakeProjectManager::Constants::QMAKE_BC_ID);
    addSupportedTargetDeviceType(Constants::MER_DEVICE_TYPE);
}

} // namespace Internal
} // namespace Mer
