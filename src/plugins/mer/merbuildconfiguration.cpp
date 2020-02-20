/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
** Copyright (C) 2019-2020 Open Mobile Platform LLC.
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
#include "mersdkkitinformation.h"

#include <sfdk/buildengine.h>
#include <sfdk/virtualmachine.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
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

// Avoid actions on temporary changes and asking questions too hastily
const int UPDATE_EXTRA_PARSER_ARGUMENTS_DELAY_MS = 3000;

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
        if (!document->filePath().toString().contains(QRegExp("\\.spec$|\\.yaml$")))
            return;
        if (!project()->files(Project::AllFiles).contains(document->filePath())) {
            if (document->filePath().isChildOf(project()->rootProjectDirectory())) {
                MessageManager::write(tr("Warning: RPM *.spec (or *.yaml) file not registered to the project."),
                        MessageManager::Flash);
            }
            return;
        }

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

void MerBuildConfiguration::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_maybeUpdateExtraParserArgumentsTimer.timerId()) {
        m_maybeUpdateExtraParserArgumentsTimer.stop();
        const bool now = true;
        maybeUpdateExtraParserArguments(now);
    } else  {
        QmakeBuildConfiguration::timerEvent(event);
    }
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
        if (!file.exists()) {
            // Do not ask again
            const bool mkpathOk = QFileInfo(file.fileName()).dir().mkpath(".");
            QTC_CHECK(mkpathOk);
            const bool createdOk = file.open(QIODevice::WriteOnly);
            QTC_CHECK(createdOk);

            maybeUpdateExtraParserArguments();
            return;
        } else {
            const bool openOk = file.open(QIODevice::ReadOnly);
            QTC_ASSERT(openOk, return);
            QByteArray rawArgs = file.readAll();
            if (rawArgs.endsWith('\0'))
                rawArgs.chop(1);
            args = Utils::transform(rawArgs.split('\0'),
                                    QOverload<const QByteArray &>::of(QString::fromUtf8));
        }
    }

    if (Log::qmakeArgs().isDebugEnabled()) {
        if (qmakeStep()->extraParserArguments() == args) {
            qCDebug(Log::qmakeArgs) << "Preserving extra parser arguments for" << displayName()
                << "under target" << target()->displayName() << "as:";
        } else {
            qCDebug(Log::qmakeArgs) << "Changing extra parser arguments for" << displayName()
                << "under target" << target()->displayName() << "to:";
        }
        for (const QString &arg : qAsConst(args))
            qCDebug(Log::qmakeArgs) << "    " << arg;
    }

    if (qmakeStep()->extraParserArguments() != args) {
        qmakeStep()->setExtraParserArguments(args);
        auto *const qmakeProject = static_cast<QmakeProject *>(project());
        qmakeProject->rootProFile()->scheduleUpdate(QmakeProFile::ParseLater);
    }
}

void MerBuildConfiguration::maybeUpdateExtraParserArguments(bool now)
{
    if (!now) {
        m_maybeUpdateExtraParserArgumentsTimer.start(UPDATE_EXTRA_PARSER_ARGUMENTS_DELAY_MS, this);
        return;
    }

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
    const BuildEngine *const buildEngine = MerSdkKitInformation::buildEngine(target()->kit());
    QTC_ASSERT(buildEngine, return);

    if (buildEngine->virtualMachine()->state() != VirtualMachine::Connected) {
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
