/****************************************************************************
**
** Copyright (C) 2018-2019 Jolla Ltd.
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

#include "wwwproxycommand.h"

#include "merremoteprocess.h"

#include <QRegularExpression>

WwwProxyCommand::WwwProxyCommand()
{

}

QString WwwProxyCommand::name() const
{
    return QLatin1String("wwwproxy");
}

int WwwProxyCommand::execute()
{
    QStringList args = arguments();
    QString conf = QLatin1String("proxy");
    QRegularExpression reg("[']");
    conf.append(QLatin1Char(' '));
    conf.append(args.at(1));
    if (args.length() > 2) {
        conf.append(QLatin1Char(' '));
        conf.append(QLatin1Char('\''));
        conf.append(QString(args.at(2)).replace(reg, "'\\''"));
        conf.append(QLatin1Char('\''));
    }
    if (args.length() > 3) {
        conf.append(QLatin1Char(' '));
        conf.append(QLatin1String("--excludes"));
        conf.append(QLatin1Char(' '));
        conf.append(QLatin1Char('\''));
        conf.append(QString(args.at(3)).replace(reg, "'\\''"));
        conf.append(QLatin1Char('\''));
    }
    QString command = QLatin1String("sudo connmanctl config ethernet_$(cat /sys/class/net/eth0/address |sed 's/://g')_cable") +
                      QLatin1Char(' ') + conf;
    MerRemoteProcess process;
    process.setSshParameters(sshParameters());
    process.setCommand(remotePathMapping(command));
    return process.executeAndWait();
}

bool WwwProxyCommand::isValid() const
{
    return arguments().length() > 1;
}
