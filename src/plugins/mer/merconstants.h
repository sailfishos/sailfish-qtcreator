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

const char MER_QT[]  = "Qt4ProjectManager.QtVersion.Mer";
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

const char MER_TASKHUB_CATEGORY[] = "Qt4ProjectManager.TaskHub.Mer";
const char MER_DEVICE_TYPE[] = "Mer.Device.Type";

#ifdef Q_OS_WIN
#define SCRIPT_EXTENSION ".cmd"
#else // Q_OS_WIN
#define SCRIPT_EXTENSION ""
#endif // Q_OS_WIN

const char MER_WRAPPER_QMAKE[] = "qmake"SCRIPT_EXTENSION;
const char MER_WRAPPER_MAKE[] = "make"SCRIPT_EXTENSION;
const char MER_WRAPPER_GCC[] = "gcc"SCRIPT_EXTENSION;
const char MER_WRAPPER_GDB[] = "gdb"SCRIPT_EXTENSION;
const char MER_WRAPPER_SPECIFY[] = "specify"SCRIPT_EXTENSION;
const char MER_WRAPPER_MB[] = "mb"SCRIPT_EXTENSION;
const char MER_WRAPPER_RM[] = "rm"SCRIPT_EXTENSION;
const char MER_WRAPPER_MV[] = "mv"SCRIPT_EXTENSION;

const char MER_SDK_DEFAULTUSER[] = "mersdk";
const char MER_DEVICE_DEFAULTUSER[] = "nemo";
const char MER_SDK_DEFAULTHOST[] = "localhost";

const char MER_EXECUTIONTYPE_STANDARD[] = "standard";
const char MER_EXECUTIONTYPE_SB2[] = "sb2";

const char MERSSH_PARAMETER_SDKTOOLSDIR[] = "-sdktoolsdir";
const char MERSSH_PARAMETER_MERTARGET[] = "-mertarget";
const char MERSSH_PARAMETER_COMMANDTYPE[] = "-commandtype";

// Keys used for ini files
const char MER_SDK[] = "MerSDK";
const char MER_SDK_TOOLS[] = "/mer-sdk-tools/";
const char SHARED_HOME[] = "SharedHome";
const char SHARED_TARGET[] = "SharedTarget";
const char SHARED_SSH[] = "SharedSsh";
const char AUTHENTICATION_TYPE[] = "AuthenticationType";
const char USERNAME[] = "Username";
const char PASSWORD[] = "Password";
const char KEY[] = "Key";
const char PRIVATE_KEY_FILE[] = "PrivateKeyFile";
const char SSH_PORT[] = "SshPort";
const char ID[] = "Id";
const char NAME[] = "Name";
const char KITS[] = "Kits";
const char SYSROOT[] = "Sysroot";
const char TOOL_CHAINS[] = "ToolChains";
const char QT_VERSIONS[] = "QtVersions";
const char AUTO_DETECTED[] = "AutoDetected";
const char TYPE[] = "Type";
const char TARGET[] = "Target";
const char VIRTUAL_MACHINE[] = "VirtualMachine";
const char SB2_TARGET_NAME[] = "SB2.TargetName";

const char MER_RUNCONFIGURATION_PREFIX[] = "Qt4ProjectManager.MerRunConfiguration:";
const char MER_SFTP_DEPLOY_STRING[] = QT_TRANSLATE_NOOP("Mer", "Copy Files to Device");
const char MER_SFTP_DEPLOY_CONFIGURATION_ID[] = "Qt4ProjectManager.MerSftpDeployConfiguration";

const char MER_EMULATOR_START_ACTION_ID[] = "Mer.MerEmulatorStartAction";

const char SAILFISHOS_FEATURE[] = "QtSupport.Wizards.FeatureSailfishOS";

} // namespace Constants
} // namespace Mer

#endif // MERCONSTANTS_H
