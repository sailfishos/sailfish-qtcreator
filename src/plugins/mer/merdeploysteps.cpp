/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
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
#include "merconstants.h"
#include "merdeploysteps.h"
#include "mersdkmanager.h"
#include "mersdkkitinformation.h"
#include "mertargetkitinformation.h"
#include "merconnectionmanager.h"
#include "meremulatordevice.h"
#include "merconnectionprompt.h"
#include "mervirtualboxmanager.h"
#include "merconnection.h"
#include <utils/qtcassert.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <coreplugin/idocument.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/variablemanager.h>
#include <QMessageBox>
#include <QTimer>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

class MerSimpleBuildStepConfigWidget : public BuildStepConfigWidget {
public:

    MerSimpleBuildStepConfigWidget(const QString& displayText, const QString& summaryText)
        :m_displayText(displayText),m_summaryText(summaryText){}

    QString summaryText() const
    {
        return QString::fromLatin1("<b>%1:</b> %2").arg(displayName()).arg(m_summaryText);
    }

    QString displayName() const
    {
        return m_displayText;
    }

     bool showWidget() const { return false; }

private:
    QString m_displayText;
    QString m_summaryText;
};


MerProcessStep::MerProcessStep(ProjectExplorer::BuildStepList *bsl,const Core::Id id)
    :AbstractProcessStep(bsl,id)
{

}

MerProcessStep::MerProcessStep(ProjectExplorer::BuildStepList *bsl, MerProcessStep *bs)
    :AbstractProcessStep(bsl,bs)
{

}

bool MerProcessStep::init()
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();

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
    if (device.isNull() && DeviceTypeKitInformation::deviceTypeId(this->target()->kit()) != Constants::MER_DEVICE_TYPE_ARM) {
        addOutput(tr("Cannot deploy: Missing MerDevice information in the kit"),ErrorMessageOutput);
        return false;
    }

    const QString projectDirectory = project()->projectDirectory();
    const QString wrapperScriptsDir =  MerSdkManager::sdkToolsDirectory() + merSdk->virtualMachineName()
            + QLatin1Char('/') + target;
    const QString deployCommand = wrapperScriptsDir + QLatin1Char('/') + QLatin1String(Constants::MER_WRAPPER_DEPLOY);

    ProcessParameters *pp = processParameters();

    Utils::Environment env = bc ? bc->environment() : Utils::Environment::systemEnvironment();
    //TODO HACK
    if(!device.isNull())
        env.appendOrSet(QLatin1String(Constants::MER_SSH_DEVICE_NAME),device->displayName());
    pp->setMacroExpander(bc ? bc->macroExpander() : Core::VariableManager::instance()->macroExpander());
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

const Core::Id MerEmulatorStartStep::stepId()
{
    return Core::Id("Qt4ProjectManager.MerEmulatorStartStep");
}

QString MerEmulatorStartStep::displayName()
{
    return tr("Start Emulator");
}

MerEmulatorStartStep::MerEmulatorStartStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}

MerEmulatorStartStep::MerEmulatorStartStep(ProjectExplorer::BuildStepList *bsl, MerEmulatorStartStep *bs)
    :MerProcessStep(bsl,bs)
    , m_vm(bs->vitualMachine())
    , m_ssh(bs->sshParams())
{
    setDefaultDisplayName(displayName());
}

QString MerEmulatorStartStep::vitualMachine()
{
    return m_vm;
}

QSsh::SshConnectionParameters MerEmulatorStartStep::sshParams()
{
    return m_ssh;
}

bool MerEmulatorStartStep::init()
{
    IDevice::ConstPtr d = DeviceKitInformation::device(this->target()->kit());
    if(d.isNull() || d->type() != Constants::MER_DEVICE_TYPE_I486) {
        setEnabled(false);
        return false;
    }
    const MerEmulatorDevice* device = static_cast<const MerEmulatorDevice*>(d.data());
    m_vm = device->virtualMachine();
    m_ssh = device->sshParameters();
    return !m_vm.isEmpty();
}

bool MerEmulatorStartStep::immutable() const
{
    return false;
}

void MerEmulatorStartStep::run(QFutureInterface<bool> &fi)
{
    MerConnectionManager *em = MerConnectionManager::instance();
    if(em->isConnected(m_vm)) {
        emit addOutput(tr("Emulator is already running. Nothing to do."),MessageOutput);
        fi.reportResult(true);
        emit finished();
    } else {
        emit addOutput(tr("Starting Emulator..."), MessageOutput);
        QString error = tr("Could not connect to %1 Virtual Machine.").arg(m_vm);
        MerRemoteConnection::createConnectionErrorTask(m_vm,error,Constants::MER_TASKHUB_EMULATOR_CATEGORY);
        if(!MerVirtualBoxManager::isVirtualMachineRunning(m_vm)) {
            MerConnectionPrompt *connectioPrompt = new MerConnectionPrompt(m_vm, 0);
            connectioPrompt->prompt(MerConnectionPrompt::Start);
        }
        fi.reportResult(false);
        emit finished();
    }
}

BuildStepConfigWidget *MerEmulatorStartStep::createConfigWidget()
{
    return new MerSimpleBuildStepConfigWidget(displayName(),tr("Starts Emulator virtual machine, if necessary."));
}

///////////////////////////////////////////////////////////////////////////////////////////

const Core::Id MerRsyncDeployStep::stepId()
{
    return Core::Id("Qt4ProjectManager.MerRsyncDeployStep");
}

QString MerRsyncDeployStep::displayName()
{
    return QLatin1String("Rsync");
}

MerRsyncDeployStep::MerRsyncDeployStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--rsync"));
}

MerRsyncDeployStep::MerRsyncDeployStep(ProjectExplorer::BuildStepList *bsl, MerRsyncDeployStep *bs)
    :MerProcessStep(bsl,bs)
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--rsync"));
}

bool MerRsyncDeployStep::init()
{
    return MerProcessStep::init();
}

bool MerRsyncDeployStep::immutable() const
{
    return false;
}

void MerRsyncDeployStep::run(QFutureInterface<bool> &fi)
{
   emit addOutput(tr("Deploying binaries..."), MessageOutput);
   AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerRsyncDeployStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Deploys with rsync."));
    return widget;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////


const Core::Id MerRpmDeployStep::stepId()
{
    return Core::Id("Qt4ProjectManager.MerRpmDeployStep");
}

QString MerRpmDeployStep::displayName()
{
    return QLatin1String("Rpm");
}

MerRpmDeployStep::MerRpmDeployStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--pkcon"));
}


MerRpmDeployStep::MerRpmDeployStep(ProjectExplorer::BuildStepList *bsl, MerRpmDeployStep *bs)
    :MerProcessStep(bsl,bs)
{
    setDefaultDisplayName(displayName());
    setArguments(QLatin1String("--pkcon"));
}

bool MerRpmDeployStep::init()
{
    return MerProcessStep::init();
}

bool MerRpmDeployStep::immutable() const
{
    return false;
}

void MerRpmDeployStep::run(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Deploying rpm package..."), MessageOutput);
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerRpmDeployStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Deploys rpm package."));
    return widget;
}

//TODO: HACK
////////////////////////////////////////////////////////////////////////////////////////////////////////////


const Core::Id MerRpmBuildStep::stepId()
{
    return Core::Id("Qt4ProjectManager.MerRpmBuildStep");
}

QString MerRpmBuildStep::displayName()
{
    return QLatin1String("Rpm");
}

MerRpmBuildStep::MerRpmBuildStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}


MerRpmBuildStep::MerRpmBuildStep(ProjectExplorer::BuildStepList *bsl, MerRpmBuildStep *bs)
    :MerProcessStep(bsl,bs)
{
    setDefaultDisplayName(displayName());
}

bool MerRpmBuildStep::init()
{
    bool success = MerProcessStep::init();
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

bool MerRpmBuildStep::immutable() const
{
    return false;
}
//TODO: This is hack
void MerRpmBuildStep::run(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Building rpm package..."), MessageOutput);
    AbstractProcessStep::run(fi);
}
//TODO: This is hack
void MerRpmBuildStep::processFinished(int exitCode, QProcess::ExitStatus status)
{
    MerProcessStep::processFinished(exitCode, status);
    if(exitCode == 0 && status == QProcess::NormalExit && !m_packages.isEmpty()){
        new RpmInfo(m_packages);
    }
}

BuildStepConfigWidget *MerRpmBuildStep::createConfigWidget()
{
    MerDeployStepWidget *widget = new MerDeployStepWidget(this);
    widget->setDisplayName(displayName());
    widget->setSummaryText(tr("Builds rpm package."));
    widget->setCommandText(QLatin1String("mb2 rpm"));
    return widget;
}

void MerRpmBuildStep::stdOutput(const QString &line)
{
    QRegExp rexp(QLatin1String("^Wrote: (/.*RPMS.*\\.rpm)"));
    if (rexp.indexIn(line) != -1) {
        QString file = rexp.cap(1);
        //TODO First replace shared home and then shared src (error prone!)
        file.replace(QRegExp(QLatin1String("^/home/mersdk")),m_sharedHome);
        file.replace(QRegExp(QLatin1String("^/home/src1")),m_sharedSrc);
        m_packages.append(QDir::toNativeSeparators(file));
    }
    MerProcessStep::stdOutput(line);
}

RpmInfo::RpmInfo(const QStringList& list):
    m_list(list)
{
    QTimer::singleShot(0,this,SLOT(info()));
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
    QMessageBox::information(0, tr("Packages created"),message);
    this->deleteLater();
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////


MerDeployStepWidget::MerDeployStepWidget(MerProcessStep *step)
        : m_step(step)
{
    m_ui.setupUi(this);
    m_ui.commandArgumentsLineEdit->setText(m_step->arguments());
    connect(m_ui.commandArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(commandArgumentsLineEditTextEdited()));
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
