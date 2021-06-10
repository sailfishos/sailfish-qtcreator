/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
** Copyright (C) 2019-2020 Open Mobile Platform LLC.
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

const char EXE_NAME[] = "sfdk";
const char APP_ID[] = "sfdk";
const char SDK_FRONTEND_ID[] = "sfdk";
const char DISPATCH_TR_CONTEXT[] = "Sfdk::Dispatch";

const char GENERAL_DOMAIN_NAME[] = "general";
const char DEVICE_OPTION_NAME[] = "device";
const char HOOKS_DIR_OPTION_NAME[] = "hooks-dir";
const char TARGET_OPTION_NAME[] = "target";
const char SNAPSHOT_OPTION_NAME[] = "snapshot";
const char NO_SNAPSHOT_OPTION_NAME[] = "no-snapshot";

const char BUILD_ENGINE_SYSTEM_BUS_CONNECTION[] = "build-engine-system-bus";
const char SAILFISH_SDK_SFDK_DBUS_SERVICE[] = "SAILFISH_SDK_SFDK_DBUS_SERVICE";

const char WORKSPACE_PSEUDO_VARIABLE[] = "$WORKSPACE";

const char OPTIONS_ENV_VAR[] = "SFDK_OPTIONS";
const char DISABLE_REVERSE_PATH_MAPPING_ENV_VAR[] = "SFDK_NO_REVERSE_PATH_MAP";
const char MSYS_DETECTION_ENV_VAR[] = "MSYSTEM";
const char NO_WARN_ABOUT_WINPTY_ENV_VAR[] = "SFDK_NO_WINPTY";
const char DISABLE_VM_INFO_CACHE_ENV_VAR[] = "SFDK_NO_VM_INFO_CACHE";
const char EXIT_ABNORMAL_ENV_VAR[] = "SFDK_EXIT_ABNORMAL";
const int EXIT_ABNORMAL_DEFAULT_CODE = 120;
const char NO_QPA_PLATFORM_MINIMAL_ENV_VAR[] = "SFDK_NO_QPA_PLATFORM_MINIMAL";
const char CONNECTED_TO_TERMINAL_HINT_ENV_VAR[] = "SFDK_CONNECTED_TO_TERMINAL";
const char NO_TEMP_REPOSITORY_ENV_VAR[] = "SFDK_NO_TEMP_REPOSITORY";
const char NO_BYPASS_OPENSSL_ARMCAP_ENV_VAR[] = "SFDK_NO_BYPASS_OPENSSL_ARMCAP";
const char NO_SESSION_ENV_VAR[] = "SFDK_NO_SESSION";
const char SFDK_AUTO_STOP_VMS_ENV_VAR[] = "SFDK_AUTO_STOP_VMS";

} // namespace Constants
} // namespace Sfdk
