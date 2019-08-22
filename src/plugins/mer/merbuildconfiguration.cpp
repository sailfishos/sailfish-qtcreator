/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "merbuildconfiguration.h"

#include "merconstants.h"
#include "merbuildsteps.h"
#include "merlogging.h"
#include "mersettings.h"
#include "mersdk.h"
#include "mersdkkitinformation.h"

#include <sfdk/vmconnection.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakestep.h>
#include <utils/checkablemessagebox.h>

#include <QCheckBox>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace Sfdk;
using Utils::CheckableMessageBox;

namespace {

const char MER_VARIABLES_CACHE_FILENAME[] = ".mb2/qmake_variables.cache";

} // namespace anonymous

namespace Mer {
namespace Internal {

MerBuildConfiguration::MerBuildConfiguration(Target *target, Core::Id id)
    : QmakeBuildConfiguration(target, id)
{
    auto setupExtraParserArgumentsIfActive = [this]() {
        if (isReallyActive())
            setupExtraParserArguments();
    };

    connect(MerSettings::instance(), &MerSettings::importQmakeVariablesEnabledChanged,
            this, setupExtraParserArgumentsIfActive);

    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, setupExtraParserArgumentsIfActive);
    connect(project(), &Project::activeTargetChanged,
            this, setupExtraParserArgumentsIfActive);
    connect(target, &Target::activeBuildConfigurationChanged,
            this, setupExtraParserArgumentsIfActive);

    QmakeProject *qmakeProject = static_cast<QmakeProject *>(project());
    // Note that this is emited more than once during qmake step execution - once
    // for each executed process
    connect(qmakeProject, &QmakeProject::buildDirectoryInitialized,
            this, setupExtraParserArgumentsIfActive);

    connect(EditorManager::instance(), &EditorManager::saved,
            this, [this](IDocument *document) {
        if (!isReallyActive())
            return;
        QTC_ASSERT(project(), return);
        if (!project()->files(Project::AllFiles).contains(document->filePath())
                || !document->filePath().toString().contains(QRegExp("\\.spec$|\\.yaml$")))
            return;

        maybeUpdateExtraParserArguments();
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

bool MerBuildConfiguration::isReallyActive() const
{
    QTC_ASSERT(project(), return false);
    return SessionManager::startupProject() == project() && isActive();
}

void MerBuildConfiguration::setupExtraParserArguments()
{
    if (!qmakeStep())
        return;

    QStringList args;
    if (MerSettings::isImportQmakeVariablesEnabled()) {
        QFile file(buildDirectory().toString() + "/" + MER_VARIABLES_CACHE_FILENAME);
        if (file.exists() && file.open(QIODevice::ReadOnly)) {
            QByteArray rawArgs = file.readAll();
            if (rawArgs.endsWith('\0'))
                rawArgs.chop(1);
            args = Utils::transform(rawArgs.split('\0'),
                                    QOverload<const QByteArray &>::of(QString::fromUtf8));
        }
    }

    if (Log::qmakeArgs().isDebugEnabled()) {
        qCDebug(Log::qmakeArgs) << "Setting up extra parser arguments for" << displayName()
            << "under target" << target()->displayName() << "as:";
        for (const QString &arg : qAsConst(args))
            qCDebug(Log::qmakeArgs) << "    " << arg;
    }

    qmakeStep()->setExtraParserArguments(args);
}

void MerBuildConfiguration::maybeUpdateExtraParserArguments()
{
    if (m_qmakeQuestion)
        return; // already asking

    if (!MerSettings::isImportQmakeVariablesEnabled())
        return;

    if (!MerSettings::isAskImportQmakeVariablesEnabled()) {
        updateExtraParserArguments();
        return;
    }

    m_qmakeQuestion = new QMessageBox(
            QMessageBox::Question,
            tr("Run qmake?"),
            tr("Run qmake to detect possible additional qmake arguments now?")
                + QString(45, ' '), // more horizontal space for the informative text
            QMessageBox::Yes | QMessageBox::No,
            ICore::mainWindow());
    m_qmakeQuestion->setInformativeText(tr("Additional qmake arguments that may influence project "
                "parsing may be introduced at the rpmbuild level. These are recognized only after "
                "qmake is executed and the effect is local to the active build configuration - "
                "you may need to run qmake manually when project parsing fails to deliver the "
                "expected results after switching to another build configuration."));
    m_qmakeQuestion->setCheckBox(new QCheckBox(CheckableMessageBox::msgDoNotAskAgain()));
    connect(m_qmakeQuestion, &QMessageBox::finished, [=](int result) {
        if (m_qmakeQuestion->checkBox()->isChecked())
            MerSettings::setAskImportQmakeVariablesEnabled(false);
        if (result == QMessageBox::Yes)
            updateExtraParserArguments();
    });
    m_qmakeQuestion->setEscapeButton(QMessageBox::No);
    m_qmakeQuestion->setAttribute(Qt::WA_DeleteOnClose);
    m_qmakeQuestion->show();
    m_qmakeQuestion->raise();
}

void MerBuildConfiguration::updateExtraParserArguments()
{
    const MerSdk *const merSdk = MerSdkKitInformation::sdk(target()->kit());
    QTC_ASSERT(merSdk, return);
    QTC_ASSERT(merSdk->connection(), return);

    if (merSdk->connection()->state() != VmConnection::Connected) {
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
    return true;
}

MerBuildConfigurationFactory::MerBuildConfigurationFactory()
{
    registerBuildConfiguration<MerBuildConfiguration>(QmakeProjectManager::Constants::QMAKE_BC_ID);
    addSupportedTargetDeviceType(Constants::MER_DEVICE_TYPE);
}

} // namespace Internal
} // namespace Mer
