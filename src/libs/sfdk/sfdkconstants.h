/****************************************************************************
**
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

#pragma once

namespace Sfdk {
namespace Constants {

const char LIB_ID[] = "libsfdk";

const int DEFAULT_QML_LIVE_PORT = 10234;
const int MAX_PORT_LIST_PORTS = 10;

const char VIRTUAL_MACHINE_URI_SCHEME[] = "sfdkvm";

const char BUILD_ENGINES_SETTINGS_FILE_NAME[] = "buildengines.xml";
// "QtCreator" prefix is enforced by sdktool
const char BUILD_ENGINES_SETTINGS_DOC_TYPE[] = "QtCreatorSfdkBuildEngines";
const char BUILD_ENGINES_VERSION_KEY[] = "BuildEngines.Version";
const char BUILD_ENGINES_INSTALL_DIR_KEY[] = "BuildEngines.InstallDir";
const char BUILD_ENGINES_COUNT_KEY[] = "BuildEngines.Count";
const char BUILD_ENGINES_DATA_KEY_PREFIX[] = "BuildEngine.";
const char BUILD_ENGINE_VM_URI[] = "VirtualMachineUri";
const char BUILD_ENGINE_AUTODETECTED[] = "Autodetected";
const char BUILD_ENGINE_SHARED_HOME[] = "SharedHome";
const char BUILD_ENGINE_SHARED_TARGET[] = "SharedTarget";
const char BUILD_ENGINE_SHARED_CONFIG[] = "SharedConfig";
const char BUILD_ENGINE_SHARED_SRC[] = "SharedSrc";
const char BUILD_ENGINE_SHARED_SSH[] = "SharedSsh";
const char BUILD_ENGINE_HOST[] = "Host";
const char BUILD_ENGINE_USER_NAME[] = "UserName";
const char BUILD_ENGINE_PRIVATE_KEY_FILE[] = "PrivateKeyFile";
const char BUILD_ENGINE_SSH_PORT[] = "SshPort";
const char BUILD_ENGINE_SSH_TIMEOUT[] = "SshTimeout";
const char BUILD_ENGINE_WWW_PORT[] = "WwwPort";
const char BUILD_ENGINE_WWW_PROXY_TYPE[] = "WwwProxyType";
const char BUILD_ENGINE_WWW_PROXY_SERVERS[] = "WwwProxyServers";
const char BUILD_ENGINE_WWW_PROXY_EXCLUDES[] = "WwwProxyExcludes";
const char BUILD_ENGINE_HEADLESS[] = "Headless";
const char BUILD_ENGINE_TARGETS_COUNT_KEY[] = "BuildTargets.Count";
const char BUILD_ENGINE_TARGET_DATA_KEY_PREFIX[] = "BuildTarget.";

const char BUILD_TARGET_NAME[] = "Name";
const char BUILD_TARGET_GCC_DUMP_MACHINE[] = "GccDumpMachine";
const char BUILD_TARGET_GCC_DUMP_MACROS[] = "GccDumpMacros";
const char BUILD_TARGET_GCC_DUMP_INCLUDES[] = "GccDumpIncludes";
const char BUILD_TARGET_QMAKE_QUERY[] = "QmakeQuery";
const char BUILD_TARGET_RPM_VALIDATION_SUITES[] = "RpmValidationSuites";

#ifdef Q_OS_WIN
#define SCRIPT_EXTENSION ".cmd"
#else
#define SCRIPT_EXTENSION ""
#endif

const char BUILD_TARGET_TOOLS[] = "build-target-tools";
const char WRAPPER_QMAKE[] = "qmake" SCRIPT_EXTENSION;
const char WRAPPER_MAKE[] = "make" SCRIPT_EXTENSION;
const char WRAPPER_GCC[] = "gcc" SCRIPT_EXTENSION;
const char WRAPPER_DEPLOY[] = "deploy" SCRIPT_EXTENSION;
const char WRAPPER_RPM[] = "rpm" SCRIPT_EXTENSION;
const char WRAPPER_RPMVALIDATION[] = "rpmvalidation" SCRIPT_EXTENSION;
const char WRAPPER_LUPDATE[] = "lupdate" SCRIPT_EXTENSION;
const char WRAPPER_PKG_CONFIG[] = "pkg-config" SCRIPT_EXTENSION;
const char QMAKE_QUERY_CACHE[] = "qmake.query";
const char GCC_DUMP_MACHINE_CACHE[] = "gcc.dumpmachine";
const char GCC_DUMP_MACROS_CACHE[] = "gcc.dumpmacros";
const char GCC_DUMP_INCLUDES_CACHE[] = "gcc.dumpincludes";

#undef SCRIPT_EXTENSION

// FIXME
const char MER_SSH_DEVICE_NAME[] = "MER_SSH_DEVICE_NAME";
const char MER_SSH_ENGINE_NAME[] = "MER_SSH_ENGINE_NAME";
const char MER_SSH_PORT[] = "MER_SSH_PORT";
const char MER_SSH_PRIVATE_KEY[] = "MER_SSH_PRIVATE_KEY";
const char MER_SSH_SDK_TOOLS[] = "MER_SSH_SDK_TOOLS";
const char MER_SSH_SHARED_HOME[] = "MER_SSH_SHARED_HOME";
const char MER_SSH_SHARED_SRC[] = "MER_SSH_SHARED_SRC";
const char MER_SSH_SHARED_TARGET[] = "MER_SSH_SHARED_TARGET";
const char MER_SSH_TARGET_NAME[] = "MER_SSH_TARGET_NAME";
const char MER_SSH_USERNAME[] = "MER_SSH_USERNAME";

const char WWW_PROXY_DISABLED[] = "direct";
const char WWW_PROXY_AUTOMATIC[] = "auto";
const char WWW_PROXY_MANUAL[] = "manual";

const char i486_IDENTIFIER[] = "i486";
const char ARM_IDENTIFIER[] = "arm";

const char DEFAULT_DEBUGGER_i486_FILENAME[] = "gdb-i486-meego-linux-gnu";
const char DEFAULT_DEBUGGER_ARM_FILENAME[] = "gdb-armv7hl-meego-linux-gnueabi";
const char DEBUGGER_FILENAME_PREFIX[] = "gdb-";

const char BUILD_ENGINE_DEFAULT_HOST[] = "localhost";
const char BUILD_ENGINE_DEFAULT_USER_NAME[] = "mersdk";
const int BUILD_ENGINE_DEFAULT_SSH_TIMEOUT = 30;

const char EMULATORS_SETTINGS_FILE_NAME[] = "emulators.xml";
// "QtCreator" prefix is enforced by sdktool
const char EMULATORS_SETTINGS_DOC_TYPE[] = "QtCreatorSfdkEmulators";
const char EMULATORS_VERSION_KEY[] = "Emulators.Version";
const char EMULATORS_INSTALL_DIR_KEY[] = "Emulators.InstallDir";
const char EMULATORS_COUNT_KEY[] = "Emulators.Count";
const char EMULATORS_DATA_KEY_PREFIX[] = "Emulator.";
const char EMULATOR_VM_URI[] = "VirtualMachineUri";
const char EMULATOR_AUTODETECTED[] = "Autodetected";
const char EMULATOR_SHARED_CONFIG[] = "SharedConfig";
const char EMULATOR_SHARED_SSH[] = "SharedSsh";
const char EMULATOR_HOST[] = "Host";
const char EMULATOR_USER_NAME[] = "UserName";
const char EMULATOR_PRIVATE_KEY_FILE[] = "PrivateKeyFile";
const char EMULATOR_SSH_PORT[] = "SshPort";
const char EMULATOR_SSH_TIMEOUT[] = "SshTimeout";
const char EMULATOR_FREE_PORTS[] = "FreePorts";
const char EMULATOR_QML_LIVE_PORTS[] = "QmlLivePorts";
const char EMULATOR_FACTORY_SNAPSHOT[] = "FactorySnapshot";
const char EMULATOR_MAC[] = "Mac";
const char EMULATOR_SUBNET[] = "Subnet";
const char EMULATOR_DEVICE_MODEL[] = "DeviceModel";
const char EMULATOR_ORIENTATION[] = "Orientation";
const char EMULATOR_VIEW_SCALED[] = "ViewScaled";

const char DEVICE_MODELS_COUNT_KEY[] = "DeviceModels.Count";
const char DEVICE_MODELS_DATA_KEY_PREFIX[] = "DeviceModel.";
const char DEVICE_MODEL_NAME[] = "Name";
const char DEVICE_MODEL_AUTODETECTED[] = "Autodetected";
const char DEVICE_MODEL_DISPLAY_RESOLUTION[] = "DisplayResolution";
const char DEVICE_MODEL_DISPLAY_SIZE[] = "DisplaySize";
const char DEVICE_MODEL_DCONF[] = "Dconf";

const char EMULATOR_DEFAULT_HOST[] = "localhost";
const char EMULATOR_DEFAULT_USER_NAME[] = "nemo";
const int EMULATOR_DEFAULT_SSH_TIMEOUT = 30;

const char VIRTUAL_BOX_VIRTUAL_SUBNET[] = "10.220.220";

const char EMULATOR_COMPOSITOR_CONFIG_FILENAME[] = "65-emul-wayland-ui-scale.conf";
const char EMULATOR_DCONF_DB_FILENAME[] = "device-model.txt";

} // namespace Constants
} // namespace Sfdk
