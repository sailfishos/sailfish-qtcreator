/****************************************************************************
**
** Copyright (C) 2020 Open Mobile Platform LLC.
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

#include "cmakecommand.h"

#include <mer/merconstants.h>
#include <sfdk/sfdkconstants.h>
#include <utils/qtcassert.h>

CMakeCommand::CMakeCommand()
{

}

QString CMakeCommand::name() const
{
    return QLatin1String("cmake");
}

int CMakeCommand::execute()
{
    if (arguments().contains(QLatin1String("-E"))
            && arguments().contains(QLatin1String("capabilities"))) {
        fprintf(stdout, "%s", capabilities().data());
        return 0;
    }

    if (arguments().contains(QLatin1String("--version"))
            || arguments().contains(QLatin1String("--help"))) {
        QTC_ASSERT(false, return 1);
    }

    QString command = QLatin1String("mb2") +
                      QLatin1String(" -t ") +
                      targetName() +
                      QLatin1Char(' ') + arguments().join(QLatin1Char(' ')) + QLatin1Char(' ');

    return executeRemoteCommand(command);
}

bool CMakeCommand::isValid() const
{
    return Command::isValid() && !targetName().isEmpty() && !sdkToolsPath().isEmpty();
}

QByteArray CMakeCommand::capabilities()
{
    return R"({
        "generators": [
            {
                "extraGenerators": [
                    "CodeBlocks"
                ],
                "name": "Unix Makefiles",
                "platformSupport": false,
                "toolsetSupport": false
            }
        ],
        "serverMode": false
    })";
}
