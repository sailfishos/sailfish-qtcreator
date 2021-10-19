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

#include "merbuildconfigurationaspect.h"
#include "merconstants.h"
#include "merbuildsteps.h"
#include "merlogging.h"
#include "mersettings.h"
#include "mersdkkitaspect.h"

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
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeproject.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <qmakeprojectmanager/qmakestep.h>
#include <utils/infobar.h>

#include <QApplication>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace Sfdk;
using namespace Utils;

namespace {

// Avoid actions on temporary changes and asking questions too hastily
const int UPDATE_EXTRA_PARSER_ARGUMENTS_DELAY_MS = 3000;

const char RUN_QMAKE_ENTRY_ID[] = "Mer.RunQmake";

} // namespace anonymous

namespace Mer {
namespace Internal {

MerQmakeBuildConfiguration::MerQmakeBuildConfiguration(Target *target, Utils::Id id)
    : QmakeBuildConfiguration(target, id)
{
    auto aspect = addAspect<MerBuildConfigurationAspect>(this);
    connect(aspect, &BaseAspect::changed,
            this, &BuildConfiguration::updateCacheAndEmitEnvironmentChanged);

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

    connect(this, &BuildConfiguration::buildDirectoryChanged,
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
                MessageManager::writeFlashing(
                        tr("Warning: RPM *.spec (or *.yaml) file not registered to the project."));
            }
            return;
        }

        maybeUpdateExtraParserArguments();
    });

    disableQmakeSystem();
}

MerQmakeBuildConfiguration::~MerQmakeBuildConfiguration()
{
    Core::ICore::infoBar()->removeInfo(RUN_QMAKE_ENTRY_ID);
}

void MerQmakeBuildConfiguration::doInitialize(const ProjectExplorer::BuildInfo &info)
{
    QmakeBuildConfiguration::doInitialize(info);

    BuildStepList *buildSteps = this->buildSteps();
    QTC_ASSERT(buildSteps, return);
    buildSteps->insertStep(0, MerSdkStartStep::stepId());

    BuildStepList *cleanSteps = this->cleanSteps();
    QTC_ASSERT(cleanSteps, return);
    cleanSteps->insertStep(0, MerSdkStartStep::stepId());
    cleanSteps->insertStep(1, MerClearBuildEnvironmentStep::stepId());
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
        QFile file(buildDirectory().toString() + "/.sfdk/qmake_variables."
                   + MerSdkKitAspect::buildTargetName(target()->kit()) + ".cache");
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
        qmakeBuildSystem()->rootProFile()->scheduleUpdate(QmakeProFile::ParseLater);
    }
}

void MerQmakeBuildConfiguration::maybeUpdateExtraParserArguments(bool now)
{
    if (!now) {
        m_maybeUpdateExtraParserArgumentsTimer.start(UPDATE_EXTRA_PARSER_ARGUMENTS_DELAY_MS, this);
        return;
    }

    if (!MerSettings::isImportQmakeVariablesEnabled())
        return;

    if (!MerSettings::isAskImportQmakeVariablesEnabled()) {
        updateExtraParserArguments();
        return;
    }

    if (!Core::ICore::infoBar()->canInfoBeAdded(RUN_QMAKE_ENTRY_ID))
        return; // already asking

    Utils::InfoBarEntry info(RUN_QMAKE_ENTRY_ID,
            tr("Run qmake to detect possible additional qmake arguments?"));
    info.setDetailsWidgetCreator([]() {
        auto *label = new QLabel(
                tr("<p>Additional qmake arguments may be introduced at the rpmbuild level. The qmake "
                "build step needs to be executed to recognize these and augment the project model "
                "with this information. (Remember to re-run qmake manually whenever you find "
                "project parsing inaccurate after changes to the build configuration.)</p>"
                "<a href='#configure'>Configure…</a>"));
        label->setWordWrap(true);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        label->setContentsMargins(0, 0, 0, 8);
        connect(label, &QLabel::linkActivated, []() {
            Core::ICore::showOptionsDialog(Mer::Constants::MER_GENERAL_OPTIONS_ID,
                    Core::ICore::dialogParent());
        });

        return label;
    });
    info.setCustomButtonInfo(tr("Run qmake"), [=]() {
        Core::ICore::infoBar()->removeInfo(RUN_QMAKE_ENTRY_ID);
        updateExtraParserArguments();
    });
    Core::ICore::infoBar()->addInfo(info);
}

void MerQmakeBuildConfiguration::updateExtraParserArguments()
{
    QMakeStep *qs = qmakeStep();
    QTC_ASSERT(qs, return);

    // If the full build or just the qmake part was started meanwhile
    if (BuildManager::isBuilding(qs)) {
        qCDebug(Log::qmakeArgs) << "qmake already running/scheduled - skipping";
        return;
    }

    const BuildEngine *const buildEngine = MerSdkKitAspect::buildEngine(target()->kit());
    QTC_ASSERT(buildEngine, return);

    if (buildEngine->virtualMachine()->state() != VirtualMachine::Connected) {
        BuildStepList *steps = buildSteps();
        QTC_ASSERT(steps, return);
        Mer::Internal::MerSdkStartStep *sdkStartStep = steps->firstOfType<Mer::Internal::MerSdkStartStep>();
        QTC_ASSERT(sdkStartStep, return);

        BuildManager::appendStep(sdkStartStep, sdkStartStep->displayName());
    }

    qs->setForced(true); // gets cleared automatically
    qs->setRecursive(false);
    BuildManager::appendStep(qs, tr("Updating cache"));
    qs->setRecursive(true);
}

void MerQmakeBuildConfiguration::disableQmakeSystem()
{
    TriStateAspect *runSystemAspect = qobject_cast<TriStateAspect *>(Utils::findOrDefault(aspects(),
                    Utils::equal(&BaseAspect::settingsKey, QString("RunSystemFunction"))));
    QTC_ASSERT(runSystemAspect, return);
    runSystemAspect->setValue(TriState::Disabled);
    runSystemAspect->setVisible(false);
}

bool MerQmakeBuildConfiguration::fromMap(const QVariantMap &map)
{
    if (!QmakeBuildConfiguration::fromMap(map))
        return false;
    disableQmakeSystem();
    return true;
}

QList<NamedWidget *> MerQmakeBuildConfiguration::createSubConfigWidgets()
{
    auto retv = QmakeBuildConfiguration::createSubConfigWidgets();

    auto aspect = this->aspect<MerBuildConfigurationAspect>();
    QTC_ASSERT(aspect, return retv);

    auto widget = qobject_cast<NamedWidget *>(aspect->createConfigWidget());
    QTC_ASSERT(widget, return retv);

    retv += widget;
    return retv;
}

void MerQmakeBuildConfiguration::addToEnvironment(Utils::Environment &env) const
{
    QmakeBuildConfiguration::addToEnvironment(env);

    auto aspect = this->aspect<MerBuildConfigurationAspect>();
    QTC_ASSERT(aspect, return);
    aspect->addToEnvironment(env);
}

MerQmakeBuildConfigurationFactory::MerQmakeBuildConfigurationFactory()
{
    registerBuildConfiguration<MerQmakeBuildConfiguration>(QmakeProjectManager::Constants::QMAKE_BC_ID);
    addSupportedTargetDeviceType(Constants::MER_DEVICE_TYPE);
}

} // namespace Internal
} // namespace Mer
