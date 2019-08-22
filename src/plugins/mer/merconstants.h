/****************************************************************************
**
** Copyright (C) 2012-2019 Jolla Ltd.
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

#include <qglobal.h> // for Q_OS_*

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
const char MER_i486_IDENTIFIER[] = "i486";
const char MER_ARM_IDENTIFIER[] = "arm";
const char MER_SAILFISH_MENU[] = "Mer.Sailfish.Menu";
const char MER_SAILFISH_START_MENU[] = "Mer.Sailfish.Start.Menu";
const char MER_SAILFISH_STOP_MENU[] = "Mer.Sailfish.Stop.Menu";
const char MER_SAILFISH_VM_GROUP_MENU[] = "Mer.Sailfish.Vm.Group.Menu";
const char MER_SAILFISH_OTHER_GROUP_MENU[] = "Mer.Sailfish.Other.Group.Menu";

const char MER_TOOLS_MENU[] = "Mer.Tools.Menu";
const char MER_EMULATOR_MODE_ACTION_ID[] = "Mer.Emulator.Mode.Action";
const char MER_START_QML_LIVE_BENCH_ACTION_ID[] = "Mer.StartQmlLiveBench.Action";

#ifdef Q_OS_WIN
#define SCRIPT_EXTENSION ".cmd"
#else // Q_OS_WIN
#define SCRIPT_EXTENSION ""
#endif // Q_OS_WIN

const char MER_WRAPPER_RPMBUILD[] = "rpmbuild" SCRIPT_EXTENSION;
const char MER_WRAPPER_QMAKE[] = "qmake" SCRIPT_EXTENSION;
const char MER_WRAPPER_MAKE[] = "make" SCRIPT_EXTENSION;
const char MER_WRAPPER_GCC[] = "gcc" SCRIPT_EXTENSION;
const char MER_WRAPPER_DEPLOY[] = "deploy" SCRIPT_EXTENSION;
const char MER_WRAPPER_RPM[] = "rpm" SCRIPT_EXTENSION;
const char MER_WRAPPER_RPMVALIDATION[] = "rpmvalidation" SCRIPT_EXTENSION;
const char MER_WRAPPER_LUPDATE[] = "lupdate" SCRIPT_EXTENSION;
const char MER_WRAPPER_PKG_CONFIG[] = "pkg-config" SCRIPT_EXTENSION;

const char MER_SDK_DEFAULTUSER[] = "mersdk";
const char MER_DEVICE_DEFAULTUSER[] = "nemo";
const char MER_DEVICE_ROOTUSER[] = "root";
const char MER_SDK_DEFAULTHOST[] = "localhost";
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
const char VIRTUAL_MACHINE[] = "VirtualMachine";
const char SB2_TARGET_NAME[] = "SB2.TargetName";

const char MER_RUNCONFIGURATION_PREFIX[] = "QmakeProjectManager.MerRunConfiguration:";
const char MER_QMLRUNCONFIGURATION[] = "QmakeProjectManager.MerQmlRunConfiguration";
const char SAILFISH_QML_LAUNCHER[] = "/usr/bin/sailfish-qml";

const char MER_EMULATOR_CONNECTON_ACTION_ID[] = "Mer.EmulatorConnecitonAction";
const char MER_SDK_CONNECTON_ACTION_ID[] = "Mer.SdkConnectionAction";

const char MER_WIZARD_FEATURE_SAILFISHOS[] = "Mer.Wizard.Feature.SailfishOS";
const char MER_WIZARD_FEATURE_EMULATOR[] = "Mer.Wizard.Feature.Emulator";

const char MER_SDK_FILENAME[] = "/qtcreator/mersdk.xml";
const char MER_TARGETS_FILENAME[] = "/targets.xml";
const char MER_DEVICES_FILENAME[] = "/devices.xml";
const char MER_DEVICE_MODELS_FILENAME[] = "/qtcreator/mersdk-device-models.xml";
const char MER_COMPOSITOR_CONFIG_FILENAME[] = "65-emul-wayland-ui-scale.conf";
const char MER_EMULATOR_DCONF_DB_FILENAME[] = "device-model.txt";
const char MER_DEBUGGER_i486_FILENAME[] = "gdb-i486-meego-linux-gnu";
const char MER_DEBUGGER_ARM_FILENAME[] = "gdb-armv7hl-meego-linux-gnueabi";
const char MER_DEBUGGER_FILENAME_PREFIX[] = "gdb-";

const char MER_SDK_SHARED_HOME_MOUNT_POINT[] = "/home/mersdk/share";
const char MER_SDK_SHARED_SRC_MOUNT_POINT[] = "/home/src1";

const char MER_TARGET_NAME[] = "MerTarget.Name";
const char MER_TARGET_QMAKE_DUMP[] = "MerTarget.QmakeQuery";
const char MER_TARGET_GCC_DUMP_MACHINE[] = "MerTarget.GccDumpMachine";
const char MER_TARGET_GCC_DUMP_MACROS[] = "MerTarget.GccDumpMacros";
const char MER_TARGET_GCC_DUMP_INCLUDES[] = "MerTarget.GccDumpIncludes";
const char MER_TARGET_RPMVALIDATION_DUMP[] = "MerTarget.RpmValidationSuites";
const char MER_TARGET_DEFAULT_DEBUGGER[] = "MerTarget.DefaultDebugger";

const char QMAKE_QUERY[] = "qmake.query";
const char QMAKE_VERSION[] = "qmake.version";
const char GCC_DUMPMACHINE[] = "gcc.dumpmachine";
const char GCC_DUMPVERSION[] = "gcc.dumpversion";
const char GCC_DUMPMACROS[] = "gcc.dumpmacros";
const char GCC_DUMPINCLUDES[] = "gcc.dumpincludes";

const char MER_AUTHORIZEDKEYS_FOLDER[] = "authorized_keys";

const char MER_SSH_SHARED_HOME[] = "MER_SSH_SHARED_HOME";
const char MER_SSH_SHARED_TARGET[] = "MER_SSH_SHARED_TARGET";
const char MER_SSH_SHARED_SRC[] = "MER_SSH_SHARED_SRC";
const char MER_SSH_SDK_TOOLS[] = "MER_SSH_SDK_TOOLS";
const char MER_SSH_USERNAME[] = "MER_SSH_USERNAME";
const char MER_SSH_PRIVATE_KEY[] = "MER_SSH_PRIVATE_KEY";
const char MER_SSH_TARGET_NAME[] = "MER_SSH_TARGET_NAME";
const char MER_SSH_DEVICE_NAME[] = "MER_SSH_DEVICE_NAME";
const char MER_SSH_PORT[] = "MER_SSH_PORT";
const char MER_SSH_ENGINE_NAME[] = "MER_SSH_ENGINE_NAME";
const char SAILFISH_OS_SDK_ENVIRONMENT_FILTER_DEPRECATED[] = "SAILFISH_OS_SDK_ENVIRONMENT_FILTER";
const char SAILFISH_SDK_ENVIRONMENT_FILTER[] = "SAILFISH_SDK_ENVIRONMENT_FILTER";
const char SAILFISH_SDK_FRONTEND[] = "SAILFISH_SDK_FRONTEND";
const char SAILFISH_SDK_FRONTEND_ID[] = "qtcreator";
const char MER_DEVICE_VIRTUAL_MACHINE[] = "MER_DEVICE_VIRTUAL_MACHINE";
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
