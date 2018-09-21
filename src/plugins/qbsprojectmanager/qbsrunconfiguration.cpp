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

#include "qbsrunconfiguration.h"

#include "qbsprojectmanagerconstants.h"
#include "qbsdeployconfigurationfactory.h"
#include "qbsinstallstep.h"
#include "qbsproject.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/variablechooser.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/deployconfiguration.h>
#include <projectexplorer/localenvironmentaspect.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <projectexplorer/target.h>
#include <projectexplorer/runconfigurationaspects.h>
#include <utils/algorithm.h>
#include <utils/qtcprocess.h>
#include <utils/pathchooser.h>
#include <utils/detailswidget.h>
#include <utils/stringutils.h>
#include <utils/persistentsettings.h>
#include <utils/utilsicons.h>
#include <qtsupport/qtoutputformatter.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/hostosinfo.h>

#include "api/runenvironment.h"

#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include <QToolButton>
#include <QComboBox>
#include <QDir>

using namespace ProjectExplorer;
using namespace Utils;

namespace QbsProjectManager {
namespace Internal {

const char QBS_RC_PREFIX[] = "Qbs.RunConfiguration:";

static QString rcNameSeparator() { return QLatin1String("---Qbs.RC.NameSeparator---"); }

static QString usingLibraryPathsKey() { return QString("Qbs.RunConfiguration.UsingLibraryPaths"); }

const qbs::ProductData findProduct(const qbs::ProjectData &pro, const QString &uniqeName)
{
    foreach (const qbs::ProductData &product, pro.allProducts()) {
        if (QbsProject::uniqueProductName(product) == uniqeName)
            return product;
    }
    return qbs::ProductData();
}

// --------------------------------------------------------------------
// QbsRunConfiguration:
// --------------------------------------------------------------------

QbsRunConfiguration::QbsRunConfiguration(Target *target)
    : RunConfiguration(target, QBS_RC_PREFIX)
{
    auto envAspect = new LocalEnvironmentAspect(this,
            [](RunConfiguration *rc, Environment &env) {
                static_cast<QbsRunConfiguration *>(rc)->addToBaseEnvironment(env);
            });
    addExtraAspect(envAspect);
    connect(static_cast<QbsProject *>(target->project()), &Project::parsingFinished, this,
            [envAspect]() { envAspect->buildEnvironmentHasChanged(); });
    addExtraAspect(new ArgumentsAspect(this, "Qbs.RunConfiguration.CommandLineArguments"));
    addExtraAspect(new WorkingDirectoryAspect(this, "Qbs.RunConfiguration.WorkingDirectory"));

    addExtraAspect(new TerminalAspect(this, "Qbs.RunConfiguration.UseTerminal", isConsoleApplication()));

    QbsProject *project = static_cast<QbsProject *>(target->project());
    connect(project, &Project::parsingFinished, this, [this](bool success) {
        auto terminalAspect = extraAspect<TerminalAspect>();
        if (success && !terminalAspect->isUserSet())
            terminalAspect->setUseTerminal(isConsoleApplication());
    });
    connect(project, &QbsProject::dataChanged, this, [this] { m_envCache.clear(); });
    connect(BuildManager::instance(), &BuildManager::buildStateChanged, this,
            [this, project](Project *p) {
                if (p == project && !BuildManager::isBuilding(p)) {
                    const QString defaultWorkingDir = baseWorkingDirectory();
                    if (!defaultWorkingDir.isEmpty()) {
                        extraAspect<WorkingDirectoryAspect>()->setDefaultWorkingDirectory(
                                    Utils::FileName::fromString(defaultWorkingDir));
                    }
                    emit enabledChanged();
                }
            }
    );

    connect(target, &Target::activeDeployConfigurationChanged,
            this, &QbsRunConfiguration::installStepChanged);
}

QVariantMap QbsRunConfiguration::toMap() const
{
    QVariantMap map = RunConfiguration::toMap();
    map.insert(usingLibraryPathsKey(), usingLibraryPaths());
    return map;
}

bool QbsRunConfiguration::fromMap(const QVariantMap &map)
{
    if (!RunConfiguration::fromMap(map))
        return false;

    QString extraId = ProjectExplorer::idFromMap(map).suffixAfter(id());
    if (!extraId.isEmpty()) {
        const int sepPos = extraId.indexOf(rcNameSeparator());
        m_uniqueProductName = extraId.left(sepPos);
        m_productDisplayName = sepPos == -1 ? QString() : extraId.mid(sepPos + rcNameSeparator().size());
    }

    setDefaultDisplayName(defaultDisplayName());
    m_usingLibraryPaths = map.value(usingLibraryPathsKey(), true).toBool();
    installStepChanged();

    return true;
}

QString QbsRunConfiguration::extraId() const
{
    return m_uniqueProductName + rcNameSeparator() + m_productDisplayName;
}


QWidget *QbsRunConfiguration::createConfigurationWidget()
{
    return new QbsRunConfigurationWidget(this);
}

void QbsRunConfiguration::installStepChanged()
{
    if (m_currentInstallStep) {
        disconnect(m_currentInstallStep, &QbsInstallStep::changed,
                   this, &QbsRunConfiguration::targetInformationChanged);
    }
    if (m_currentBuildStepList) {
        disconnect(m_currentBuildStepList, &BuildStepList::stepInserted,
                   this, &QbsRunConfiguration::installStepChanged);
        disconnect(m_currentBuildStepList, &BuildStepList::stepRemoved,
                   this, &QbsRunConfiguration::installStepChanged);
        disconnect(m_currentBuildStepList, &BuildStepList::stepMoved,
                   this, &QbsRunConfiguration::installStepChanged);
    }

    QbsDeployConfiguration *activeDc = qobject_cast<QbsDeployConfiguration *>(target()->activeDeployConfiguration());
    m_currentBuildStepList = activeDc ? activeDc->stepList() : 0;
    if (m_currentInstallStep) {
        connect(m_currentInstallStep, &QbsInstallStep::changed,
                this, &QbsRunConfiguration::targetInformationChanged);
    }
    if (m_currentBuildStepList) {
        connect(m_currentBuildStepList, &BuildStepList::stepInserted,
                this, &QbsRunConfiguration::installStepChanged);
        connect(m_currentBuildStepList, &BuildStepList::aboutToRemoveStep, this,
                &QbsRunConfiguration::installStepToBeRemoved);
        connect(m_currentBuildStepList, &BuildStepList::stepRemoved,
                this, &QbsRunConfiguration::installStepChanged);
        connect(m_currentBuildStepList, &BuildStepList::stepMoved,
                this, &QbsRunConfiguration::installStepChanged);
    }

    emit targetInformationChanged();
}

void QbsRunConfiguration::installStepToBeRemoved(int pos)
{
    QTC_ASSERT(m_currentBuildStepList, return);
    // TODO: Our logic is rather broken. Users can create as many qbs install steps as they want,
    // but we ignore all but the first one.
    if (m_currentBuildStepList->steps().at(pos) != m_currentInstallStep)
        return;
    disconnect(m_currentInstallStep, &QbsInstallStep::changed,
               this, &QbsRunConfiguration::targetInformationChanged);
    m_currentInstallStep = 0;
}

Runnable QbsRunConfiguration::runnable() const
{
    StandardRunnable r;
    r.executable = executable();
    r.workingDirectory = extraAspect<WorkingDirectoryAspect>()->workingDirectory().toString();
    r.commandLineArguments = extraAspect<ArgumentsAspect>()->arguments();
    r.runMode = extraAspect<TerminalAspect>()->runMode();
    r.environment = extraAspect<LocalEnvironmentAspect>()->environment();
    return r;
}

QString QbsRunConfiguration::executable() const
{
    QbsProject *pro = static_cast<QbsProject *>(target()->project());
    const qbs::ProductData product = findProduct(pro->qbsProjectData(), uniqueProductName());

    if (!product.isValid() || !pro->qbsProject().isValid())
        return QString();

    return product.targetExecutable();
}

bool QbsRunConfiguration::isConsoleApplication() const
{
    QbsProject *pro = static_cast<QbsProject *>(target()->project());
    const qbs::ProductData product = findProduct(pro->qbsProjectData(), uniqueProductName());
    return product.properties().value(QLatin1String("consoleApplication"), false).toBool();
}

void QbsRunConfiguration::setUsingLibraryPaths(bool useLibPaths)
{
    m_usingLibraryPaths = useLibPaths;
    extraAspect<LocalEnvironmentAspect>()->environmentChanged();
}

QString QbsRunConfiguration::baseWorkingDirectory() const
{
    const QString exe = executable();
    if (!exe.isEmpty())
        return QFileInfo(exe).absolutePath();
    return QString();
}

void QbsRunConfiguration::addToBaseEnvironment(Utils::Environment &env) const
{
    const auto key = qMakePair(env.toStringList(), m_usingLibraryPaths);
    const auto it = m_envCache.constFind(key);
    if (it != m_envCache.constEnd()) {
        env = it.value();
        return;
    }
    QbsProject *project = static_cast<QbsProject *>(target()->project());
    if (project && project->qbsProject().isValid()) {
        const qbs::ProductData product = findProduct(project->qbsProjectData(), uniqueProductName());
        if (product.isValid()) {
            QProcessEnvironment procEnv = env.toProcessEnvironment();
            procEnv.insert(QLatin1String("QBS_RUN_FILE_PATH"), executable());
            QStringList setupRunEnvConfig;
            if (!m_usingLibraryPaths)
                setupRunEnvConfig << QLatin1String("ignore-lib-dependencies");
            qbs::RunEnvironment qbsRunEnv = project->qbsProject().getRunEnvironment(product,
                    qbs::InstallOptions(), procEnv, setupRunEnvConfig, QbsManager::settings());
            qbs::ErrorInfo error;
            procEnv = qbsRunEnv.runEnvironment(&error);
            if (error.hasError()) {
                Core::MessageManager::write(tr("Error retrieving run environment: %1")
                                            .arg(error.toString()));
            }
            if (!procEnv.isEmpty()) {
                env = Utils::Environment();
                foreach (const QString &key, procEnv.keys())
                    env.set(key, procEnv.value(key));
            }
        }
    }
    m_envCache.insert(key, env);
}

QString QbsRunConfiguration::buildSystemTarget() const
{
    return m_productDisplayName;
}

QString QbsRunConfiguration::uniqueProductName() const
{
    return m_uniqueProductName;
}

QString QbsRunConfiguration::defaultDisplayName()
{
    QString defaultName = m_productDisplayName;
    if (defaultName.isEmpty())
        defaultName = tr("Qbs Run Configuration");
    return defaultName;
}

Utils::OutputFormatter *QbsRunConfiguration::createOutputFormatter() const
{
    return new QtSupport::QtOutputFormatter(target()->project());
}

// --------------------------------------------------------------------
// QbsRunConfigurationWidget:
// --------------------------------------------------------------------

QbsRunConfigurationWidget::QbsRunConfigurationWidget(QbsRunConfiguration *rc)
    : m_rc(rc)
{
    auto vboxTopLayout = new QVBoxLayout(this);
    vboxTopLayout->setMargin(0);

    auto detailsContainer = new Utils::DetailsWidget(this);
    detailsContainer->setState(Utils::DetailsWidget::NoSummary);
    vboxTopLayout->addWidget(detailsContainer);
    auto detailsWidget = new QWidget(detailsContainer);
    detailsContainer->setWidget(detailsWidget);
    auto toplayout = new QFormLayout(detailsWidget);
    toplayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    toplayout->setMargin(0);

    m_executableLineLabel = new QLabel(this);
    m_executableLineLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
    setExecutableLineText();
    toplayout->addRow(tr("Executable:"), m_executableLineLabel);

    m_rc->extraAspect<ArgumentsAspect>()->addToMainConfigurationWidget(this, toplayout);
    m_rc->extraAspect<WorkingDirectoryAspect>()->addToMainConfigurationWidget(this, toplayout);

    m_rc->extraAspect<TerminalAspect>()->addToMainConfigurationWidget(this, toplayout);
    m_usingLibPathsCheckBox = new QCheckBox(tr("Add library paths to run environment"));
    m_usingLibPathsCheckBox->setChecked(m_rc->usingLibraryPaths());
    connect(m_usingLibPathsCheckBox, &QCheckBox::toggled, m_rc,
            &QbsRunConfiguration::setUsingLibraryPaths);
    toplayout->addRow(QString(), m_usingLibPathsCheckBox);

    connect(m_rc, &QbsRunConfiguration::targetInformationChanged,
            this, &QbsRunConfigurationWidget::targetInformationHasChanged, Qt::QueuedConnection);

    connect(m_rc, &RunConfiguration::enabledChanged,
            this, &QbsRunConfigurationWidget::targetInformationHasChanged);
    targetInformationHasChanged();

    Core::VariableChooser::addSupportForChildWidgets(this, rc->macroExpander());
}

void QbsRunConfigurationWidget::targetInformationHasChanged()
{
    m_ignoreChange = true;
    setExecutableLineText(m_rc->executable());

    WorkingDirectoryAspect *aspect = m_rc->extraAspect<WorkingDirectoryAspect>();
    aspect->pathChooser()->setBaseFileName(m_rc->target()->project()->projectDirectory());
    m_ignoreChange = false;
}

void QbsRunConfigurationWidget::setExecutableLineText(const QString &text)
{
    const QString newText = text.isEmpty() ? tr("<unknown>") : text;
    m_executableLineLabel->setText(newText);
}

// --------------------------------------------------------------------
// QbsRunConfigurationFactory:
// --------------------------------------------------------------------

QbsRunConfigurationFactory::QbsRunConfigurationFactory(QObject *parent) :
    IRunConfigurationFactory(parent)
{
    setObjectName("QbsRunConfigurationFactory");
    registerRunConfiguration<QbsRunConfiguration>(QBS_RC_PREFIX);
    addSupportedProjectType(Constants::PROJECT_ID);
    setSupportedTargetDeviceTypes({ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE});
}

bool QbsRunConfigurationFactory::canCreateHelper(Target *parent, const QString &buildTarget) const
{
    QbsProject *project = static_cast<QbsProject *>(parent->project());
    QString product = buildTarget.left(buildTarget.indexOf(rcNameSeparator()));
    return findProduct(project->qbsProjectData(), product).isValid();
}

QList<BuildTargetInfo> QbsRunConfigurationFactory::availableBuildTargets(Target *parent, CreationMode mode) const
{
    QList<qbs::ProductData> products;

    QbsProject *project = qobject_cast<QbsProject *>(parent->project());
    if (!project || !project->qbsProject().isValid())
        return {};

    foreach (const qbs::ProductData &product, project->qbsProjectData().allProducts()) {
        if (product.isRunnable() && product.isEnabled())
            products << product;
    }

    if (mode == AutoCreate) {
        std::function<bool (const qbs::ProductData &)> hasQtcRunnable = [](const qbs::ProductData &product) {
            return product.properties().value("qtcRunnable").toBool();
        };

        if (Utils::anyOf(products, hasQtcRunnable))
            Utils::erase(products, std::not1(hasQtcRunnable));
    }

    return Utils::transform(products, [project](const qbs::ProductData &product) {
        QString displayName = product.fullDisplayName();
        BuildTargetInfo bti;
        bti.targetName = QbsProject::uniqueProductName(product) + rcNameSeparator() + displayName;
        bti.displayName = displayName;
        return bti;
    });
}

} // namespace Internal
} // namespace QbsProjectManager
