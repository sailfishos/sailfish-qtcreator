/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "core_global.h"

#include <utils/fileutils.h>

QT_BEGIN_NAMESPACE
class QWidget;
QT_END_NAMESPACE

namespace Utils { class Environment; }

namespace Core {

enum class HandleIncludeGuards { No, Yes };

struct CORE_EXPORT FileUtils
{
    // Helpers for common directory browser options.
    static void showInGraphicalShell(QWidget *parent, const QString &path);
    static void openTerminal(const QString &path);
    static void openTerminal(const QString &path, const Utils::Environment &env);
    static QString msgFindInDirectory();
    // Platform-dependent action descriptions
    static QString msgGraphicalShellAction();
    static QString msgTerminalHereAction();
    static QString msgTerminalWithAction();
    // File operations aware of version control and file system case-insensitiveness
    static void removeFile(const QString &filePath, bool deleteFromFS);
    static void removeFiles(const Utils::FilePaths &filePaths, bool deleteFromFS);
    static bool renameFile(const QString &from, const QString &to,
                           HandleIncludeGuards handleGuards = HandleIncludeGuards::No);
    // This method is used to refactor the include guards in the renamed headers
    static bool updateHeaderFileGuardAfterRename(const QString &headerPath, const QString &oldHeaderBaseName);
};

} // namespace Core
