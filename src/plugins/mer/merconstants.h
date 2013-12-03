/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

const char MER_QT[] = "Qt4ProjectManager.QtVersion.Mer";
const char MER_TOOLCHAIN_ID[] = "Qt4ProjectManager.ToolChain.Mer";

const char MER_PLATFORM[] = "Mer";
const char MER_PLATFORM_TR[] = QT_TRANSLATE_NOOP("QtSupport", "Mer");
const char MER_TOOLCHAIN[] = "Mer GCC";
const char MER_TOOLCHAIN_TYPE[] = "Mer.Gcc";

const char MER_OPTIONS_CATEGORY[] = "W.Mer";
const char MER_OPTIONS_CATEGORY_TR[] = QT_TRANSLATE_NOOP("Mer", "Mer");
const char MER_OPTIONS_CATEGORY_ICON[] = ":/mer/images/icon-s-sailfish-qtcreator.png";
const char MER_OPTIONS_ID[] = "A.Mer";
const char MER_OPTIONS_NAME[] = QT_TRANSLATE_NOOP("Mer", "SDK");

const char MER_TASKHUB_EMULATOR_CATEGORY[] = "Qt4ProjectManager.TaskHub.Emulator.Mer";
const char MER_TASKHUB_SDK_CATEGORY[] = "Qt4ProjectManager.TaskHub.Sdk.Mer";
const char MER_DEVICE_TYPE_I486[] = "Mer.Device.Type.i486";
const char MER_DEVICE_TYPE_ARM[] = "Mer.Device.Type.Arm";

const char MER_SAILFISH_MENU[] = "Mer.Sailfish.Menu";
const char MER_SAILFISH_START_MENU[] = "Mer.Sailfish.Start.Menu";
const char MER_SAILFISH_STOP_MENU[] = "Mer.Sailfish.Stop.Menu";
const char MER_SAILFISH_VM_GROUP_MENU[] = "Mer.Sailfish.Vm.Group.Menu";
const char MER_SAILFISH_OTHER_GROUP_MENU[] = "Mer.Sailfish.Other.Group.Menu";
const char MER_SAILFISH_START_ICON[] = ":/mer/images/sdk-run.png";
const char MER_SAILFISH_STOP_ICON[] = ":/mer/images/sdk-stop.png";

#ifdef Q_OS_WIN
#define SCRIPT_EXTENSION ".cmd"
#else // Q_OS_WIN
#define SCRIPT_EXTENSION ""
#endif // Q_OS_WIN

const char MER_WRAPPER_RPMBUILD[] = "rpmbuild"SCRIPT_EXTENSION;
const char MER_WRAPPER_QMAKE[] = "qmake"SCRIPT_EXTENSION;
const char MER_WRAPPER_MAKE[] = "make"SCRIPT_EXTENSION;
const char MER_WRAPPER_GCC[] = "gcc"SCRIPT_EXTENSION;
const char MER_WRAPPER_GDB[] = "gdb"SCRIPT_EXTENSION;
const char MER_WRAPPER_DEPLOY[] = "deploy"SCRIPT_EXTENSION;
const char MER_WRAPPER_RPM[] = "rpm"SCRIPT_EXTENSION;
const char MER_WRAPPER_RM[] = "rm"SCRIPT_EXTENSION;
const char MER_WRAPPER_MV[] = "mv"SCRIPT_EXTENSION;

const char MER_SDK_DEFAULTUSER[] = "mersdk";
const char MER_DEVICE_DEFAULTUSER[] = "nemo";
const char MER_DEVICE_ROOTUSER[] = "root";
const char MER_SDK_DEFAULTHOST[] = "localhost";

const char MER_EXECUTIONTYPE_STANDARD[] = "standard";
const char MER_EXECUTIONTYPE_SB2[] = "sb2";
const char MER_EXECUTIONTYPE_MB2[] = "mb2";

const char MERSSH_PARAMETER_SDKTOOLSDIR[] = "-sdktoolsdir";
const char MERSSH_PARAMETER_MERTARGET[] = "-mertarget";
const char MERSSH_PARAMETER_COMMANDTYPE[] = "-commandtype";

// Keys used for ini files
const char MER_SDK[] = "MerSDK";
const char MER_SDK_TOOLS[] = "/mer-sdk-tools/";
const char MER_SDK_DATA_KEY[] = "MerSDK.";
const char MER_TARGET_KEY[] = "MerTarget.Target";
const char MER_TARGET_DATA_KEY[] = "MerTarget.";
const char MER_SDK_COUNT_KEY[] = "MerSDK.Count";
const char MER_TARGET_COUNT_KEY[] = "MerTarget.Count";
const char MER_SDK_FILE_VERSION_KEY[] = "MerSDK.Version";
const char MER_TARGET_FILE_VERSION_KEY[] = "MerTarget.Version";
const char VM_NAME[] = "VirtualMachineName";
const char SHARED_HOME[] = "SharedHome";
const char SHARED_TARGET[] = "SharedTarget";
const char SHARED_CONFIG[] = "SharedConfig";
const char SHARED_SRC[] = "SharedSrc";
const char SHARED_SSH[] = "SharedSsh";
const char AUTHENTICATION_TYPE[] = "AuthenticationType";
const char HOST[] = "Host";
const char USERNAME[] = "Username";
const char PASSWORD[] = "Password";
const char KEY[] = "Key";
const char PRIVATE_KEY_FILE[] = "PrivateKeyFile";
const char SSH_PORT[] = "SshPort";
const char WWW_PORT[] = "WwwPort";
const char HEADLESS[] = "Headless";
const char ID[] = "Id";
const char NAME[] = "Name";
const char KITS[] = "Kits";
const char SYSROOT[] = "Sysroot";
const char TOOL_CHAINS[] = "ToolChains";
const char QT_VERSIONS[] = "QtVersions";
const char AUTO_DETECTED[] = "AutoDetected";
const char TARGET[] = "Target";
const char VIRTUAL_MACHINE[] = "VirtualMachine";
const char SB2_TARGET_NAME[] = "SB2.TargetName";

const char MER_RUNCONFIGURATION_PREFIX[] = "Qt4ProjectManager.MerRunConfiguration:";
const char MER_SFTP_DEPLOY_STRING[] = QT_TRANSLATE_NOOP("Mer", "Copy Files to Device");

const char MER_EMULATOR_START_ACTION_ID[] = "Mer.MerEmulatorStartAction";
const char MER_EMULATOR_DEPLOYKEY_ACTION_ID[] = "Mer.MerEmulatorDeployAction";
const char MER_HARDWARE_DEPLOYKEY_ACTION_ID[] = "Mer.MerHardwareDeployAction";
const char MER_EMULATOR_TEST_ID[] = "Mer.MerEmulatorTestAction";
const char MER_HARDWARE_TEST_ID[] = "Mer.MerHardwareTestAction";
const char MER_EMULATOR_CONNECTON_ACTION_ID[] = "Mer.EmulatorConnecitonAction";
const char MER_SDK_CONNECTON_ACTION_ID[] = "Mer.SdkConnectionAction";

const char MER_WIZARD_FEATURE_SAILFISHOS[] = "Mer.Wizard.Feature.SailfishOS";
const char MER_WIZARD_FEATURE_EMULATOR[] = "Mer.Wizard.Feature.Emulator";

const char MER_SDK_FILENAME[] = "/qtcreator/mersdk.xml";
const char MER_TARGETS_FILENAME[] = "/targets.xml";
const char MER_DEVICES_FILENAME[] = "/devices.xml";
const char MER_DEBUGGER_FILENAME[] = "gdb-i486-meego-linux-gnu";

const char MER_TARGET_NAME[] = "MerTarget.Name";
const char MER_TARGET_QMAKE_DUMP[] = "MerTarget.QmakeQuery";
const char MER_TARGET_GCC_DUMP[] = "MerTarget.GccDumpMachine";
const char MER_TARGET_DEFAULT_DEBUGGER[] = "MerTarget.DefaultDebugger";

const char QMAKE_QUERY[] = "qmake.query";
const char QMAKE_VERSION[] = "qmake.version";
const char GCC_DUMPMACHINE[] = "gcc.dumpmachine";
const char GCC_DUMPVERSION[] = "gcc.dumpversion";

const char MER_AUTHORIZEDKEYS_FOLDER[] = "authorized_keys";
const char MER_SDK_KIT_INFORMATION[] = "Mer.Sdk.Kit.Information";
const char MER_TARGET_KIT_INFORMATION[] = "Mer.Target.Kit.Information";
const char MER_KIT_SPECIFY_INFORMATION[] = "Mer.Specify.Kit.Information";

const char MER_SSH_SHARED_HOME[] = "MER_SSH_SHARED_HOME";
const char MER_SSH_SHARED_TARGET[] = "MER_SSH_SHARED_TARGET";
const char MER_SSH_SHARED_SRC[] = "MER_SSH_SHARED_SRC";
const char MER_SSH_USERNAME[] = "MER_SSH_USERNAME";
const char MER_SSH_PRIVATE_KEY[] = "MER_SSH_PRIVATE_KEY";
const char MER_SSH_TARGET_NAME[] = "MER_SSH_TARGET_NAME";
const char MER_SSH_DEVICE_NAME[] = "MER_SSH_DEVICE_NAME";
const char MER_SSH_PORT[] = "MER_SSH_PORT";
const char MER_DEVICE_VIRTUAL_MACHINE[] = "MER_DEVICE_VIRTUAL_MACHINE";
const char MER_DEVICE_MAC[] = "MER_DEVICE_MAC";
const char MER_DEVICE_SUBNET[] = "MER_DEVICE_SUBNET";
const char MER_DEVICE_SHARED_SSH[] = "MER_DEVICE_SHARED_SSH";
const char MER_DEVICE_SHARED_CONFIG[]= "MER_DEVICE_SHARED_CONFIG";
const char MER_PROJECTPATH_ENVVAR_NAME[] = "MER_PROJECTPATH";

const char MER_YAML_MIME_TYPE[] = "text/x-yaml";
const char MER_YAML_EDITOR_ID[] = "Mer.YamlEditor";

} // namespace Constants
} // namespace Mer

#endif // MERCONSTANTS_H
