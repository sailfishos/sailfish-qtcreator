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

#include "merqmakebuildconfiguration.h"

#include "merconstants.h"
#include "merbuildsteps.h"
#include "merlogging.h"
#include "mersettings.h"
#include "mersdkkitinformation.h"

#include <sfdk/buildengine.h>
#include <sfdk/virtualmachine.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/modemanager.h>
#include <debugger/debuggerconstants.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakestep.h>
#include <utils/checkablemessagebox.h>

#include <QApplication>
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

MerQmakeBuildConfiguration::MerQmakeBuildConfiguration(Target *target, Core::Id id)
    : QmakeBuildConfiguration(target, id)
{
    connect(MerSettings::instance(), &MerSettings::importQmakeVariablesEnabledChanged,
            this, &MerQmakeBuildConfiguration::maybeSetupExtraParserArguments);

    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &MerQmakeBuildConfiguration::maybeSetupExtraParserArguments);
    connect(project(), &Project::activeTargetChanged,
            this, &MerQmakeBuildConfiguration::maybeSetupExtraParserArguments);
    connect(target, &Target::activeBuildConfigurationChanged,
            this, &MerQmakeBuildConfiguration::maybeSetupExtraParserArguments);
    connect(ModeManager::instance(), &ModeManager::currentModeChanged,
            this, &MerQmakeBuildConfiguration::maybeSetupExtraParserArguments);

    QmakeProject *qmakeProject = static_cast<QmakeProject *>(project());
    // Note that this is emited more than once during qmake step execution - once
    // for each executed process
    connect(qmakeProject, &QmakeProject::buildDirectoryInitialized,
            this, &MerQmakeBuildConfiguration::maybeSetupExtraParserArguments);

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

void MerQmakeBuildConfiguration::initialize(const ProjectExplorer::BuildInfo &info)
{
    QmakeBuildConfiguration::initialize(info);

    BuildStepList *buildSteps = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_BUILD));
    Q_ASSERT(buildSteps);
    buildSteps->insertStep(0, new MerSdkStartStep(buildSteps));

    BuildStepList *cleanSteps = stepList(Core::Id(ProjectExplorer::Constants::BUILDSTEPS_CLEAN));
    Q_ASSERT(cleanSteps);
    cleanSteps->insertStep(0, new MerSdkStartStep(cleanSteps));
}

void MerQmakeBuildConfiguration::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_maybeUpdateExtraParserArgumentsTimer.timerId()) {
        m_maybeUpdateExtraParserArgumentsTimer.stop();
        const bool now = true;
        maybeUpdateExtraParserArguments(now);
    } else  {
        QmakeBuildConfiguration::timerEvent(event);
    }
}

bool MerQmakeBuildConfiguration::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Close) {
        QTimer::singleShot(0, this, &MerQmakeBuildConfiguration::maybeSetupExtraParserArguments);
        watched->removeEventFilter(this);
    }

    return QmakeBuildConfiguration::eventFilter(watched, event);
}

bool MerQmakeBuildConfiguration::isReallyActive() const
{
    QTC_ASSERT(project(), return false);
    return SessionManager::startupProject() == project() && isActive();
}

void MerQmakeBuildConfiguration::maybeSetupExtraParserArguments()
{
    if (!isReallyActive())
        return;

    // Most importantly, avoid taking actions on temporary changes in Projects mode, but
    // more cases are covered
    if (ModeManager::currentModeId() != Core::Constants::MODE_EDIT &&
            ModeManager::currentModeId() != Debugger::Constants::MODE_DEBUG) {
        return;
    }

    // Most importantly, avoid taking actions on temporary changes when the mini
    // project target selector is open, but more cases are covered
    if (QApplication::activePopupWidget()) {
        QApplication::activePopupWidget()->installEventFilter(this);
        return;
    }

    if (!qmakeStep())
        return;

    if (project()->needsConfiguration())
        return;

    setupExtraParserArguments();
};

void MerQmakeBuildConfiguration::setupExtraParserArguments()
{
    QTC_ASSERT(qmakeStep(), return);
    QTC_ASSERT(!project()->needsConfiguration(), return);

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

void MerQmakeBuildConfiguration::maybeUpdateExtraParserArguments(bool now)
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
    m_qmakeQuestion->setInformativeText(tr("Additional qmake arguments may be "
                "introduced at the rpmbuild level. The qmake build step needs to "
                "be executed to recognize these and augment the project model "
                "with this information."
                "\n\n"
                "(You may need to re-run qmake manually when you find project "
                "parsing inaccurate after changes to the build configuration.)"));
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

void MerQmakeBuildConfiguration::updateExtraParserArguments()
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

    qs->setForced(true); // gets cleared automatically
    qs->setRecursive(false);
    BuildManager::appendStep(qs, tr("Updating cache"));
    qs->setRecursive(true);
}

bool MerQmakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!QmakeBuildConfiguration::fromMap(map))
        return false;
    return true;
}

MerQmakeBuildConfigurationFactory::MerQmakeBuildConfigurationFactory()
{
    registerBuildConfiguration<MerQmakeBuildConfiguration>(QmakeProjectManager::Constants::QMAKE_BC_ID);
    addSupportedTargetDeviceType(Constants::MER_DEVICE_TYPE);
}

} // namespace Internal
} // namespace Mer
