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

#include "genericmakestep.h"
#include "genericprojectconstants.h"
#include "genericproject.h"
#include "ui_genericmakestep.h"
#include "genericbuildconfiguration.h"

#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>

#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>

using namespace Core;
using namespace ProjectExplorer;

namespace GenericProjectManager {
namespace Internal {

const char GENERIC_MS_ID[] = "GenericProjectManager.GenericMakeStep";
const char GENERIC_MS_DISPLAY_NAME[] = QT_TRANSLATE_NOOP("GenericProjectManager::Internal::GenericMakeStep",
                                                     "Make");

const char BUILD_TARGETS_KEY[] = "GenericProjectManager.GenericMakeStep.BuildTargets";
const char MAKE_ARGUMENTS_KEY[] = "GenericProjectManager.GenericMakeStep.MakeArguments";
const char MAKE_COMMAND_KEY[] = "GenericProjectManager.GenericMakeStep.MakeCommand";
const char CLEAN_KEY[] = "GenericProjectManager.GenericMakeStep.Clean";

GenericMakeStep::GenericMakeStep(BuildStepList *parent, const QString &buildTarget)
    : AbstractProcessStep(parent, GENERIC_MS_ID)
{
    setDefaultDisplayName(QCoreApplication::translate("GenericProjectManager::Internal::GenericMakeStep",
                                                      GENERIC_MS_DISPLAY_NAME));
    setBuildTarget(buildTarget, true);
}

bool GenericMakeStep::init(QList<const BuildStep *> &earlierSteps)
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();
    if (!bc)
        emit addTask(Task::buildConfigurationMissingTask());

    ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
    if (!tc)
        emit addTask(Task::compilerMissingTask());

    if (!bc || !tc) {
        emitFaultyConfigurationMessage();
        return false;
    }

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc->macroExpander());
    pp->setWorkingDirectory(bc->buildDirectory().toString());
    Utils::Environment env = bc->environment();
    Utils::Environment::setupEnglishOutput(&env);
    pp->setEnvironment(env);
    pp->setCommand(makeCommand(bc->environment()));
    pp->setArguments(allArguments());
    pp->resolveAll();

    // If we are cleaning, then make can fail with an error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on an already clean project
    setIgnoreReturnValue(m_clean);

    setOutputParser(new GnuMakeParser());
    IOutputParser *parser = target()->kit()->createOutputParser();
    if (parser)
        appendOutputParser(parser);
    outputParser()->setWorkingDirectory(pp->effectiveWorkingDirectory());

    return AbstractProcessStep::init(earlierSteps);
}

void GenericMakeStep::setClean(bool clean)
{
    m_clean = clean;
}

bool GenericMakeStep::isClean() const
{
    return m_clean;
}

QVariantMap GenericMakeStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(BUILD_TARGETS_KEY, m_buildTargets);
    map.insert(MAKE_ARGUMENTS_KEY, m_makeArguments);
    map.insert(MAKE_COMMAND_KEY, m_makeCommand);
    map.insert(CLEAN_KEY, m_clean);
    return map;
}

bool GenericMakeStep::fromMap(const QVariantMap &map)
{
    m_buildTargets = map.value(BUILD_TARGETS_KEY).toStringList();
    m_makeArguments = map.value(MAKE_ARGUMENTS_KEY).toString();
    m_makeCommand = map.value(MAKE_COMMAND_KEY).toString();
    m_clean = map.value(CLEAN_KEY).toBool();

    return BuildStep::fromMap(map);
}

QString GenericMakeStep::allArguments() const
{
    QString args = m_makeArguments;
    Utils::QtcProcess::addArgs(&args, m_buildTargets);
    return args;
}

QString GenericMakeStep::makeCommand(const Utils::Environment &environment) const
{
    QString command = m_makeCommand;
    if (command.isEmpty()) {
        ToolChain *tc = ToolChainKitInformation::toolChain(target()->kit(), ProjectExplorer::Constants::CXX_LANGUAGE_ID);
        if (tc)
            command = tc->makeCommand(environment);
        else
            command = "make";
    }
    return command;
}

void GenericMakeStep::run(QFutureInterface<bool> &fi)
{
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *GenericMakeStep::createConfigWidget()
{
    return new GenericMakeStepConfigWidget(this);
}

bool GenericMakeStep::immutable() const
{
    return false;
}

bool GenericMakeStep::buildsTarget(const QString &target) const
{
    return m_buildTargets.contains(target);
}

void GenericMakeStep::setBuildTarget(const QString &target, bool on)
{
    QStringList old = m_buildTargets;
    if (on && !old.contains(target))
         old << target;
    else if (!on && old.contains(target))
        old.removeOne(target);

    m_buildTargets = old;
}

//
// GenericMakeStepConfigWidget
//

GenericMakeStepConfigWidget::GenericMakeStepConfigWidget(GenericMakeStep *makeStep) :
    m_makeStep(makeStep)
{
    m_ui = new Ui::GenericMakeStep;
    m_ui->setupUi(this);

    const auto pro = static_cast<GenericProject *>(m_makeStep->target()->project());
    const auto buildTargets = pro->buildTargets();
    for (const QString &target : buildTargets) {
        auto item = new QListWidgetItem(target, m_ui->targetsList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(m_makeStep->buildsTarget(item->text()) ? Qt::Checked : Qt::Unchecked);
    }

    m_ui->makeLineEdit->setText(m_makeStep->m_makeCommand);
    m_ui->makeArgumentsLineEdit->setText(m_makeStep->m_makeArguments);
    updateMakeOverrideLabel();
    updateDetails();

    connect(m_ui->targetsList, &QListWidget::itemChanged,
            this, &GenericMakeStepConfigWidget::itemChanged);
    connect(m_ui->makeLineEdit, &QLineEdit::textEdited,
            this, &GenericMakeStepConfigWidget::makeLineEditTextEdited);
    connect(m_ui->makeArgumentsLineEdit, &QLineEdit::textEdited,
            this, &GenericMakeStepConfigWidget::makeArgumentsLineEditTextEdited);

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &GenericMakeStepConfigWidget::updateMakeOverrideLabel);
    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, &GenericMakeStepConfigWidget::updateDetails);

    connect(m_makeStep->target(), &Target::kitChanged,
            this, &GenericMakeStepConfigWidget::updateMakeOverrideLabel);

    pro->subscribeSignal(&BuildConfiguration::environmentChanged, this, [this]() {
        if (static_cast<BuildConfiguration *>(sender())->isActive()) {
            updateMakeOverrideLabel();
            updateDetails();
        }
    });
    connect(pro, &Project::activeProjectConfigurationChanged,
            this, [this](ProjectConfiguration *pc) {
        if (pc && pc->isActive()) {
            updateMakeOverrideLabel();
            updateDetails();
        }
    });
}

GenericMakeStepConfigWidget::~GenericMakeStepConfigWidget()
{
    delete m_ui;
}

QString GenericMakeStepConfigWidget::displayName() const
{
    return tr("Make", "GenericMakestep display name.");
}

void GenericMakeStepConfigWidget::updateMakeOverrideLabel()
{
    BuildConfiguration *bc = m_makeStep->buildConfiguration();
    if (!bc)
        bc = m_makeStep->target()->activeBuildConfiguration();

    m_ui->makeLabel->setText(tr("Override %1:").arg(QDir::toNativeSeparators(m_makeStep->makeCommand(bc->environment()))));
}

void GenericMakeStepConfigWidget::updateDetails()
{
    BuildConfiguration *bc = m_makeStep->buildConfiguration();
    if (!bc)
        bc = m_makeStep->target()->activeBuildConfiguration();

    ProcessParameters param;
    param.setMacroExpander(bc->macroExpander());
    param.setWorkingDirectory(bc->buildDirectory().toString());
    param.setEnvironment(bc->environment());
    param.setCommand(m_makeStep->makeCommand(bc->environment()));
    param.setArguments(m_makeStep->allArguments());
    m_summaryText = param.summary(displayName());
    emit updateSummary();
}

QString GenericMakeStepConfigWidget::summaryText() const
{
    return m_summaryText;
}

void GenericMakeStepConfigWidget::itemChanged(QListWidgetItem *item)
{
    m_makeStep->setBuildTarget(item->text(), item->checkState() & Qt::Checked);
    updateDetails();
}

void GenericMakeStepConfigWidget::makeLineEditTextEdited()
{
    m_makeStep->m_makeCommand = m_ui->makeLineEdit->text();
    updateDetails();
}

void GenericMakeStepConfigWidget::makeArgumentsLineEditTextEdited()
{
    m_makeStep->m_makeArguments = m_ui->makeArgumentsLineEdit->text();
    updateDetails();
}

//
// GenericMakeAllStepFactory
//

GenericMakeAllStepFactory::GenericMakeAllStepFactory()
{
    struct Step : GenericMakeStep
    {
        Step(BuildStepList *bsl) : GenericMakeStep(bsl, QString("all")) { }
    };

    registerStep<Step>(GENERIC_MS_ID);
    setDisplayName(QCoreApplication::translate(
        "GenericProjectManager::Internal::GenericMakeStep", GENERIC_MS_DISPLAY_NAME));
    setSupportedProjectType(Constants::GENERICPROJECT_ID);
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_BUILD,
                           ProjectExplorer::Constants::BUILDSTEPS_DEPLOY});
}

//
// GenericMakeCleanStepFactory
//

GenericMakeCleanStepFactory::GenericMakeCleanStepFactory()
{
    struct Step : GenericMakeStep
    {
        Step(BuildStepList *bsl) : GenericMakeStep(bsl)
        {
            setBuildTarget("clean", true);
            setClean(true);
        }
    };

    registerStep<Step>(GENERIC_MS_ID);
    setDisplayName(QCoreApplication::translate(
        "GenericProjectManager::Internal::GenericMakeStep", GENERIC_MS_DISPLAY_NAME));
    setSupportedProjectType(Constants::GENERICPROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
}

} // namespace Internal
} // namespace GenericProjectManager
