/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included
** in the packaging of this file. Please review the following information
** to ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/lgpl-2.1.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

namespace SailfishWizards {
namespace Constants {

// Generated file mime types
const char JAVA_SCRIPT_MIMETYPE[] = "application/javascript";
const char DESKTOP_MIMETYPE[] = "application/x-desktop";
const char YAML_MIMETYPE[] = "application/x-yaml";

// Regular expressions
const char SPEC_FILE_NAME_REG_EXP[] = "(^[a-zA-Z0-9]+([a-zA-Z_0-9.-]*))$";
const char SPEC_VERSION_REG_EXP[] = "^[0-9]+([.][0-9]+)*$";
const char SPEC_EPOCH_EXP[] = "^[0-9]*$";
const char REG_EXP_FOR_NAME_FILE[] = "(^[a-zA-Z0-9]+([a-zA-Z_0-9.-]*))$";
const char REG_EXP_FOR_DESKTOP_FILE_PROFILE_PREFIX[] =
    "(desktop+([0-9]*)+(.files = )+([a-zA-Z_0-9.- \n]*))$";
const char REG_EXP_FOR_ICONS_PROFILE_PREFIX[] =
    "(icon86+_+([0-9]*)+(.files = )+([a-zA-Z_0-9.- \n/]*))$";
const char REG_EXP_FOR_NAME_CLASS[] = "(^[a-zA-Z0-9]+([a-zA-Z_0-9]*)((::([a-zA-Z_0-9]+))?)*)$";
const char CPP_HEADER_MYMETYPE[] = "text/x-c++hdr";
const char CPP_SOURCE_MYMETYPE[] = "text/x-c++src";
const char QML_MIMETYPE[] = "text/x-qml";

const char ACTION_ID[] = "CodeSnippetWizard.Action";
const char MENU_ID[] = "CodeSnippetWizard.Menu";
const char MER_TOOLS_MENU[] = "Mer.Tools.Menu";

} // namespace SailfishWizards
} // namespace Constants
