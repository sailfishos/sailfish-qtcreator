/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#ifndef MERCONSTANTS_H
#define MERCONSTANTS_H

#include <qglobal.h>

namespace Mer {
namespace Constants {

const char MER_QT[] = "QmakeProjectManager.QtVersion.Mer";
const char MER_TOOLCHAIN_ID[] = "QmakeProjectManager.ToolChain.Mer";

const char MER_OPTIONS_CATEGORY[] = "W.Mer";
const char MER_OPTIONS_CATEGORY_TR[] = QT_TRANSLATE_NOOP("Mer", "Sailfish OS");
const char MER_OPTIONS_ID[] = "A.Mer";
const char MER_OPTIONS_NAME[] = QT_TRANSLATE_NOOP("Mer", "Build Engine");
const char MER_GENERAL_OPTIONS_ID[] = "A.MerGeneral";
const char MER_GENERAL_OPTIONS_NAME[] = QT_TRANSLATE_NOOP("Mer", "General");
const char MER_EMULATOR_MODE_OPTIONS_ID[] = "A.MerEmulatorMode";
const char MER_EMULATOR_MODE_OPTIONS_NAME[] = QT_TRANSLATE_NOOP("Mer", "Emulator Modes");
const char MER_EMULATOR_MODE_ACTION_NAME[] = QT_TRANSLATE_NOOP("Mer", "&Emulator mode...");

const char MER_DEVICE_TYPE[] = "Mer.Device.Type";

const char MER_TOOLS_MENU[] = "Mer.Tools.Menu";
const char MER_EMULATOR_MODE_ACTION_ID[] = "Mer.Emulator.Mode.Action";
const char MER_START_QML_LIVE_BENCH_ACTION_ID[] = "Mer.StartQmlLiveBench.Action";

const char MER_DEVICE_DEFAULTUSER[] = "nemo";
const char MER_DEVICE_ROOTUSER[] = "root";

const char MER_SDK_PROXY_DISABLED[] = "direct";
const char MER_SDK_PROXY_AUTOMATIC[] = "auto";
const char MER_SDK_PROXY_MANUAL[] = "manual";

// Keys used for ini files
const char MER_SDK_TOOLS[] = "/mer-sdk-tools/";
const char MER_SDK_DATA_KEY[] = "MerSDK.";
const char MER_TARGET_KEY[] = "MerTarget.Target";
const char MER_TARGET_DATA_KEY[] = "MerTarget.";
const char MER_SDK_COUNT_KEY[] = "MerSDK.Count";
const char MER_TARGET_COUNT_KEY[] = "MerTarget.Count";
const char MER_SDK_FILE_VERSION_KEY[] = "MerSDK.Version";
const char MER_TARGET_FILE_VERSION_KEY[] = "MerTarget.Version";
const char MER_SDK_INSTALLDIR[] = "MerSDK.InstallDir";
const char MER_DEVICE_MODELS_FILE_VERSION_KEY[] = "MerDeviceModels.Version";
const char MER_DEVICE_MODELS_COUNT_KEY[] = "MerDeviceModels.Count";
const char MER_DEVICE_MODELS_DATA_KEY[] = "MerDeviceModels.";
const char MER_DEVICE_MODEL_NAME[] = "MerDeviceModel.Name";
const char MER_DEVICE_MODEL_DISPLAY_RESOLUTION[] = "MerDeviceModel.DisplayResolution";
const char MER_DEVICE_MODEL_DISPLAY_SIZE[] = "MerDeviceModel.DisplaySize";
const char MER_DEVICE_MODEL_DCONF_DB[] = "MerDeviceModel.DconfDb";
const char VM_NAME[] = "VirtualMachineName";
const char SHARED_HOME[] = "SharedHome";
const char SHARED_TARGET[] = "SharedTarget";
const char SHARED_CONFIG[] = "SharedConfig";
const char SHARED_SRC[] = "SharedSrc";
const char SHARED_SSH[] = "SharedSsh";
const char AUTHENTICATION_TYPE[] = "AuthenticationType";
const char HOST[] = "Host";
const char USERNAME[] = "Username";
const char KEY[] = "Key";
const char PRIVATE_KEY_FILE[] = "PrivateKeyFile";
const char SSH_PORT[] = "SshPort";
const char SSH_TIMEOUT[] = "SshTimeout";
const char WWW_PORT[] = "WwwPort";
const char WWW_PROXY[] = "WwwProxy";
const char WWW_PROXY_SERVERS[] = "WwwProxyServers";
const char WWW_PROXY_EXCLUDES[] = "WwwProxyExcludes";
const char HEADLESS[] = "Headless";
const char MEMORY_SIZE_MB[] = "MemorySize";
const char CPU_COUNT[] = "CpuCount";
const char VDI_CAPACITY_MB[] = "VdiCapacity";
const char ID[] = "Id";
const char NAME[] = "Name";
const char KITS[] = "Kits";
const char SYSROOT[] = "Sysroot";
const char TOOL_CHAINS[] = "ToolChains";
const char QT_VERSIONS[] = "QtVersions";
const char AUTO_DETECTED[] = "AutoDetected";
const char BUILD_ENGINE_URI[] = "BuildEngineUri";
const char SB2_TARGET_NAME[] = "SB2.TargetName";

const char MER_RUNCONFIGURATION_PREFIX[] = "QmakeProjectManager.MerRunConfiguration:";
const char MER_QMLRUNCONFIGURATION[] = "QmakeProjectManager.MerQmlRunConfiguration";
const char SAILFISH_QML_LAUNCHER[] = "/usr/bin/sailfish-qml";

const char MER_EMULATOR_CONNECTON_ACTION_ID[] = "Mer.EmulatorConnecitonAction";
const char MER_SDK_CONNECTON_ACTION_ID[] = "Mer.SdkConnectionAction";

const char MER_WIZARD_FEATURE_SAILFISHOS[] = "Mer.Wizard.Feature.SailfishOS";
const char MER_WIZARD_FEATURE_EMULATOR[] = "Mer.Wizard.Feature.Emulator";

const char MER_DEVICES_FILENAME[] = "/devices.xml";
const char MER_DEVICE_MODELS_FILENAME[] = "/qtcreator/mersdk-device-models.xml";
const char MER_COMPOSITOR_CONFIG_FILENAME[] = "65-emul-wayland-ui-scale.conf";
const char MER_EMULATOR_DCONF_DB_FILENAME[] = "device-model.txt";

const char MER_SDK_SHARED_HOME_MOUNT_POINT[] = "/home/mersdk/share";
const char MER_SDK_SHARED_SRC_MOUNT_POINT[] = "/home/src1";

const char MER_AUTHORIZEDKEYS_FOLDER[] = "authorized_keys";

const char SAILFISH_OS_SDK_ENVIRONMENT_FILTER_DEPRECATED[] = "SAILFISH_OS_SDK_ENVIRONMENT_FILTER";
const char SAILFISH_SDK_ENVIRONMENT_FILTER[] = "SAILFISH_SDK_ENVIRONMENT_FILTER";
const char SAILFISH_SDK_FRONTEND[] = "SAILFISH_SDK_FRONTEND";
const char SAILFISH_SDK_FRONTEND_ID[] = "qtcreator";
const char MER_DEVICE_VIRTUAL_MACHINE_URI[] = "MER_DEVICE_VIRTUAL_MACHINE_URI";
const char MER_DEVICE_FACTORY_SNAPSHOT[] = "MER_DEVICE_FACTORY_SNAPSHOT";
const char MER_DEVICE_MAC[] = "MER_DEVICE_MAC";
const char MER_DEVICE_SUBNET[] = "MER_DEVICE_SUBNET";
const char MER_DEVICE_SHARED_SSH[] = "MER_DEVICE_SHARED_SSH";
const char MER_DEVICE_QML_LIVE_PORTS[] = "MER_DEVICE_QML_LIVE_PORTS";
const char MER_DEVICE_SHARED_CONFIG[]= "MER_DEVICE_SHARED_CONFIG";
const char MER_DEVICE_ARCHITECTURE[]= "MER_DEVICE_ARCHITECTURE";
const char MER_DEVICE_DEVICE_MODEL[]= "MER_DEVICE_DEVICE_MODEL";
const char MER_DEVICE_ORIENTATION[]= "MER_DEVICE_ORIENTATION";
const char MER_DEVICE_VIEW_SCALED[]= "MER_DEVICE_VIEW_SCALED";

const char MER_RUN_CONFIGURATION_ASPECT[] = "Mer.RunConfigurationAspect";

const char QML_LIVE_HELP_URL[] = "qthelp://org.qt-project.qtcreator/doc/creator-qtquick-qmllive-sailfish.html";

} // namespace Constants
} // namespace Mer

#endif // MERCONSTANTS_H
