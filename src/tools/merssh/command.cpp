/****************************************************************************
**
** Copyright (C) 2012-2015,2018-2019 Jolla Ltd.
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

#include "command.h"

#include <mer/merconstants.h>
#include <sfdk/sfdkconstants.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QRegularExpression>
#include <QTextStream>

using namespace Utils;

Command::Command()
{
}

Command::~Command()
{
}

int Command::executeSfdk(const QStringList &arguments)
{
    QTextStream qerr(stderr);

    static const QString program = QCoreApplication::applicationDirPath()
        + '/' + RELATIVE_REVERSE_LIBEXEC_PATH + "/sfdk" QTC_HOST_EXE_SUFFIX;

    qerr << "+ " << program << ' ' << QtcProcess::joinArgs(arguments, Utils::OsTypeLinux) << endl;

    QProcess process;
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.setProgram(program);
    process.setArguments(arguments);

    int rc{};
    process.start();
    if (!process.waitForFinished(-1) || process.error() == QProcess::FailedToStart)
        rc = -2; // like QProcess::execute does
    else
        rc = process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;

    return rc;
}

QString Command::sdkToolsPath() const
{
    return m_toolsPath;
}

void Command::setSdkToolsPath(const QString& path)
{
    m_toolsPath = QDir::fromNativeSeparators(path);
}

void Command::setArguments(const QStringList &args)
{
    m_args = args;
}

QStringList Command::arguments() const
{
    return m_args;
}

bool Command::isValid() const
{
    return true;
}
