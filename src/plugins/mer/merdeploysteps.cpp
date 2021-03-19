/****************************************************************************
**
** Copyright (C) 2012-2019,2021 Jolla Ltd.
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

#include "merbuildconfigurationaspect.h"
#include "merconstants.h"
#include "mercmakebuildconfiguration.h"
#include "mercompilationdatabasebuildconfiguration.h"
#include "merdeployconfiguration.h"
#include "meremulatordevice.h"
#include "merqmakebuildconfiguration.h"
#include "merrpmvalidationparser.h"
#include "mersdkkitaspect.h"
#include "mersdkmanager.h"
#include "mersettings.h"
#include "ui_merrpminfo.h"

#include <sfdk/emulator.h>
#include <sfdk/sdk.h>
#include <sfdk/sfdkconstants.h>

#include <cmakeprojectmanager/cmakebuildconfiguration.h>
#include <coreplugin/fileutils.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildaspects.h>
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
#include <utils/layoutbuilder.h>
#include <utils/utilsicons.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QClipboard>
#include <QHeaderView>
#include <QMimeData>
#include <QScrollBar>
#include <QTimer>
#include <QTreeWidget>

using namespace Core;
using namespace ProjectExplorer;
using namespace CMakeProjectManager::Internal;
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

class ElidedWebsiteLabel : public QLabel
{
public:
    explicit ElidedWebsiteLabel(const QString &text, QWidget *parent=nullptr)
        : QLabel(QString(), parent)
        , m_text(text)
    {
    }
    void resizeEvent(QResizeEvent *event)
    {
        QWidget::resizeEvent(event);

        QFontMetrics metrics(font());
        QString elidedText = metrics.elidedText(m_text, Qt::ElideRight, width());
        setText(QString("<a href='%1'>%2</a>").arg(m_text, elidedText));
    }
private:
    const QString m_text;
};

class MerRpmValidationSuitesAspect : public BaseAspect
{
    Q_OBJECT

public:
    MerRpmValidationSuitesAspect()
    {
        setSettingsKey("MerRpmValidationSuites");
    }

    void setTarget(const BuildTargetData &target)
    {
        m_target = target;
        m_selectedSuites = defaultSuites();
    }

    void fromMap(const QVariantMap &map) override
    {
        QTC_ASSERT(!settingsKey().isEmpty(), return);

        m_selectedSuites = map.value(settingsKey(), defaultSuites()).toStringList();
    }

    void toMap(QVariantMap &map) const override
    {
        saveToMap(map, m_selectedSuites, defaultSuites());
    }

    void addToLayout(LayoutBuilder &builder) override
    {
        QTC_CHECK(!m_suitesTreeWidget);

        builder.addItem(new QLabel(tr("Suites:")));

        auto vbox = new QVBoxLayout;
        vbox->setContentsMargins(0, 0, 0, 0);

        m_suitesTreeWidget = new QTreeWidget;
        m_suitesTreeWidget->setColumnCount(3);
        m_suitesTreeWidget->setRootIsDecorated(false);
        m_suitesTreeWidget->setHeaderHidden(true);
        m_suitesTreeWidget->header()->setStretchLastSection(false);
        vbox->addWidget(m_suitesTreeWidget);

        auto warningHBox = new QHBoxLayout;
        warningHBox->setContentsMargins(0, 0, 0, 0);

        m_warningLabelIcon = new QLabel;
        warningHBox->addWidget(m_warningLabelIcon);
        m_warningLabel = new QLabel;
        warningHBox->addWidget(m_warningLabel, 1);

        vbox->addItem(warningHBox);

        bool hasSuite = false;
        bool hasSelectedSuite = false;
        for (const RpmValidationSuiteData &suite : m_target.rpmValidationSuites) {
            hasSuite = true;
            const bool selected = m_selectedSuites.contains(suite.id);
            hasSelectedSuite |= selected;
            auto *item = new QTreeWidgetItem(m_suitesTreeWidget);
            item->setText(0, suite.name);
            auto websiteLabel = new ElidedWebsiteLabel(suite.website);
            websiteLabel->setOpenExternalLinks(true);
            websiteLabel->setToolTip(suite.website);
            m_suitesTreeWidget->setItemWidget(item, 1, websiteLabel);
            item->setText(2, suite.essential
                    ? tr("Essential", "RPM validation suite")
                    : tr("Optional", "RPM validation suite"));
            item->setCheckState(0, selected ? Qt::Checked : Qt::Unchecked);
            item->setData(0, Qt::UserRole, suite.id);
        }
        m_suitesTreeWidget->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
        m_suitesTreeWidget->header()->setSectionResizeMode(1, QHeaderView::Stretch);
        m_suitesTreeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

        connect(m_suitesTreeWidget, &QTreeWidget::itemChanged,
                this, &MerRpmValidationSuitesAspect::onItemChanged);

        m_warningLabelIcon->setPixmap(Utils::Icons::CRITICAL.pixmap());
        updateWarningLabel(hasSuite, hasSelectedSuite);

        builder.addItem(vbox);
    }

    QStringList defaultSuites() const
    {
        QStringList defaultSuites;
        for (const RpmValidationSuiteData &suite : m_target.rpmValidationSuites) {
            if (suite.essential)
                defaultSuites.append(suite.id);
        }
        return defaultSuites;
    }

    QStringList selectedSuites() const { return m_selectedSuites; }

private:
    void onItemChanged()
    {
        QStringList checked;
        const int suitesCount = m_suitesTreeWidget->topLevelItemCount();
        for (int i = 0; i < suitesCount; ++i) {
            QTreeWidgetItem *item = m_suitesTreeWidget->topLevelItem(i);
            if (item->checkState(0) == Qt::Checked)
                checked.append(item->data(0, Qt::UserRole).toString());
        }
        m_selectedSuites = checked;
        updateWarningLabel(suitesCount > 0, checked.count() > 0);
    }

    void updateWarningLabel(bool hasSuite, bool hasSelectedSuite)
    {
        if (!hasSuite) {
            m_warningLabel->setText(tr("No RPM validation suite is available for the current "
                        "%1 build target").arg(Sdk::osVariant()));
            m_warningLabelIcon->setVisible(true);
            m_warningLabel->setVisible(true);
        } else if (!hasSelectedSuite) {
            m_warningLabel->setText(tr("At least one RPM validation suite must be selected"));
            m_warningLabelIcon->setVisible(true);
            m_warningLabel->setVisible(true);
        } else {
            m_warningLabelIcon->setVisible(false);
            m_warningLabel->setVisible(false);
        }
    }

private:
    Sfdk::BuildTargetData m_target;
    QStringList m_selectedSuites;
    QTreeWidget *m_suitesTreeWidget = nullptr;
    QLabel *m_warningLabel = nullptr;
    QLabel *m_warningLabelIcon = nullptr;
};

MerProcessStep::MerProcessStep(BuildStepList *bsl, Id id)
    :AbstractProcessStep(bsl,id)
{
    m_command = addAspect<StringAspect>();
    m_command->setSettingsKey("MerProcessStep.Command");
    m_command->setLabelText(tr("Command:"));
    m_command->setDisplayStyle(StringAspect::LabelDisplay);
    m_command->setReadOnly(true);
    m_command->setValue("sfdk deploy");

    m_arguments = addAspect<StringAspect>();
    m_arguments->setSettingsKey("MerProcessStep.Arguments");
    m_arguments->setLabelText(tr("Arguments:"));
    m_arguments->setDisplayStyle(StringAspect::LineEditDisplay);
}

bool MerProcessStep::init()
{
    return init(NoInitOption);
}

bool MerProcessStep::init(InitOptions options)
{
    BuildConfiguration *const bc = buildConfiguration();
    if (!qobject_cast<MerCMakeBuildConfiguration *>(bc)
            && !qobject_cast<MerCompilationDatabaseBuildConfiguration *>(bc)
            && !qobject_cast<MerQmakeBuildConfiguration *>(bc)) {
        addOutput(tr("Cannot deploy: Unsupported build configuration."),
                OutputFormat::ErrorMessage);
        return false;
    }

    const BuildEngine *const engine = MerSdkKitAspect::buildEngine(target()->kit());

    if (!engine) {
        addOutput(tr("Cannot deploy: Missing %1 build-engine information in the kit").arg(Sdk::osVariant()),
                OutputFormat::ErrorMessage);
        return false;
    }

    const QString target = MerSdkKitAspect::buildTargetName(this->target()->kit());

    if (target.isEmpty()) {
        addOutput(tr("Cannot deploy: Missing %1 build-target information in the kit").arg(Sdk::osVariant()),
                OutputFormat::ErrorMessage);
        return false;
    }

    IDevice::ConstPtr device = DeviceKitAspect::device(this->target()->kit());

    //TODO: HACK
    if (device.isNull() && !(options & DoNotNeedDevice)) {
        addOutput(tr("Cannot deploy: Missing %1 device information in the kit").arg(Sdk::osVariant()),
                OutputFormat::ErrorMessage);
        return false;
    }

    setCommandLineProvider([=]() {
        const FilePath toolsPath = engine->buildTarget(target).toolsPath;
        CommandLine deployCommand(toolsPath.pathAppended(Sfdk::Constants::WRAPPER_DEPLOY));
        deployCommand.addArgs(arguments(), CommandLine::Raw);
        return deployCommand;
    });

    bool retv = AbstractProcessStep::init();

    //TODO HACK
    if (!device.isNull()) {
        ProcessParameters *pp = processParameters();
        Environment env = pp->environment();
        env.appendOrSet(QLatin1String(Sfdk::Constants::MER_SSH_DEVICE_NAME), device->displayName());
        pp->setEnvironment(env);
    }

    return retv;
}

QString MerProcessStep::command() const
{
    return m_command->value();
}

void MerProcessStep::setCommand(const QString &command)
{
    m_command->setValue(command);
}

QString MerProcessStep::arguments() const
{
    return m_arguments->value();
}

void MerProcessStep::setArguments(const QString &arguments)
{
    m_arguments->setValue(arguments);
}

///////////////////////////////////////////////////////////////////////////////////////////

Utils::Id MerEmulatorStartStep::stepId()
{
    return Utils::Id("QmakeProjectManager.MerEmulatorStartStep");
}

QString MerEmulatorStartStep::displayName()
{
    return tr("Start Emulator");
}

MerEmulatorStartStep::MerEmulatorStartStep(BuildStepList *bsl, Utils::Id id)
    : MerAbstractVmStartStep(bsl, id)
{
    setDefaultDisplayName(displayName());
}

bool MerEmulatorStartStep::init()
{
    IDevice::ConstPtr d = DeviceKitAspect::device(this->target()->kit());
    const MerEmulatorDevice* device = dynamic_cast<const MerEmulatorDevice*>(d.data());
    if (!device) {
        setEnabled(false);
        return false;
    }

    setVirtualMachine(device->emulator()->virtualMachine());

    return MerAbstractVmStartStep::init();
}

///////////////////////////////////////////////////////////////////////////////////////////

Utils::Id MerConnectionTestStep::stepId()
{
    return Utils::Id("QmakeProjectManager.MerConnectionTestStep");
}

QString MerConnectionTestStep::displayName()
{
    return tr("Test Device Connection");
}

MerConnectionTestStep::MerConnectionTestStep(BuildStepList *bsl, Utils::Id id)
    : BuildStep(bsl, id)
{
    setDefaultDisplayName(displayName());
    setSummaryText(QString("<b>%1:</b> %2")
            .arg(displayName())
            .arg(tr("Verifies connection to the device can be established.")));
    setWidgetExpandedByDefault(false);
}

bool MerConnectionTestStep::init()
{
    IDevice::ConstPtr d = DeviceKitAspect::device(this->target()->kit());
    if (!d) {
        setEnabled(false);
        return false;
    }

    return true;
}

void MerConnectionTestStep::doRun()
{
    IDevice::ConstPtr d = DeviceKitAspect::device(this->target()->kit());
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

Utils::Id MerPrepareTargetStep::stepId()
{
    return Utils::Id("QmakeProjectManager.MerPrepareTargetStep");
}

QString MerPrepareTargetStep::displayName()
{
    return tr("Prepare Target");
}

MerPrepareTargetStep::MerPrepareTargetStep(BuildStepList *bsl, Utils::Id id)
    : BuildStep(bsl, id)
{
    setSummaryText(QString("<b>%1:</b> %2")
            .arg(displayName())
            .arg(tr("Prepares target device for deployment")));
    setWidgetExpandedByDefault(false);
}

bool MerPrepareTargetStep::init()
{
    IDevice::ConstPtr d = DeviceKitAspect::device(this->target()->kit());
    if (!d) {
        setEnabled(false);
        return false;
    }

    if (d->machineType() == IDevice::Emulator)
        m_impl = new MerEmulatorStartStep(stepList(), MerEmulatorStartStep::stepId());
    else
        m_impl = new MerConnectionTestStep(stepList(), MerConnectionTestStep::stepId());

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

///////////////////////////////////////////////////////////////////////////////////////////

Utils::Id MerMb2MakeInstallStep::stepId()
{
    return Utils::Id("QmakeProjectManager.MerMakeInstallBuildStep");
}

QString MerMb2MakeInstallStep::displayName()
{
    return QLatin1String("Make install");
}

MerMb2MakeInstallStep::MerMb2MakeInstallStep(BuildStepList *bsl, Id id)
    : MerProcessStep(bsl, id)
{
    setCommand("sfdk make-install");
    setSummaryUpdater([this]() {
        BuildConfiguration *const bc = buildConfiguration();
        const QString summary = tr("make install in %1").arg(bc->buildDirectory().toString());
        return QString("<b>%1:</b> %2")
                .arg(displayName())
                .arg(summary);
    });

}

bool MerMb2MakeInstallStep::init()
{
    bool success = MerProcessStep::init(DoNotNeedDevice);
    //hack
    ProcessParameters *pp = processParameters();
    QString make = pp->command().executable().toString();
    make.replace(
            QLatin1String(Sfdk::Constants::WRAPPER_DEPLOY),
            QLatin1String(Sfdk::Constants::WRAPPER_MAKE_INSTALL));
    CommandLine makeCommand(make);
    makeCommand.addArgs(pp->command().arguments(), CommandLine::Raw);
    pp->setCommandLine(makeCommand);
    return success;
}

void MerMb2MakeInstallStep::doRun()
{
   AbstractProcessStep::doRun();
}

///////////////////////////////////////////////////////////////////////////////////////////

Utils::Id MerMb2RsyncDeployStep::stepId()
{
    return Utils::Id("QmakeProjectManager.MerRsyncDeployStep");
}

QString MerMb2RsyncDeployStep::displayName()
{
    return QLatin1String("Rsync deploy");
}

MerMb2RsyncDeployStep::MerMb2RsyncDeployStep(BuildStepList *bsl, Utils::Id id)
    : MerProcessStep(bsl, id)
{
    setSummaryText(QString("<b>%1:</b> %2")
            .arg(displayName())
            .arg(tr("Deploys with rsync")));
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////

Utils::Id MerLocalRsyncDeployStep::stepId()
{
    return Utils::Id("QmakeProjectManager.MerLocalRsyncDeployStep");
}

QString MerLocalRsyncDeployStep::displayName()
{
    return QLatin1String("Local Rsync");
}

MerLocalRsyncDeployStep::MerLocalRsyncDeployStep(BuildStepList *bsl, Utils::Id id)
    : MerProcessStep(bsl, id)
{
    setCommand(tr("Deploy using local installed Rsync"));
    setSummaryText(QString("<b>%1:</b> %2")
            .arg(displayName())
            .arg(tr("Deploys with local installed rsync")));
}

bool MerLocalRsyncDeployStep::init()
{
    BuildConfiguration *const bc = buildConfiguration();
    if (!qobject_cast<MerCMakeBuildConfiguration *>(bc)
            && !qobject_cast<MerCompilationDatabaseBuildConfiguration *>(bc)
            && !qobject_cast<MerQmakeBuildConfiguration *>(bc)) {
        addOutput(tr("Cannot deploy: Unsupported build configuration."),
                OutputFormat::ErrorMessage);
        return false;
    }

    BuildEngine *const engine = MerSdkKitAspect::buildEngine(target()->kit());

    if (!engine) {
        addOutput(tr("Cannot deploy: Missing %1 build-engine information in the kit").arg(Sdk::osVariant()),
                OutputFormat::ErrorMessage);
        return false;
    }

    const QString target = MerSdkKitAspect::buildTargetName(this->target()->kit());

    if (target.isEmpty()) {
        addOutput(tr("Cannot deploy: Missing %1 build-target information in the kit").arg(Sdk::osVariant()),
                OutputFormat::ErrorMessage);
        return false;
    }

    setCommandLineProvider([this]() {
        CommandLine deployCommand("rsync");
        deployCommand.addArgs(arguments(), CommandLine::Raw);
        return deployCommand;
    });

    return AbstractProcessStep::init();
}

void MerLocalRsyncDeployStep::doRun()
{
   emit addOutput(tr("Deploying binaries..."), OutputFormat::NormalMessage);
   AbstractProcessStep::doRun();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////


Utils::Id MerMb2RpmDeployStep::stepId()
{
    return Utils::Id("QmakeProjectManager.MerRpmDeployStep");
}

QString MerMb2RpmDeployStep::displayName()
{
    return QLatin1String("RPM Deploy");
}

MerMb2RpmDeployStep::MerMb2RpmDeployStep(BuildStepList *bsl, Utils::Id id)
    : MerProcessStep(bsl, id)
{
    setSummaryText(QString("<b>%1:</b> %2")
            .arg(displayName())
            .arg(tr("Deploys RPM package")));
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

//TODO: HACK
////////////////////////////////////////////////////////////////////////////////////////////////////////////


Utils::Id MerMb2RpmBuildStep::stepId()
{
    return Utils::Id("QmakeProjectManager.MerRpmBuildStep");
}

QString MerMb2RpmBuildStep::displayName()
{
    return QLatin1String("RPM Build");
}

MerMb2RpmBuildStep::MerMb2RpmBuildStep(BuildStepList *bsl, Utils::Id id)
    : MerProcessStep(bsl, id)
{
    setCommand(QLatin1String("sfdk package"));
    setSummaryText(QString("<b>%1:</b> %2")
            .arg(displayName())
            .arg(tr("Builds RPM package")));
}

bool MerMb2RpmBuildStep::init()
{
    m_showResultDialog = !deployConfiguration()->stepList()->contains(MerMb2RpmDeployStep::stepId());

    bool success = MerProcessStep::init(DoNotNeedDevice);
    m_packages.clear();
    BuildEngine *const engine = MerSdkKitAspect::buildEngine(target()->kit());
    m_sharedSrc = QDir::cleanPath(engine->sharedSrcPath().toString());

    //hack
    ProcessParameters *pp = processParameters();
    QString rpm = pp->command().executable().toString();
    rpm.replace(
            QLatin1String(Sfdk::Constants::WRAPPER_DEPLOY),
            QLatin1String(Sfdk::Constants::WRAPPER_RPM));
    CommandLine rpmCommand(rpm);
    rpmCommand.addArgs(pp->command().arguments(), CommandLine::Raw);

    auto aspect = buildConfiguration()->aspect<MerBuildConfigurationAspect>();
    QTC_ASSERT(aspect, return false);
    if (aspect->signPackages())
        rpmCommand.addArgs("--sign", CommandLine::Raw);

    pp->setCommandLine(rpmCommand);
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
    if (m_showResultDialog && exitCode == 0 && status == QProcess::NormalExit && !m_packages.isEmpty()) {
        new RpmInfo(m_packages, ICore::dialogParent());
    }
}

QString MerMb2RpmBuildStep::mainPackageFileName() const
{
    QTC_ASSERT(!m_packages.isEmpty(), return {});
    return m_packages.first();
}

void MerMb2RpmBuildStep::stdOutput(const QString &line)
{
    QRegExp rexp(QLatin1String("^Wrote: (.*RPMS.*\\.rpm)"));
    if (rexp.indexIn(line) != -1) {
        QString file = rexp.cap(1);
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

Utils::Id MerRpmValidationStep::stepId()
{
    return Utils::Id("QmakeProjectManager.MerRpmValidationStep");
}

QString MerRpmValidationStep::displayName()
{
    return QLatin1String("RPM Validation");
}

MerRpmValidationStep::MerRpmValidationStep(BuildStepList *bsl, Utils::Id id)
    : MerProcessStep(bsl, id)
{
    setEnabled(MerSettings::rpmValidationByDefault());
    setSummaryText(QString("<b>%1:</b> %2")
            .arg(displayName())
            .arg(tr("Validates RPM package")));

    m_suites = addAspect<MerRpmValidationSuitesAspect>();
    m_suites->setTarget(MerSdkKitAspect::buildTarget(target()->kit()));
    connect(m_suites, &BaseAspect::changed, this, [this]() {
        updateFixedArguments(m_suites->selectedSuites());
    });
    updateFixedArguments(m_suites->selectedSuites());
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

    return true;
}

void MerRpmValidationStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new MerRpmValidationParser);
    MerProcessStep::setupOutputFormatter(formatter);
}

void MerRpmValidationStep::doRun()
{
    const BuildTargetData target = MerSdkKitAspect::buildTarget(this->target()->kit());

    if (target.rpmValidationSuites.isEmpty()) {
        const QString message(tr("No RPM validation suite is available for the current "
                    "%1 build target, the package will not be validated").arg(Sdk::osVariant()));
        emit addOutput(message, OutputFormat::ErrorMessage);
        emit addTask(Task(Task::Error, message, Utils::FilePath(), -1,
                    ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM),
                2);
        emit addOutput("  " + tr("Disable the RPM Validation deploy step to avoid this error"),
                OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    } else if (m_suites->selectedSuites().isEmpty()) {
        const QString message(tr("No RPM validation suite is selected in deployment settings,"
                    " the package will not be validated"));
        emit addOutput(message, OutputFormat::ErrorMessage);
        emit addTask(Task(Task::Error, message, Utils::FilePath(), -1,
                    ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM),
                2);
        emit addOutput("  " + tr("Either select at least one suite or disable the RPM Validation "
                    "deploy step to avoid this error"), OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    emit addOutput(tr("Validating RPM package..."), OutputFormat::NormalMessage);

    const QString packageFile = m_packagingStep->mainPackageFileName();
    if(!packageFile.endsWith(QLatin1String(".rpm"))){
        const QString message(tr("No package to validate found"));
        emit addTask(Task(Task::Error, message, Utils::FilePath(), -1,
                    ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM),
                1);
        emit addOutput(message, OutputFormat::ErrorMessage);
        emit finished(false);
        return;
    }

    // hack
    ProcessParameters *pp = processParameters();
    QString rpmValidation = pp->command().executable().toString();
    rpmValidation.replace(
            QLatin1String(Sfdk::Constants::WRAPPER_DEPLOY),
            QLatin1String(Sfdk::Constants::WRAPPER_RPMVALIDATION));
    CommandLine rpmValidationCommand(rpmValidation);
    QStringList arguments{ m_fixedArguments, this->arguments(), packageFile };
    rpmValidationCommand.addArgs(arguments.join(' '), CommandLine::Raw);
    pp->setCommandLine(rpmValidationCommand);

    AbstractProcessStep::doRun();
}

void MerRpmValidationStep::updateFixedArguments(const QStringList &selectedSuites)
{
    if (!selectedSuites.isEmpty()) {
        if (selectedSuites != m_suites->defaultSuites())
            m_fixedArguments = "--suites " + selectedSuites.join(',');
        else
            m_fixedArguments.clear();
    } else {
        m_fixedArguments.clear();
    }

    QString command = "rpmvalidation";
    if (!m_fixedArguments.isEmpty()) {
        command += ' ';
        command += m_fixedArguments;
    }
    setCommand(command);
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
    m_runner->run(m_command, deviceConfiguration()->sshParameters());
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

MerResetAmbienceDeployStep::MerResetAmbienceDeployStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id)
    : AbstractRemoteLinuxDeployStep(bsl, id)
{
    QString ambienceName = target()->project()->displayName();

    QFile scriptFile(QStringLiteral(":/mer/reset-ambience.sh"));
    bool ok = scriptFile.open(QIODevice::ReadOnly);
    QTC_CHECK(ok);
    QString script = QString::fromUtf8(scriptFile.readAll());
    script.prepend(QStringLiteral("AMBIENCE_URL='file:///usr/share/ambience/%1/%1.ambience'\n").arg(ambienceName));

    auto service = createDeployService<MerNamedCommandDeployService>();
    service->setDescription(tr("Starting remote command to reset ambience '%1'...").arg(ambienceName));
    service->setCommand(script);

    setInternalInitializer([service]() {
        return service->isDeploymentPossible();
    });
}

Utils::Id MerResetAmbienceDeployStep::stepId()
{
    return Utils::Id("Mer.MerResetAmbienceDeployStep");
}

QString MerResetAmbienceDeployStep::displayName()
{
    return tr("Reset %1 Ambience").arg(Sdk::osVariant());
}

} // Internal
} // Mer

#include "merdeploysteps.moc"
