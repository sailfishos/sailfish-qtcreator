/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "cmakecommandbuilder.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>

#include <utils/qtcprocess.h>

#include <cmakeprojectmanager/cmakeprojectconstants.h> // Compile-time only

#include <QRegularExpression>
#include <QStandardPaths>

using namespace ProjectExplorer;

namespace IncrediBuild {
namespace Internal {

QList<Utils::Id> CMakeCommandBuilder::migratableSteps() const
{
    return {CMakeProjectManager::Constants::CMAKE_BUILD_STEP_ID};
}

QString CMakeCommandBuilder::defaultCommand() const
{
    const QString defaultCMake = "cmake";
    const QString cmake = QStandardPaths::findExecutable(defaultCMake);
    return cmake.isEmpty() ? defaultCMake : cmake;
}

QString CMakeCommandBuilder::defaultArguments() const
{
    // Build folder or "."
    QString buildDir;
    BuildConfiguration *buildConfig = buildStep()->buildConfiguration();
    if (buildConfig)
        buildDir = buildConfig->buildDirectory().toString();

    if (buildDir.isEmpty())
        buildDir = ".";

    return Utils::QtcProcess::joinArgs({"--build", buildDir, "--target", "all"});
}

QString CMakeCommandBuilder::setMultiProcessArg(QString args)
{
    QRegularExpression regExp("\\s*\\-j\\s+\\d+");
    args.remove(regExp);
    args.append(" -- -j 200");

    return args;
}

} // namespace Internal
} // namespace IncrediBuild
