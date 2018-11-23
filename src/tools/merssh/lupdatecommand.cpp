/****************************************************************************
**
** Copyright (C) 2018 Jolla Ltd.
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
****************************************************************************/

#include "lupdatecommand.h"

#include "merremoteprocess.h"

#include <mer/merconstants.h>

#include <QDir>
#include <QFile>
#include <QStringList>

LUpdateCommand::LUpdateCommand()
{

}

QString LUpdateCommand::name() const
{
    return QLatin1String("lupdate");
}

int LUpdateCommand::execute()
{
    QString command = QLatin1String("sb2 ") +
                      arguments().join(QLatin1Char(' '));

    MerRemoteProcess process;
    process.setSshParameters(sshParameters());
    process.setCommand(remotePathMapping(command));
    return process.executeAndWait();
}

bool LUpdateCommand::isValid() const
{
    return Command::isValid();
}
