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

#ifdef MER_BASE_OS_NAME
const char MER_OS_NAME[] = QT_TRANSLATE_NOOP("Mer", MER_BASE_OS_NAME " OS");
const char MER_SDK_NAME[] = QT_TRANSLATE_NOOP("Mer", MER_BASE_OS_NAME " SDK");
#else
const char MER_OS_NAME[] = "Sailfish OS";
const char MER_SDK_NAME[] = "Sailfish SDK";
#endif

const char MER_QT[] = "QmakeProjectManager.QtVersion.Mer";
const char MER_TOOLCHAIN_ID[] = "QmakeProjectManager.ToolChain.Mer";

const char MER_OPTIONS_CATEGORY[] = "W.Mer";
const char MER_OPTIONS_CATEGORY_TR[] = QT_TRANSLATE_NOOP("Mer", MER_BASE_OS_NAME " OS");
const char MER_BUILD_ENGINE_OPTIONS_ID[] = "A.Mer";
const char MER_BUILD_ENGINE_OPTIONS_NAME[] = QT_TRANSLATE_NOOP("Mer", "Build Engine");
const char MER_EMULATOR_OPTIONS_ID[] = "B.MerEmulator";
const char MER_EMULATOR_OPTIONS_NAME[] = QT_TRANSLATE_NOOP("Mer", "Emulator");
const char MER_GENERAL_OPTIONS_ID[] = "Z.MerGeneral";
const char MER_GENERAL_OPTIONS_NAME[] = QT_TRANSLATE_NOOP("Mer", "General");
const char MER_EMULATOR_MODE_OPTIONS_ID[] = "C.MerEmulatorMode";
const char MER_EMULATOR_MODE_OPTIONS_NAME[] = QT_TRANSLATE_NOOP("Mer", "Emulator Mode");

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

const char BUILD_ENGINE_URI[] = "BuildEngineUri";
const char BUILD_TARGET_NAME[] = "BuildTargetName";

const char MER_RUNCONFIGURATION_PREFIX[] = "QmakeProjectManager.MerRunConfiguration:";
const char MER_QMLRUNCONFIGURATION[] = "QmakeProjectManager.MerQmlRunConfiguration";

const char SAILFISH_QML_LAUNCHER[] = "/usr/bin/sailfish-qml";

const char MER_EMULATOR_CONNECTON_ACTION_ID[] = "Mer.EmulatorConnecitonAction";
const char MER_SDK_CONNECTON_ACTION_ID[] = "Mer.SdkConnectionAction";

const char MER_WIZARD_FEATURE_SAILFISHOS[] = "Mer.Wizard.Feature.SailfishOS";
const char MER_WIZARD_FEATURE_EMULATOR[] = "Mer.Wizard.Feature.Emulator";

const char MER_SDK_SHARED_HOME_MOUNT_POINT[] = "/home/mersdk/share";
const char MER_SDK_SHARED_SRC_MOUNT_POINT[] = "/home/src1";

const char MER_AUTHORIZEDKEYS_FOLDER[] = "authorized_keys";

const char SAILFISH_OS_SDK_ENVIRONMENT_FILTER_DEPRECATED[] = "SAILFISH_OS_SDK_ENVIRONMENT_FILTER";
const char SAILFISH_SDK_ENVIRONMENT_FILTER[] = "SAILFISH_SDK_ENVIRONMENT_FILTER";

const char SAILFISH_SDK_FRONTEND[] = "SAILFISH_SDK_FRONTEND";
const char SAILFISH_SDK_FRONTEND_ID[] = "qtcreator";

const char MER_DEVICE_SHARED_SSH[] = "MER_DEVICE_SHARED_SSH";
const char MER_DEVICE_QML_LIVE_PORTS[] = "MER_DEVICE_QML_LIVE_PORTS";
const char MER_DEVICE_ARCHITECTURE[]= "MER_DEVICE_ARCHITECTURE";

const char MER_RUN_CONFIGURATION_ASPECT[] = "Mer.RunConfigurationAspect";

const char QML_LIVE_HELP_URL[] = "qthelp://org.qt-project.qtcreator/doc/creator-qtquick-qmllive-sailfish.html";

} // namespace Constants
} // namespace Mer

#endif // MERCONSTANTS_H
