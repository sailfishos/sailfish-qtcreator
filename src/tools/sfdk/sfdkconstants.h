/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

const char DISABLE_REVERSE_PATH_MAPPING_ENV_VAR[] = "SFDK_NO_REVERSE_PATH_MAP";
const char MSYS_DETECTION_ENV_VAR[] = "MSYSTEM";
const char NO_WARN_ABOUT_WINPTY_ENV_VAR[] = "SFDK_NO_WINPTY";
const char DISABLE_VM_INFO_CACHE_ENV_VAR[] = "SFDK_NO_VM_INFO_CACHE";

const int EXIT_ABNORMAL = -1;

} // namespace Constants
} // namespace Sfdk
