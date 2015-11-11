/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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
#include "mervirtualboxmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakebuildconfiguration.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>

#include <QMessageBox>
#include <QTimer>

using namespace Core;
using namespace ProjectExplorer;
using namespace QmakeProjectManager;
using namespace QSsh;
using namespace Utils;

namespace Mer {
namespace Internal {

namespace {
const int CONNECTION_TEST_CHECK_FOR_CANCEL_INTERVAL = 2000;
}

class MerConnectionTestStepConfigWidget : public SimpleBuildStepConfigWidget
{
    Q_OBJECT

public:
    MerConnectionTestStepConfigWidget(MerConnectionTestStep *step)
        : SimpleBuildStepConfigWidget(step)
    {
    }

    QString summaryText() const override
    {
        return QString::fromLatin1("<b>%1:</b> %2")
            .arg(displayName())
            .arg(tr("Verifies connection to the device can be established."));
    }
};

class MerPrepareTargetStepConfigWidget : public SimpleBuildStepConfigWidget
{
    Q_OBJECT

public:
    MerPrepareTargetStepConfigWidget(MerPrepareTargetStep *step)
        : SimpleBuildStepConfigWidget(step)
    {
    }

    QString summaryText() const override
    {
        return QString::fromLatin1("<b>%1:</b> %2")
            .arg(displayName())
            .arg(tr("Prepares target device for deployment"));
    }
};

MerProcessStep::MerProcessStep(BuildStepList *bsl, Id id)
    :AbstractProcessStep(bsl,id)
{

}

MerProcessStep::MerProcessStep(BuildStepList *bsl, MerProcessStep *bs)
    :AbstractProcessStep(bsl,bs)
{

}

bool MerProcessStep::init(InitOptions options)
{
    QmakeBuildConfiguration *bc = qobject_cast<QmakeBuildConfiguration*>(buildConfiguration());
    if (!bc)
        bc =  qobject_cast<QmakeBuildConfiguration*>(target()->activeBuildConfiguration());

    if (!bc) {
        addOutput(tr("Cannot deploy: No active build configuration."),
            ErrorMessageOutput);
        return false;
    }

    const MerSdk *const merSdk = MerSdkKitInformation::sdk(target()->kit());

    if (!merSdk) {
        addOutput(tr("Cannot deploy: Missing MerSdk information in the kit"),ErrorMessageOutput);
        return false;
    }

    const QString target = MerTargetKitInformation::targetName(this->target()->kit());

    if (target.isEmpty()) {
        addOutput(tr("Cannot deploy: Missing MerTarget information in the kit"),ErrorMessageOutput);
        return false;
    }

    IDevice::ConstPtr device = DeviceKitInformation::device(this->target()->kit());

    //TODO: HACK
    if (device.isNull() && !(options & DoNotNeedDevice)) {
        addOutput(tr("Cannot deploy: Missing MerDevice information in the kit"),ErrorMessageOutput);
        return false;
    }


    const QString projectDirectory = bc->isShadowBuild() ? bc->rawBuildDirectory().toString() : project()->projectDirectory().toString();
    const QString wrapperScriptsDir =  MerSdkManager::sdkToolsDirectory() + merSdk->virtualMachineName()
            + QLatin1Char('/') + target;
    const QString deployCommand = wrapperScriptsDir + QLatin1Char('/') + QLatin1String(Constants::MER_WRAPPER_DEPLOY);

    ProcessParameters *pp = processParameters();

    Environment env = bc ? bc->environment() : Environment::systemEnvironment();

    // By default, MER_SSH_PROJECT_PATH is set to the project explorer variable
    // "%{CurrentProject:Path}". If multiple projects are open, "CurrentProject" may
    // not be the project set as active (i.e. the project being deployed), leading
    // to RPM build looking for files in the wrong project. Instead, MER_SSH_PROJECT_PATH
    // needs to be set to the source directory of the active project. Note that this path
    // is the same, whether or not shadow builds are enabled.
    env.set(QLatin1String(Constants::MER_SSH_PROJECT_PATH), project()->projectDirectory().toString());

    //TODO HACK
    if(!device.isNull())
        env.appendOrSet(QLatin1String(Constants::MER_SSH_DEVICE_NAME),device->displayName());
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

MerDeployConfiguration *MerProcessStep::deployConfiguration() const
{
    return qobject_cast<MerDeployConfiguration *>(AbstractProcessStep::deployConfiguration());
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

MerEmulatorStartStep::MerEmulatorStartStep(BuildStepList *bsl, MerEmulatorStartStep *bs)
    : MerAbstractVmStartStep(bsl, bs)
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

    setConnection(device->connection());

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
}

MerConnectionTestStep::MerConnectionTestStep(BuildStepList *bsl,
        MerConnectionTestStep *bs)
    : BuildStep(bsl, bs)
{
    setDefaultDisplayName(displayName());
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

void MerConnectionTestStep::run(QFutureInterface<bool> &fi)
{
    IDevice::ConstPtr d = DeviceKitInformation::device(this->target()->kit());
    if (!d) {
        fi.reportResult(false);
        emit finished();
        return;
    }

    m_futureInterface = &fi;

    m_connection = new SshConnection(d->sshParameters(), this);
    connect(m_connection, &SshConnection::connected,
            this, &MerConnectionTestStep::onConnected);
    connect(m_connection, &SshConnection::error,
            this, &MerConnectionTestStep::onConnectionFailure);

    emit addOutput(tr("%1: Testing connection to \"%2\"...")
            .arg(displayName()).arg(d->displayName()), MessageOutput);

    m_checkForCancelTimer = new QTimer(this);
    connect(m_checkForCancelTimer, &QTimer::timeout,
            this, &MerConnectionTestStep::checkForCancel);
    m_checkForCancelTimer->start(CONNECTION_TEST_CHECK_FOR_CANCEL_INTERVAL);

    m_connection->connectToHost();
}

BuildStepConfigWidget *MerConnectionTestStep::createConfigWidget()
{
    return new MerConnectionTestStepConfigWidget(this);
}

bool MerConnectionTestStep::immutable() const
{
    return false;
}

bool MerConnectionTestStep::runInGuiThread() const
{
    return true;
}

void MerConnectionTestStep::onConnected()
{
    finish(true);
}

void MerConnectionTestStep::onConnectionFailure()
{
    finish(false);
}

void MerConnectionTestStep::checkForCancel()
{
    if (m_futureInterface->isCanceled()) {
        finish(false);
    }
}

void MerConnectionTestStep::finish(bool result)
{
    m_connection->disconnect(this);
    m_connection->deleteLater(), m_connection = 0;

    m_futureInterface->reportResult(result);
    m_futureInterface = 0;

    delete m_checkForCancelTimer, m_checkForCancelTimer = 0;

    emit finished();
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
}

MerPrepareTargetStep::MerPrepareTargetStep(BuildStepList *bsl,
        MerPrepareTargetStep *bs)
    : BuildStep(bsl, bs)
{
    setDefaultDisplayName(displayName());
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

    connect(m_impl, &BuildStep::addTask,
            this, &MerPrepareTargetStep::addTask);
    connect(m_impl, &BuildStep::addOutput,
            this, &MerPrepareTargetStep::addOutput);
    connect(m_impl, &BuildStep::finished,
            this, &MerPrepareTargetStep::onImplFinished);

    return true;
}

void MerPrepareTargetStep::run(QFutureInterface<bool> &fi)
{
    m_impl->run(fi);
}

BuildStepConfigWidget *MerPrepareTargetStep::createConfigWidget()
{
    return new MerPrepareTargetStepConfigWidget(this);
}

bool MerPrepareTargetStep::immutable() const
{
    return false;
}

bool MerPrepareTargetStep::runInGuiThread() const
{
    return true;
}

void MerPrepareTargetStep::onImplFinished()
{
    m_impl->disconnect(this);
    m_impl->deleteLater(), m_impl = 0;
    emit finished();
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

MerMb2RsyncDeployStep::MerMb2RsyncDeployStep(BuildStepList *bsl, MerMb2RsyncDeployStep *bs)
    :MerProcessStep(bsl,bs)
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--rsync"));
}

bool MerMb2RsyncDeployStep::init()
{
    return MerProcessStep::init();
}

bool MerMb2RsyncDeployStep::immutable() const
{
    return false;
}

void MerMb2RsyncDeployStep::run(QFutureInterface<bool> &fi)
{
   emit addOutput(tr("Deploying binaries..."), MessageOutput);
   AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerMb2RsyncDeployStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Deploys with rsync."));
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

MerLocalRsyncDeployStep::MerLocalRsyncDeployStep(BuildStepList *bsl, MerLocalRsyncDeployStep *bs)
    :MerProcessStep(bsl,bs)
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
            ErrorMessageOutput);
        return false;
    }

    const MerSdk *const merSdk = MerSdkKitInformation::sdk(target()->kit());

    if (!merSdk) {
        addOutput(tr("Cannot deploy: Missing MerSdk information in the kit"),ErrorMessageOutput);
        return false;
    }

    const QString target = MerTargetKitInformation::targetName(this->target()->kit());

    if (target.isEmpty()) {
        addOutput(tr("Cannot deploy: Missing MerTarget information in the kit"),ErrorMessageOutput);
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

bool MerLocalRsyncDeployStep::immutable() const
{
    return false;
}

void MerLocalRsyncDeployStep::run(QFutureInterface<bool> &fi)
{
   emit addOutput(tr("Deploying binaries..."), MessageOutput);
   AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerLocalRsyncDeployStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Deploys with local installed rsync."));
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


MerMb2RpmDeployStep::MerMb2RpmDeployStep(BuildStepList *bsl, MerMb2RpmDeployStep *bs)
    :MerProcessStep(bsl,bs)
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--sdk"));
}

bool MerMb2RpmDeployStep::init()
{
    return MerProcessStep::init();
}

bool MerMb2RpmDeployStep::immutable() const
{
    return false;
}

void MerMb2RpmDeployStep::run(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Deploying RPM package..."), MessageOutput);
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerMb2RpmDeployStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Deploys RPM package."));
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


MerMb2RpmBuildStep::MerMb2RpmBuildStep(BuildStepList *bsl, MerMb2RpmBuildStep *bs)
    :MerProcessStep(bsl,bs)
{
    setDefaultDisplayName(displayName());
}

bool MerMb2RpmBuildStep::init()
{
    bool success = MerProcessStep::init(DoNotNeedDevice);
    m_packages.clear();
    const MerSdk *const sdk = MerSdkKitInformation::sdk(target()->kit());
    m_sharedHome = QDir::cleanPath(sdk->sharedHomePath());
    m_sharedSrc = QDir::cleanPath(sdk->sharedSrcPath());

    //hack
    ProcessParameters *pp = processParameters();
    QString deployCommand = pp->command();
    deployCommand.replace(QLatin1String(Constants::MER_WRAPPER_DEPLOY),QLatin1String(Constants::MER_WRAPPER_RPM));
    pp->setCommand(deployCommand);
    return success;
}

bool MerMb2RpmBuildStep::immutable() const
{
    return false;
}
//TODO: This is hack
void MerMb2RpmBuildStep::run(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Building RPM package..."), MessageOutput);
    AbstractProcessStep::run(fi);
}
//TODO: This is hack
void MerMb2RpmBuildStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    //TODO:
    MerProcessStep::processFinished(exitCode, status);
    if(exitCode == 0 && status == QProcess::NormalExit && !m_packages.isEmpty()){
        new RpmInfo(m_packages);
    }
}

QStringList MerMb2RpmBuildStep::packagesFilePath() const
{
    return m_packages;
}

BuildStepConfigWidget *MerMb2RpmBuildStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Builds RPM package."));
    widget->setCommandText(QLatin1String("mb2 rpm"));
    return widget;
}

void MerMb2RpmBuildStep::stdOutput(const QString &line)
{
    QRegExp rexp(QLatin1String("^Wrote: (/.*RPMS.*\\.rpm)"));
    if (rexp.indexIn(line) != -1) {
        QString file = rexp.cap(1);
        //TODO First replace shared home and then shared src (error prone!)
        file.replace(QRegExp(QLatin1String("^/home/mersdk/share")),m_sharedHome);
        file.replace(QRegExp(QLatin1String("^/home/src1")),m_sharedSrc);
        m_packages.append(QDir::toNativeSeparators(file));
    }
    MerProcessStep::stdOutput(line);
}

RpmInfo::RpmInfo(const QStringList& list):
    m_list(list)
{
    QTimer::singleShot(0, this, &RpmInfo::info);
}

void RpmInfo::info()
{
    QString message(QLatin1String("Following packages have been created:"));
    message.append(QLatin1String("<ul>"));
    foreach(const QString file,m_list) {
        message.append(QLatin1String("<li>"));
        message.append(file);
        message.append(QLatin1String("</li>"));
    }
    message.append(QLatin1String("</ul>"));
    QMessageBox::information(ICore::mainWindow(), tr("Packages created"),message);
    this->deleteLater();
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
}


MerRpmValidationStep::MerRpmValidationStep(BuildStepList *bsl, MerRpmValidationStep *bs)
    :MerProcessStep(bsl,bs)
{
    setEnabled(MerSettings::rpmValidationByDefault());
    setDefaultDisplayName(displayName());
}

bool MerRpmValidationStep::init()
{
    if (!MerProcessStep::init(DoNotNeedDevice)) {
        return false;
    }

    m_packagingStep = deployConfiguration()->earlierBuildStep<MerMb2RpmBuildStep>(this);
    if (!m_packagingStep) {
        emit addOutput(tr("Cannot validate: No previous \"%1\" step found")
                .arg(MerMb2RpmBuildStep::displayName()),
                ErrorMessageOutput);
        return false;
    }

    setOutputParser(new MerRpmValidationParser);

    return true;
}

bool MerRpmValidationStep::immutable() const
{
    return false;
}

void MerRpmValidationStep::run(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Validating RPM package..."), MessageOutput);

    const QString packageFile = m_packagingStep->packagesFilePath().first();
    if(!packageFile.endsWith(QLatin1String(".rpm"))){
        const QString message((tr("No package to validate found in %1")).arg(packageFile));
        emit addOutput(message, ErrorMessageOutput);
        fi.reportResult(false);
        emit finished();
        return;
    }

    // hack
    ProcessParameters *pp = processParameters();
    QString deployCommand = pp->command();
    deployCommand.replace(QLatin1String(Constants::MER_WRAPPER_DEPLOY),QLatin1String(Constants::MER_WRAPPER_RPMVALIDATION));
    pp->setCommand(deployCommand);
    pp->setArguments(packageFile);

    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerRpmValidationStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Validates RPM package."));
    widget->setCommandText(QLatin1String("rpmvalidation"));
    return widget;
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////


MerDeployStepWidget::MerDeployStepWidget(MerProcessStep *step)
        : m_step(step)
{
    m_ui.setupUi(this);
    m_ui.commandArgumentsLineEdit->setText(m_step->arguments());
    connect(m_ui.commandArgumentsLineEdit, &QLineEdit::textEdited,
            this, &MerDeployStepWidget::commandArgumentsLineEditTextEdited);
}

QString MerDeployStepWidget::summaryText() const
{
    return QString::fromLatin1("<b>%1:</b> %2").arg(displayName()).arg(m_summaryText);
}

QString MerDeployStepWidget::displayName() const
{
    return m_displayText;
}

void MerDeployStepWidget::commandArgumentsLineEditTextEdited()
{
    m_step->setArguments(m_ui.commandArgumentsLineEdit->text());
}

void MerDeployStepWidget::setCommandText(const QString& commandText)
{
     m_ui.commandLabelEdit->setText(commandText);
}

void MerDeployStepWidget::setDisplayName(const QString& displayText)
{
    m_displayText = displayText;
}

void MerDeployStepWidget::setSummaryText(const QString& summaryText)
{
    m_summaryText = summaryText;
}

QString MerDeployStepWidget::commnadText() const
{
   return  m_ui.commandLabelEdit->text();
}

} // Internal
} // Mer

#include "merdeploysteps.moc"
