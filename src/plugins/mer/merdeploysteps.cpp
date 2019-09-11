/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "merdeploysteps.h"

#include "merconstants.h"
#include "merdeployconfiguration.h"
#include "meremulatordevice.h"
#include "merrpmvalidationparser.h"
#include "mersdkkitinformation.h"
#include "mersdkmanager.h"
#include "mersettings.h"
#include "mertargetkitinformation.h"
#include "ui_merrpminfo.h"
#include "ui_merrpmvalidationstepconfigwidget.h"

#include <sfdk/sfdkconstants.h>

#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <remotelinux/abstractremotelinuxdeployservice.h>
#include <ssh/sshremoteprocessrunner.h>
#include <utils/utilsicons.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QClipboard>
#include <QMimeData>
#include <QScrollBar>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace QSsh;
using namespace RemoteLinux;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

namespace {
const int CONNECTION_TEST_CHECK_FOR_CANCEL_INTERVAL = 2000;

const char MER_RPM_VALIDATION_STEP_SELECTED_SUITES[] = "MerRpmValidationStep.SelectedSuites";
}

class MerConnectionTestStepConfigWidget : public BuildStepConfigWidget
{
    Q_OBJECT

public:
    MerConnectionTestStepConfigWidget(MerConnectionTestStep *step)
        : BuildStepConfigWidget(step)
    {
        setSummaryText(QString::fromLatin1("<b>%1:</b> %2")
                .arg(displayName())
                .arg(tr("Verifies connection to the device can be established.")));
    }
};

class MerPrepareTargetStepConfigWidget : public BuildStepConfigWidget
{
    Q_OBJECT

public:
    MerPrepareTargetStepConfigWidget(MerPrepareTargetStep *step)
        : BuildStepConfigWidget(step)
    {
        setSummaryText(QString::fromLatin1("<b>%1:</b> %2")
                .arg(displayName())
                .arg(tr("Prepares target device for deployment")));
    }
};

class MerRpmValidationStepConfigWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT

public:
    MerRpmValidationStepConfigWidget(MerRpmValidationStep *step)
        : BuildStepConfigWidget(step)
    {
        setSummaryText(QString::fromLatin1("<b>%1:</b> %2")
                .arg(displayName())
                .arg(tr("Validates RPM package.")));

        m_ui.setupUi(this);

        bool hasSuite = false;
        bool hasSelectedSuite = false;
        for (const RpmValidationSuiteData &suite : step->suites()) {
            hasSuite = true;
            hasSelectedSuite |= suite.essential;
            auto *item = new QTreeWidgetItem(m_ui.suitesTreeWidget);
            item->setText(0, suite.name);
            QLabel *websiteLabel = new QLabel(QString("<a href='%1'>%1</a>").arg(suite.website));
            websiteLabel->setOpenExternalLinks(true);
            m_ui.suitesTreeWidget->setItemWidget(item, 1, websiteLabel);
            item->setText(2, suite.essential
                    ? tr("Essential", "RPM validation suite")
                    : tr("Optional", "RPM validation suite"));
            item->setCheckState(0, step->selectedSuites().contains(suite.id) ? Qt::Checked : Qt::Unchecked);
            item->setData(0, Qt::UserRole, suite.id);
        }
        m_ui.suitesTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_ui.suitesTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_ui.suitesTreeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

        connect(m_ui.suitesTreeWidget, &QTreeWidget::itemChanged,
                this, &MerRpmValidationStepConfigWidget::onItemChanged);

        m_ui.warningLabelIcon->setPixmap(Utils::Icons::CRITICAL.pixmap());
        updateWarningLabel(hasSuite, hasSelectedSuite);

        updateCommandText();

        m_ui.commandArgumentsLineEdit->setText(step->arguments());
        connect(m_ui.commandArgumentsLineEdit, &QLineEdit::textEdited,
                step, &MerRpmValidationStep::setArguments);
    }

private:
    MerRpmValidationStep *step()
    {
        return static_cast<MerRpmValidationStep *>(BuildStepConfigWidget::step());
    }

    void onItemChanged()
    {
        QStringList checked;
        const int suitesCount = m_ui.suitesTreeWidget->topLevelItemCount();
        for (int i = 0; i < suitesCount; ++i) {
            QTreeWidgetItem *item = m_ui.suitesTreeWidget->topLevelItem(i);
            if (item->checkState(0) == Qt::Checked)
                checked.append(item->data(0, Qt::UserRole).toString());
        }
        step()->setSelectedSuites(checked);
        updateWarningLabel(suitesCount > 0, checked.count() > 0);
        updateCommandText();
    }

    void updateCommandText()
    {
        m_ui.commandLabelEdit->setText(QLatin1String("rpmvalidation ") + step()->fixedArguments());
    }

    void updateWarningLabel(bool hasSuite, bool hasSelectedSuite)
    {
        if (!hasSuite) {
            m_ui.warningLabel->setText(tr("No RPM validation suite is available for the current "
                        "Sailfish OS build target"));
            m_ui.warningLabelIcon->setVisible(true);
            m_ui.warningLabel->setVisible(true);
        } else if (!hasSelectedSuite) {
            m_ui.warningLabel->setText(tr("At least one RPM validation suite must be selected"));
            m_ui.warningLabelIcon->setVisible(true);
            m_ui.warningLabel->setVisible(true);
        } else {
            m_ui.warningLabelIcon->setVisible(false);
            m_ui.warningLabel->setVisible(false);
        }
    }

private:
    Ui::MerRpmValidationStepConfigWidget m_ui;
};

MerProcessStep::MerProcessStep(BuildStepList *bsl, Id id)
    :AbstractProcessStep(bsl,id)
{

}

bool MerProcessStep::init()
{
    return init(NoInitOption);
}

bool MerProcessStep::init(InitOptions options)
{
    QmakeBuildConfiguration *bc = qobject_cast<QmakeBuildConfiguration*>(buildConfiguration());
    if (!bc)
        bc =  qobject_cast<QmakeBuildConfiguration*>(target()->activeBuildConfiguration());

    if (!bc) {
        addOutput(tr("Cannot deploy: No active build configuration."),
                OutputFormat::ErrorMessage);
        return false;
    }

    const BuildEngine *const engine = MerSdkKitInformation::buildEngine(target()->kit());

    if (!engine) {
        addOutput(tr("Cannot deploy: Missing Sailfish OS build-engine information in the kit"),
                OutputFormat::ErrorMessage);
        return false;
    }

    const QString target = MerTargetKitInformation::targetName(this->target()->kit());

    if (target.isEmpty()) {
        addOutput(tr("Cannot deploy: Missing Sailfish OS build-target information in the kit"),
                OutputFormat::ErrorMessage);
        return false;
    }

    IDevice::ConstPtr device = DeviceKitInformation::device(this->target()->kit());

    //TODO: HACK
    if (device.isNull() && !(options & DoNotNeedDevice)) {
        addOutput(tr("Cannot deploy: Missing Sailfish OS device information in the kit"),
                OutputFormat::ErrorMessage);
        return false;
    }

    const QString projectDirectory = bc->isShadowBuild() ? bc->rawBuildDirectory().toString() : project()->projectDirectory().toString();
    const FileName toolsPath = engine->buildTarget(target).toolsPath;
    const QString deployCommand =
        FileName(toolsPath).appendPath(Sfdk::Constants::WRAPPER_DEPLOY).toString();

    ProcessParameters *pp = processParameters();

    Environment env = bc ? bc->environment() : Environment::systemEnvironment();

    //TODO HACK
    if(!device.isNull())
        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_DEVICE_NAME), device->displayName());
    pp->setMacroExpander(bc ? bc->macroExpander() : Utils::globalMacroExpander());
    pp->setEnvironment(env);
    pp->setWorkingDirectory(projectDirectory);
    pp->setCommand(deployCommand);
    pp->setArguments(arguments());
    return AbstractProcessStep::init();
}

QString MerProcessStep::arguments() const
{
    return m_arguments;
}

void MerProcessStep::setArguments(const QString &arguments)
{
    m_arguments = arguments;
}

///////////////////////////////////////////////////////////////////////////////////////////

Core::Id MerEmulatorStartStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerEmulatorStartStep");
}

QString MerEmulatorStartStep::displayName()
{
    return tr("Start Emulator");
}

MerEmulatorStartStep::MerEmulatorStartStep(BuildStepList *bsl)
    : MerAbstractVmStartStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}

bool MerEmulatorStartStep::init()
{
    IDevice::ConstPtr d = DeviceKitInformation::device(this->target()->kit());
    const MerEmulatorDevice* device = dynamic_cast<const MerEmulatorDevice*>(d.data());
    if (!device) {
        setEnabled(false);
        return false;
    }

    setVirtualMachine(device->virtualMachine());

    return MerAbstractVmStartStep::init();
}

///////////////////////////////////////////////////////////////////////////////////////////

Core::Id MerConnectionTestStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerConnectionTestStep");
}

QString MerConnectionTestStep::displayName()
{
    return tr("Test Device Connection");
}

MerConnectionTestStep::MerConnectionTestStep(BuildStepList *bsl)
    : BuildStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
    setWidgetExpandedByDefault(false);
}

bool MerConnectionTestStep::init()
{
    IDevice::ConstPtr d = DeviceKitInformation::device(this->target()->kit());
    if (!d) {
        setEnabled(false);
        return false;
    }

    return true;
}

void MerConnectionTestStep::doRun()
{
    IDevice::ConstPtr d = DeviceKitInformation::device(this->target()->kit());
    if (!d) {
        emit finished(false);
        return;
    }

    m_connection = new SshConnection(d->sshParameters(), this);
    connect(m_connection, &SshConnection::connected,
            this, &MerConnectionTestStep::onConnected);
    connect(m_connection, &SshConnection::errorOccurred,
            this, &MerConnectionTestStep::onConnectionFailure);

    emit addOutput(tr("%1: Testing connection to \"%2\"...")
            .arg(displayName()).arg(d->displayName()), OutputFormat::NormalMessage);

    m_connection->connectToHost();
}

void MerConnectionTestStep::doCancel()
{
    finish(false);
}

BuildStepConfigWidget *MerConnectionTestStep::createConfigWidget()
{
    return new MerConnectionTestStepConfigWidget(this);
}

void MerConnectionTestStep::onConnected()
{
    finish(true);
}

void MerConnectionTestStep::onConnectionFailure()
{
    finish(false);
}

void MerConnectionTestStep::finish(bool result)
{
    m_connection->disconnect(this);
    m_connection->deleteLater(), m_connection = 0;
    emit finished(result);
}

///////////////////////////////////////////////////////////////////////////////////////////

/*!
 * \class MerPrepareTargetStep
 * Wraps either MerEmulatorStartStep or MerConnectionTestStep based on IDevice::machineType
 */

Core::Id MerPrepareTargetStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerPrepareTargetStep");
}

QString MerPrepareTargetStep::displayName()
{
    return tr("Prepare Target");
}

MerPrepareTargetStep::MerPrepareTargetStep(BuildStepList *bsl)
    : BuildStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
    setWidgetExpandedByDefault(false);
}

bool MerPrepareTargetStep::init()
{
    IDevice::ConstPtr d = DeviceKitInformation::device(this->target()->kit());
    if (!d) {
        setEnabled(false);
        return false;
    }

    if (d->machineType() == IDevice::Emulator) {
        m_impl = new MerEmulatorStartStep(qobject_cast<BuildStepList *>(parent()));
    } else {
        m_impl = new MerConnectionTestStep(qobject_cast<BuildStepList *>(parent()));
    }

    if (!m_impl->init()) {
        delete m_impl, m_impl = 0;
        setEnabled(false);
        return false;
    }

    connect(m_impl, &BuildStep::finished,
            this, &MerPrepareTargetStep::finished);
    connect(m_impl, &BuildStep::addTask,
            this, &MerPrepareTargetStep::addTask);
    connect(m_impl, &BuildStep::addOutput,
            this, &MerPrepareTargetStep::addOutput);

    return true;
}

void MerPrepareTargetStep::doRun()
{
    m_impl->run();
}

void MerPrepareTargetStep::doCancel()
{
    m_impl->cancel();
}

BuildStepConfigWidget *MerPrepareTargetStep::createConfigWidget()
{
    return new MerPrepareTargetStepConfigWidget(this);
}

///////////////////////////////////////////////////////////////////////////////////////////

Core::Id MerMb2RsyncDeployStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerRsyncDeployStep");
}

QString MerMb2RsyncDeployStep::displayName()
{
    return QLatin1String("Rsync");
}

MerMb2RsyncDeployStep::MerMb2RsyncDeployStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--rsync"));
}

bool MerMb2RsyncDeployStep::init()
{
    return MerProcessStep::init();
}

void MerMb2RsyncDeployStep::doRun()
{
   emit addOutput(tr("Deploying binaries..."), OutputFormat::NormalMessage);
   AbstractProcessStep::doRun();
}

BuildStepConfigWidget *MerMb2RsyncDeployStep::createConfigWidget()
{
    auto widget = new MerDeployStepWidget(this);
    widget->formatAndSetSummaryText(tr("Deploys with rsync."));
    return widget;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

Core::Id MerLocalRsyncDeployStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerLocalRsyncDeployStep");
}

QString MerLocalRsyncDeployStep::displayName()
{
    return QLatin1String("Local Rsync");
}

MerLocalRsyncDeployStep::MerLocalRsyncDeployStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}

bool MerLocalRsyncDeployStep::init()
{
    QmakeBuildConfiguration *bc = qobject_cast<QmakeBuildConfiguration*>(buildConfiguration());
    if (!bc)
        bc =  qobject_cast<QmakeBuildConfiguration*>(target()->activeBuildConfiguration());

    if (!bc) {
        addOutput(tr("Cannot deploy: No active build configuration."),
                OutputFormat::ErrorMessage);
        return false;
    }

    BuildEngine *const engine = MerSdkKitInformation::buildEngine(target()->kit());

    if (!engine) {
        addOutput(tr("Cannot deploy: Missing Sailfish OS build-engine information in the kit"),
                OutputFormat::ErrorMessage);
        return false;
    }

    const QString target = MerTargetKitInformation::targetName(this->target()->kit());

    if (target.isEmpty()) {
        addOutput(tr("Cannot deploy: Missing Sailfish OS build-target information in the kit"),
                OutputFormat::ErrorMessage);
        return false;
    }

    const QString projectDirectory = bc->isShadowBuild() ? bc->rawBuildDirectory().toString() : project()->projectDirectory().toString();
    const QString deployCommand = QLatin1String("rsync");

    ProcessParameters *pp = processParameters();

    Environment env = bc ? bc->environment() : Environment::systemEnvironment();
    pp->setMacroExpander(bc ? bc->macroExpander() : Utils::globalMacroExpander());
    pp->setEnvironment(env);
    pp->setWorkingDirectory(projectDirectory);
    pp->setCommand(deployCommand);
    pp->setArguments(arguments());

    return AbstractProcessStep::init();
}

void MerLocalRsyncDeployStep::doRun()
{
   emit addOutput(tr("Deploying binaries..."), OutputFormat::NormalMessage);
   AbstractProcessStep::doRun();
}

BuildStepConfigWidget *MerLocalRsyncDeployStep::createConfigWidget()
{
    auto widget = new MerDeployStepWidget(this);
    widget->formatAndSetSummaryText(tr("Deploys with local installed rsync."));
    widget->setCommandText(tr("Deploy using local installed Rsync"));
    return widget;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////


Core::Id MerMb2RpmDeployStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerRpmDeployStep");
}

QString MerMb2RpmDeployStep::displayName()
{
    return QLatin1String("RPM");
}

MerMb2RpmDeployStep::MerMb2RpmDeployStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--sdk"));
}


bool MerMb2RpmDeployStep::init()
{
    return MerProcessStep::init();
}

void MerMb2RpmDeployStep::doRun()
{
    emit addOutput(tr("Deploying RPM package..."), OutputFormat::NormalMessage);
    AbstractProcessStep::doRun();
}

BuildStepConfigWidget *MerMb2RpmDeployStep::createConfigWidget()
{
    auto widget = new MerDeployStepWidget(this);
    widget->formatAndSetSummaryText(tr("Deploys RPM package."));
    return widget;
}

//TODO: HACK
////////////////////////////////////////////////////////////////////////////////////////////////////////////


Core::Id MerMb2RpmBuildStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerRpmBuildStep");
}

QString MerMb2RpmBuildStep::displayName()
{
    return QLatin1String("RPM");
}

MerMb2RpmBuildStep::MerMb2RpmBuildStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}


bool MerMb2RpmBuildStep::init()
{
    bool success = MerProcessStep::init(DoNotNeedDevice);
    m_packages.clear();
    BuildEngine *const engine = MerSdkKitInformation::buildEngine(target()->kit());
    m_sharedHome = QDir::cleanPath(engine->sharedHomePath().toString());
    m_sharedSrc = QDir::cleanPath(engine->sharedSrcPath().toString());

    //hack
    ProcessParameters *pp = processParameters();
    QString deployCommand = pp->command();
    deployCommand.replace(QLatin1String(Sfdk::Constants::WRAPPER_DEPLOY),
            QLatin1String(Sfdk::Constants::WRAPPER_RPM));
    pp->setCommand(deployCommand);
    return success;
}

//TODO: This is hack
void MerMb2RpmBuildStep::doRun()
{
    emit addOutput(tr("Building RPM package..."), OutputFormat::NormalMessage);
    AbstractProcessStep::doRun();
}
//TODO: This is hack
void MerMb2RpmBuildStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    //TODO:
    MerProcessStep::processFinished(exitCode, status);
    if(exitCode == 0 && status == QProcess::NormalExit && !m_packages.isEmpty()){
        new RpmInfo(m_packages, ICore::dialogParent());
    }
}

QStringList MerMb2RpmBuildStep::packagesFilePath() const
{
    return m_packages;
}

BuildStepConfigWidget *MerMb2RpmBuildStep::createConfigWidget()
{
    auto *widget = new MerDeployStepWidget(this);
    widget->formatAndSetSummaryText(tr("Builds RPM package."));
    widget->setCommandText(QLatin1String("mb2 rpm"));
    return widget;
}

void MerMb2RpmBuildStep::stdOutput(const QString &line)
{
    QRegExp rexp(QLatin1String("^Wrote: (/.*RPMS.*\\.rpm)"));
    if (rexp.indexIn(line) != -1) {
        QString file = rexp.cap(1);
        //TODO First replace shared home and then shared src (error prone!)
        file.replace(QRegExp(QString("^") + Constants::MER_SDK_SHARED_HOME_MOUNT_POINT), m_sharedHome);
        file.replace(QRegExp(QString("^") + Constants::MER_SDK_SHARED_SRC_MOUNT_POINT), m_sharedSrc);
        m_packages.append(QDir::toNativeSeparators(file));
    }
    MerProcessStep::stdOutput(line);
}

RpmInfo::RpmInfo(const QStringList& list, QWidget *parent)
    : QDialog(parent)
    , m_ui(new Ui::MerRpmInfo)
    , m_list(list)
{
    setAttribute(Qt::WA_DeleteOnClose);

    m_ui->setupUi(this);
    m_ui->textEdit->setPlainText(m_list.join(QLatin1Char('\n')));

    connect(m_ui->copyButton, &QAbstractButton::clicked, this, &RpmInfo::copyToClipboard);
    connect(m_ui->openButton, &QAbstractButton::clicked, this, &RpmInfo::openContainingFolder);

    QTimer::singleShot(0, m_ui->textEdit, [this]() {
        m_ui->textEdit->horizontalScrollBar()->triggerAction(QAbstractSlider::SliderToMaximum);
    });

    show();
}

RpmInfo::~RpmInfo()
{
    delete m_ui, m_ui = 0;
}

void RpmInfo::copyToClipboard()
{
    QList<QUrl> urls;
    urls.reserve(m_list.size());
    foreach (const QString &path, m_list)
        urls.append(QUrl::fromLocalFile(path));

    QMimeData *mime = new QMimeData;
    mime->setText(m_list.join(QLatin1Char('\n')));
    mime->setUrls(urls);

    QApplication::clipboard()->setMimeData(mime);
}

void RpmInfo::openContainingFolder()
{
    QTC_ASSERT(!m_list.isEmpty(), return);
    Core::FileUtils::showInGraphicalShell(ICore::mainWindow(), m_list.first());
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////

Core::Id MerRpmValidationStep::stepId()
{
    return Core::Id("QmakeProjectManager.MerRpmValidationStep");
}

QString MerRpmValidationStep::displayName()
{
    return QLatin1String("RPM Validation");
}

MerRpmValidationStep::MerRpmValidationStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setEnabled(MerSettings::rpmValidationByDefault());
    setDefaultDisplayName(displayName());

    m_target = MerTargetKitInformation::target(target()->kit());
    m_selectedSuites = defaultSuites();
}

bool MerRpmValidationStep::init()
{
    if (!MerProcessStep::init(DoNotNeedDevice)) {
        return false;
    }

    m_packagingStep = earlierBuildStep<MerMb2RpmBuildStep>(deployConfiguration(), this);
    if (!m_packagingStep) {
        emit addOutput(tr("Cannot validate: No previous \"%1\" step found")
                .arg(MerMb2RpmBuildStep::displayName()),
                OutputFormat::ErrorMessage);
        return false;
    }

    setOutputParser(new MerRpmValidationParser);

    return true;
}

void MerRpmValidationStep::doRun()
{
    if (m_target.rpmValidationSuites.isEmpty()) {
        const QString message(tr("No RPM validation suite is available for the current "
                    "Sailfish OS build target, the package will not be validated"));
        emit addOutput(message, OutputFormat::ErrorMessage);
        emit addTask(Task(Task::Error, message, Utils::FileName(), -1,
                    ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM),
                2);
        emit addOutput("  " + tr("Disable the RPM Validation deploy step to avoid this error"),
                OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    } else if (m_selectedSuites.isEmpty()) {
        const QString message(tr("No RPM validation suite is selected in deployment settings,"
                    " the package will not be validated"));
        emit addOutput(message, OutputFormat::ErrorMessage);
        emit addTask(Task(Task::Error, message, Utils::FileName(), -1,
                    ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM),
                2);
        emit addOutput("  " + tr("Either select at least one suite or disable the RPM Validation "
                    "deploy step to avoid this error"), OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    emit addOutput(tr("Validating RPM package..."), OutputFormat::NormalMessage);

    const QString packageFile = m_packagingStep->packagesFilePath().first();
    if(!packageFile.endsWith(QLatin1String(".rpm"))){
        const QString message((tr("No package to validate found in %1")).arg(packageFile));
        emit addTask(Task(Task::Error, message, Utils::FileName(), -1,
                    ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM),
                1);
        emit addOutput(message, OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    // hack
    ProcessParameters *pp = processParameters();
    QString deployCommand = pp->command();
    deployCommand.replace(QLatin1String(Sfdk::Constants::WRAPPER_DEPLOY),
            QLatin1String(Sfdk::Constants::WRAPPER_RPMVALIDATION));
    pp->setCommand(deployCommand);
    QStringList arguments{ m_fixedArguments, this->arguments(), packageFile };
    pp->setArguments(arguments.join(' '));

    AbstractProcessStep::doRun();
}

bool MerRpmValidationStep::fromMap(const QVariantMap &map)
{
    if (!MerProcessStep::fromMap(map))
        return false;

    QStringList selectedSuites = map.value(MER_RPM_VALIDATION_STEP_SELECTED_SUITES).toStringList();
    if (!selectedSuites.isEmpty())
        setSelectedSuites(selectedSuites);

    return true;
}

QVariantMap MerRpmValidationStep::toMap() const
{
    QVariantMap map = MerProcessStep::toMap();
    if (m_selectedSuites != defaultSuites())
        map.insert(MER_RPM_VALIDATION_STEP_SELECTED_SUITES, m_selectedSuites);
    return map;
}

QList<Sfdk::RpmValidationSuiteData> MerRpmValidationStep::suites() const
{
    return m_target.rpmValidationSuites;
}

QStringList MerRpmValidationStep::defaultSuites() const
{
    QStringList defaultSuites;
    for (const RpmValidationSuiteData &suite : m_target.rpmValidationSuites) {
        if (suite.essential)
            defaultSuites.append(suite.id);
    }
    return defaultSuites;
}

QStringList MerRpmValidationStep::selectedSuites() const
{
    return m_selectedSuites;
}

void MerRpmValidationStep::setSelectedSuites(const QStringList &selectedSuites)
{
    m_selectedSuites = selectedSuites;
    if (!m_selectedSuites.isEmpty()) {
        if (m_selectedSuites != defaultSuites())
            m_fixedArguments = "--suites " + m_selectedSuites.join(',');
        else
            m_fixedArguments.clear();
    } else {
        m_fixedArguments.clear();
    }
}

QString MerRpmValidationStep::fixedArguments() const
{
    return m_fixedArguments;
}

BuildStepConfigWidget *MerRpmValidationStep::createConfigWidget()
{
    auto *widget = new MerRpmValidationStepConfigWidget(this);
    return widget;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////


MerDeployStepWidget::MerDeployStepWidget(MerProcessStep *step)
        : BuildStepConfigWidget(step)
{
    m_ui.setupUi(this);
    m_ui.commandArgumentsLineEdit->setText(step->arguments());
    connect(m_ui.commandArgumentsLineEdit, &QLineEdit::textEdited,
            this, &MerDeployStepWidget::commandArgumentsLineEditTextEdited);
}

void MerDeployStepWidget::commandArgumentsLineEditTextEdited()
{
    static_cast<MerProcessStep *>(step())->setArguments(m_ui.commandArgumentsLineEdit->text());
}

void MerDeployStepWidget::setCommandText(const QString& commandText)
{
     m_ui.commandLabelEdit->setText(commandText);
}

QString MerDeployStepWidget::commnadText() const
{
   return  m_ui.commandLabelEdit->text();
}

void MerDeployStepWidget::formatAndSetSummaryText(const QString &summaryText)
{
    setSummaryText(QString::fromLatin1("<b>%1:</b> %2").arg(displayName()).arg(summaryText));
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

class MerNamedCommandDeployService : public AbstractRemoteLinuxDeployService
{
    Q_OBJECT
    enum State { Inactive, Running };

public:
    explicit MerNamedCommandDeployService(QObject *parent = 0);

    QString description() const { return m_description; }
    QString command() const { return m_command; }
    void setDescription(const QString &description) { m_description = description; }
    void setCommand(const QString &command) { m_command = command; }

    bool isDeploymentNecessary() const override { return true; }

protected:
    void doDeviceSetup() override { handleDeviceSetupDone(true); }
    void stopDeviceSetup() override { handleDeviceSetupDone(false); }
    void doDeploy() override;
    void stopDeployment() override;

private slots:
    void handleStdout();
    void handleStderr();
    void handleProcessClosed(const QString &error);

private:
    State m_state{Inactive};
    QString m_description;
    QString m_command;
    SshRemoteProcessRunner *m_runner{};
};

MerNamedCommandDeployService::MerNamedCommandDeployService(QObject *parent)
    : AbstractRemoteLinuxDeployService(parent)
{
}

void MerNamedCommandDeployService::doDeploy()
{
    QTC_CHECK(!m_description.isEmpty());
    QTC_CHECK(!m_command.isEmpty());
    QTC_ASSERT(m_state == Inactive, handleDeploymentDone());

    if (!m_runner)
        m_runner = new SshRemoteProcessRunner(this);
    connect(m_runner, &SshRemoteProcessRunner::readyReadStandardOutput, this, &MerNamedCommandDeployService::handleStdout);
    connect(m_runner, &SshRemoteProcessRunner::readyReadStandardError, this, &MerNamedCommandDeployService::handleStderr);
    connect(m_runner, &SshRemoteProcessRunner::processClosed, this, &MerNamedCommandDeployService::handleProcessClosed);

    emit progressMessage(m_description);
    m_state = Running;
    m_runner->run(m_command.toUtf8(), deviceConfiguration()->sshParameters());
}

void MerNamedCommandDeployService::stopDeployment()
{
    QTC_ASSERT(m_state == Running, return);

    disconnect(m_runner, 0, this, 0);
    m_runner->cancel();
    m_state = Inactive;
    handleDeploymentDone();
}

void MerNamedCommandDeployService::handleStdout()
{
    emit stdOutData(QString::fromUtf8(m_runner->readAllStandardOutput()));
}

void MerNamedCommandDeployService::handleStderr()
{
    emit stdErrData(QString::fromUtf8(m_runner->readAllStandardError()));
}

void MerNamedCommandDeployService::handleProcessClosed(const QString &error)
{
    QTC_ASSERT(m_state == Running, return);

    if (!error.isEmpty()) {
        emit errorMessage(tr("Remote process failed to start: %1").arg(error));
    } else if (m_runner->processExitStatus() != SshRemoteProcess::NormalExit) {
        emit errorMessage(tr("Remote process finished abnormally."));
    } else if (m_runner->processExitCode() != 0) {
        emit errorMessage(tr("Remote process finished with exit code %1.")
            .arg(m_runner->processExitCode()));
    } else {
        emit progressMessage(tr("Remote command finished successfully."));
    }

    stopDeployment();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

MerNamedCommandDeployStep::MerNamedCommandDeployStep(ProjectExplorer::BuildStepList *bsl, Core::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
    , m_deployService(new MerNamedCommandDeployService(this))
{
}

AbstractRemoteLinuxDeployService *MerNamedCommandDeployStep::deployService() const
{
    return m_deployService;
}

bool MerNamedCommandDeployStep::initInternal(QString *error)
{
    return m_deployService->isDeploymentPossible(error);
}

void MerNamedCommandDeployStep::setCommand(const QString &description, const QString &command)
{
    m_deployService->setDescription(description);
    m_deployService->setCommand(command);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////

MerResetAmbienceDeployStep::MerResetAmbienceDeployStep(ProjectExplorer::BuildStepList *bsl)
    : MerNamedCommandDeployStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
    QString ambienceName = target()->project()->displayName();

    QFile scriptFile(QStringLiteral(":/mer/reset-ambience.sh"));
    bool ok = scriptFile.open(QIODevice::ReadOnly);
    QTC_CHECK(ok);
    QString script = QString::fromUtf8(scriptFile.readAll());
    script.prepend(QStringLiteral("AMBIENCE_URL='file:///usr/share/ambience/%1/%1.ambience'\n").arg(ambienceName));

    setCommand(tr("Starting remote command to reset ambience '%1'...").arg(ambienceName), script);
}

Core::Id MerResetAmbienceDeployStep::stepId()
{
    return Core::Id("Mer.MerResetAmbienceDeployStep");
}

QString MerResetAmbienceDeployStep::displayName()
{
    return tr("Reset Sailfish OS Ambience");
}

} // Internal
} // Mer

#include "merdeploysteps.moc"
