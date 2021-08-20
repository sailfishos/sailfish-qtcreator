/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
** Copyright (C) 2020 Open Mobile Platform LLC.
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

#include "merplugin.h"

#include "mercmakebuildconfiguration.h"
#include "merqmakebuildconfiguration.h"
#include "merbuildengineoptionspage.h"
#include "merbuildsteps.h"
#include "mercompilationdatabasebuildconfiguration.h"
#include "merconnectionmanager.h"
#include "merconstants.h"
#include "mercustomrunconfiguration.h"
#include "merdeployconfiguration.h"
#include "merdeploysteps.h"
#include "merdevicedebugsupport.h"
#include "merdevicefactory.h"
#include "meremulatordevice.h"
#include "meremulatormodedialog.h"
#include "meremulatoroptionspage.h"
#include "mergeneraloptionspage.h"
#include "merhardwaredevice.h"
#include "merqmllivebenchmanager.h"
#include "merqmlrunconfiguration.h"
#include "merqmltoolingsupport.h"
#include "merqtversion.h"
#include "merrunconfigurationaspect.h"
#include "merrunconfiguration.h"
#include "mersdkkitaspect.h"
#include "mersdkmanager.h"
#include "mersettings.h"
#include "mertoolchain.h"
#include "mervmconnectionui.h"
#include "meremulatormodeoptionspage.h"

#include <sfdk/buildengine.h>
#include <sfdk/emulator.h>
#include <sfdk/sdk.h>

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/runcontrol.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <remotelinux/remotelinuxcustomrunconfiguration.h>
#include <remotelinux/remotelinuxqmltoolingsupport.h>
#include <remotelinux/remotelinuxrunconfiguration.h>
#include <utils/checkablemessagebox.h>
#include <utils/hostosinfo.h>
#include <utils/utilsicons.h>

#include <QCheckBox>
#include <QMenu>
#include <QMessageBox>
#include <QTimer>
#include <QtPlugin>
#include <QVBoxLayout>

using namespace Core;
using namespace ExtensionSystem;
using namespace ProjectExplorer;
using namespace RemoteLinux;
using namespace Sfdk;
using Utils::CheckableMessageBox;

namespace QInstaller {

class Repository
{
public:
    explicit Repository();

    static void registerMetaType();

    friend QDataStream &operator>>(QDataStream &istream, Repository &repository);
    friend QDataStream &operator<<(QDataStream &ostream, const Repository &repository);

    QUrl m_url;
    bool m_enabled;
};

QDataStream &operator>>(QDataStream &istream, QInstaller::Repository &repository);
QDataStream &operator<<(QDataStream &ostream, const QInstaller::Repository &repository);

}

Q_DECLARE_METATYPE(QInstaller::Repository)

namespace Mer {
namespace Internal {

namespace {
const char *VM_NAME_PROPERTY = "merVmName";
}

class MerPluginPrivate
{
public:
    MerPluginPrivate()
        : sdk(Sdk::VersionedSettings | Sdk::VmAutoConnect)
    {
    }

    Sdk sdk;
    MerSdkKitAspect merSdkKitAspect;
    MerSdkManager sdkManager;
    MerConnectionManager connectionManager;
    MerBuildEngineOptionsPage buildEngineOptionsPage;
    MerEmulatorOptionsPage emulatorOptionsPage;
    MerEmulatorModeOptionsPage emulatorModeOptionsPage;
    MerGeneralOptionsPage generalOptionsPage;
    MerDeviceFactory deviceFactory;
    MerEmulatorDeviceManager emulatorDeviceManager;
    MerHardwareDeviceManager hardwareDeviceManager;
    MerQtVersionFactory qtVersionFactory;
    MerToolChainFactory toolChainFactory;
    MerRpmDeployConfigurationFactory rpmDeployConfigurationFactory;
    MerMb2RpmBuildConfigurationFactory mb2RpmBuildConfigurationFactory;
    MerRsyncDeployConfigurationFactory rsyncDeployConfigurationFactory;
    MerAddRemoveSpecialDeployStepsProjectListener addRemoveSpecialDeployStepsProjectListener;
    MerRunConfigurationFactory runConfigurationFactory;
    MerCustomRunConfigurationFactory customRunConfigurationFactory;
    MerQmlRunConfigurationFactory qmlRunConfigurationFactory;
    MerBuildStepFactory<MerSdkStartStep> sdkStartStepFactory;
    MerBuildStepFactory<MerClearBuildEnvironmentStep> clearBuildEnvironmentStepFactory;
    MerDeployStepFactory<MerPrepareTargetStep> prepareTargetStepFactory;
    MerDeployStepFactory<MerMb2MakeInstallStep> mb2RsyncBuildStepFactory;
    MerDeployStepFactory<MerMb2RsyncDeployStep> mb2RsyncDeployStepFactory;
    MerDeployStepFactory<MerMb2RpmDeployStep> mb2RpmDeployStepFactory;
    MerDeployStepFactory<MerMb2RpmBuildStep> mb2RpmBuildStepFactory;
    MerDeployStepFactory<MerRpmValidationStep> rpmValidationStepFactory;
    MerDeployStepFactory<MerLocalRsyncDeployStep> localRsyncDeployStepFactory;
    MerDeployStepFactory<MerResetAmbienceDeployStep> resetAmbienceDeployStepFactory;
    MerQmlLiveBenchManager qmlLiveBenchManager;
    MerCMakeBuildConfigurationFactory cmakeBuildConfigurationFactory;
    MerQmakeBuildConfigurationFactory qmakeBuildConfigurationFactory;
    MerCompilationDatabaseMakeStepFactory compilationDbMakeStepFactory;
    MerCompilationDatabaseBuildConfigurationFactory compilationDbBuildConfigurationFactory;

    const QList<Utils::Id> supportedRunConfigs{
        Constants::MER_CUSTOMRUNCONFIGURATION_PREFIX,
        Constants::MER_QMLRUNCONFIGURATION,
        Constants::MER_RUNCONFIGURATION_PREFIX
    };

    RunWorkerFactory runnerFactory{
        RunWorkerFactory::make<SimpleTargetRunner>(),
        {ProjectExplorer::Constants::NORMAL_RUN_MODE},
        supportedRunConfigs,
        {Constants::MER_DEVICE_TYPE}
    };
    RunWorkerFactory debuggerFactory{
        RunWorkerFactory::make<MerDeviceDebugSupport>(),
        {ProjectExplorer::Constants::DEBUG_RUN_MODE},
        supportedRunConfigs,
        {Constants::MER_DEVICE_TYPE}
    };
    RunWorkerFactory qmlToolingFactory{
        RunWorkerFactory::make<MerQmlToolingSupport>(),
        {ProjectExplorer::Constants::QML_PROFILER_RUN_MODE,
        // FIXME
        /* ProjectExplorer::Constants::QML_PREVIEW_RUN_MODE */},
        supportedRunConfigs,
        {Constants::MER_DEVICE_TYPE}
    };
};

static MerPluginPrivate *dd = nullptr;

MerPlugin::MerPlugin()
{
    BuildConfiguration::setEnvironmentWidgetExtender(addInfoOnBuildEngineEnvironment);
}

MerPlugin::~MerPlugin()
{
    delete dd, dd = nullptr;
}

bool MerPlugin::initialize(const QStringList &arguments, QString *errorString)
{
    Q_UNUSED(arguments)
    Q_UNUSED(errorString)

    using namespace ProjectExplorer::Constants;

    qputenv("SFDK_NO_SESSION", "1");

    new MerSettings(this);

    RunConfiguration::registerAspect<MerRunConfigurationAspect>();

    BuildEngine::registerVmConnectionUi<MerVmConnectionUi>();
    Emulator::registerVmConnectionUi<MerVmConnectionUi>();

    dd = new MerPluginPrivate;

    Command *emulatorConnectionCommand =
        ActionManager::command(Constants::MER_EMULATOR_CONNECTON_ACTION_ID);
    ModeManager::addAction(emulatorConnectionCommand->action(), 1);

    Command *sdkConnectionCommand =
        ActionManager::command(Constants::MER_SDK_CONNECTON_ACTION_ID);
    ModeManager::addAction(sdkConnectionCommand->action(), 1);

    ActionContainer *toolsMenu =
        ActionManager::actionContainer(Core::Constants::M_TOOLS);

    ActionContainer *menu = ActionManager::createMenu(Constants::MER_TOOLS_MENU);
    menu->menu()->setTitle("&" + Sdk::osVariant());
    toolsMenu->addMenu(menu);

    MerEmulatorModeDialog *emulatorModeDialog = new MerEmulatorModeDialog(this);
    Command *emulatorModeCommand =
        ActionManager::registerAction(emulatorModeDialog->action(),
                                            Constants::MER_EMULATOR_MODE_ACTION_ID,
                                            Context(Core::Constants::C_GLOBAL));
    menu->addAction(emulatorModeCommand);

    QAction *startQmlLiveBenchAction = new QAction(tr("Start &QmlLive Bench..."), this);
    Command *startQmlLiveBenchCommand =
        ActionManager::registerAction(startQmlLiveBenchAction,
                                      Constants::MER_START_QML_LIVE_BENCH_ACTION_ID,
                                      Context(Core::Constants::C_GLOBAL));
    connect(startQmlLiveBenchAction, &QAction::triggered,
            MerQmlLiveBenchManager::startBench);
    menu->addAction(startQmlLiveBenchCommand);

    ensureCustomRunConfigurationIsTheDefaultOnCompilationDatabaseProjects();

    ensureDailyUpdateCheckForEarlyAccess();

    return true;
}

void MerPlugin::extensionsInitialized()
{
    // Do not enable updates untils devices and kits are loaded. We know devices
    // are loaded prior to kits.
    connect(KitManager::instance(), &KitManager::kitsLoaded,
            Sdk::instance(), Sdk::enableUpdates);

    connect(ICore::instance(), &ICore::saveSettingsRequested, this, &MerPlugin::saveSettings);
}

IPlugin::ShutdownFlag MerPlugin::aboutToShutdown()
{
    m_stopList.clear();
    for (BuildEngine *const engine : Sdk::buildEngines()) {
        VirtualMachine *const virtualMachine = engine->virtualMachine();
        bool headless = false;
        if (!virtualMachine->isOff(&headless) && headless) {
            QMessageBox *prompt = new QMessageBox(
                    QMessageBox::Question,
                    tr("Close Virtual Machine"),
                    tr("The headless virtual machine \"%1\" is still running.\n\n"
                        "Close the virtual machine now?").arg(virtualMachine->name()),
                    QMessageBox::Yes | QMessageBox::No,
                    ICore::mainWindow());
            prompt->setCheckBox(new QCheckBox(CheckableMessageBox::msgDoNotAskAgain()));
            prompt->setProperty(VM_NAME_PROPERTY, virtualMachine->name());
            connect(prompt, &QMessageBox::finished,
                    this, &MerPlugin::handlePromptClosed);
            m_stopList.insert(virtualMachine->name(), virtualMachine);
            if (MerSettings::isAskBeforeClosingVmEnabled()) {
                prompt->open();
            } else {
                QTimer::singleShot(0, prompt, [prompt] { prompt->done(QMessageBox::Yes); });
            }
        }
    }

    if (m_stopList.isEmpty())
        onStopListEmpty();

    return AsynchronousShutdown;
}

void MerPlugin::saveSettings()
{
    QStringList errorStrings;
    Sdk::saveSettings(&errorStrings);
    for (const QString &errorString : errorStrings) {
        // See Utils::PersistentSettingsWriter::save()
        QMessageBox::critical(ICore::dialogParent(),
                QCoreApplication::translate("Utils::FileSaverBase", "File Error"),
                errorString);
    }
}

MerBuildEngineOptionsPage *MerPlugin::buildEngineOptionsPage()
{
    return &dd->buildEngineOptionsPage;
}

MerEmulatorOptionsPage *MerPlugin::emulatorOptionsPage()
{
    return &dd->emulatorOptionsPage;
}

// FIXME VM info is not immediately available
//
// VM info initialization is only started during Sdk::enableUpdates and it is
// done asynchronously, so it still takes some time after Sdk::enableUpdates
// returns before the info becomes available.  It is not clear yet how to fix it
// properly and at the same time it is known to only become an issue when
// Options dialog is opened very soon (<3 secs) after application startup,
// hence this workaround.
void MerPlugin::workaround_ensureVirtualMachinesAreInitialized()
{
    // We do Sdk::enableUpdates on KitManager::kitsLoaded
    QTC_ASSERT(KitManager::isLoaded(), return);

    static bool first = true;
    if (!first)
        return;
    first = false;

    QList<VirtualMachineDescriptor> unusedVms;
    bool ok;
    execAsynchronous(std::tie(unusedVms, ok), Sdk::unusedVirtualMachines);
    QTC_CHECK(ok);
}

void MerPlugin::handlePromptClosed(int result)
{
    QMessageBox *prompt = qobject_cast<QMessageBox *>(sender());
    prompt->deleteLater();

    QString vm = prompt->property(VM_NAME_PROPERTY).toString();

    if (prompt->checkBox() && prompt->checkBox()->isChecked())
        MerSettings::setAskBeforeClosingVmEnabled(false);

    if (result == QMessageBox::Yes) {
        VirtualMachine *virtualMachine = m_stopList.value(vm);
        virtualMachine->lockDown(true, this, [=](bool ok) {
            Q_UNUSED(ok);
            m_stopList.remove(virtualMachine->name());
            if (m_stopList.isEmpty())
                onStopListEmpty();
        });
    } else {
        m_stopList.remove(vm);
    }

    if (m_stopList.isEmpty())
        onStopListEmpty();
}

void MerPlugin::onStopListEmpty()
{
    dd->sdk.shutDown(this, [=]() { emit asynchronousShutdownFinished(); });
}

void MerPlugin::addInfoOnBuildEngineEnvironment(QVBoxLayout *vbox)
{
    QWidget *const parent = vbox->parentWidget();

    auto filterInfoHBox = new QHBoxLayout;
    filterInfoHBox->setSpacing(6);

    QLabel *filterInfoIcon = new QLabel(parent);
    filterInfoIcon->setPixmap(Utils::Icons::INFO.pixmap());
    filterInfoHBox->addWidget(filterInfoIcon);

    QLabel *filterInfoLabel = new QLabel(parent);
    filterInfoLabel->setWordWrap(true);
    filterInfoLabel->setText(tr("Not all variables will be forwarded to a %1 build engine. "
                "<a href='environment-filter'>Configure…</a>").arg(Sdk::osVariant()));
    filterInfoLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);
    connect(filterInfoLabel, &QLabel::linkActivated, [] {
        Core::ICore::showOptionsDialog(Mer::Constants::MER_GENERAL_OPTIONS_ID, Core::ICore::dialogParent());
    });
    filterInfoHBox->addWidget(filterInfoLabel, 1);

    vbox->insertItem(0, filterInfoHBox);
}

void MerPlugin::ensureCustomRunConfigurationIsTheDefaultOnCompilationDatabaseProjects()
{
    // Hack. By default the ProjectExplorer::CustomExecutableRunConfiguration is activated
    connect(SessionManager::instance(), &SessionManager::projectAdded, [](Project *project) {
        if (project->id() != CompilationDatabaseProjectManager::Constants::COMPILATIONDATABASEPROJECT_ID)
            return;
        connect(project, &Project::addedTarget, [](Target *target) {
            if (!target->activeRunConfiguration())
                return;
            if (qobject_cast<MerCustomRunConfiguration *>(target->activeRunConfiguration()))
                return;
            for (RunConfiguration *const rc : target->runConfigurations()) {
                if (qobject_cast<MerCustomRunConfiguration *>(rc)) {
                    target->setActiveRunConfiguration(rc);
                    break;
                }
            }
        });
    });
}

void MerPlugin::ensureDailyUpdateCheckForEarlyAccess()
{
    QInstaller::Repository::registerMetaType();
    QDir iniDir = QCoreApplication::applicationDirPath();
    iniDir.cdUp();
    if (Utils::HostOsInfo::isMacHost()) {
        iniDir.cdUp();
        iniDir.cdUp();
        iniDir.cdUp();
    }
    QSettings settings(iniDir.filePath("SDKMaintenanceTool.ini"), QSettings::IniFormat);
    const QVariantList variants = settings.value(QLatin1String("DefaultRepositories"))
        .toList();
    bool eaRepoFound = false;
    foreach (const QVariant &variant, variants) {
        QInstaller::Repository repo = variant.value<QInstaller::Repository>();
        if (repo.m_url.toString().endsWith("updates-ea")) {
            eaRepoFound = true;
            if (repo.m_enabled) {
                QSettings *settings = ICore::settings();
                settings->beginGroup(QLatin1String("Updater"));
                settings->setValue(QLatin1String("CheckUpdateInterval"), QLatin1String("DailyCheck"));
                settings->endGroup();
            }
        }
    }
    QTC_CHECK(eaRepoFound);
}
} // Internal
} // Mer

namespace QInstaller {

Repository::Repository()
    : m_enabled(false)
{
    registerMetaType();
}

void Repository::registerMetaType()
{
    qRegisterMetaType<Repository>("Repository");
    qRegisterMetaTypeStreamOperators<Repository>("Repository");
}

QDataStream &operator>>(QDataStream &istream, QInstaller::Repository &repository)
{
    QByteArray url, username, password, displayname, compressed, categoryname;
    bool defaultRepository;
    istream >> url >> defaultRepository >> repository.m_enabled >> username >> password
            >> displayname >> categoryname;
    repository.m_url = QUrl::fromEncoded(QByteArray::fromBase64(url));
    return istream;
}

QDataStream &operator<<(QDataStream &ostream, const QInstaller::Repository &repository)
{
    Q_UNUSED(repository);
    return ostream;
}

}
