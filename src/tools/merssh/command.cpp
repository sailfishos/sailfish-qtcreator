/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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
#include <QFile>
#include <QDir>

Command::Command()
{
}

Command::~Command()
{
}

QString Command::sharedHomePath() const
{
    return m_sharedHomePath;
}

void Command::setSharedHomePath(const QString& path)
{
    m_sharedHomePath = QDir::fromNativeSeparators(path);
}

QString Command::targetName() const
{
    return m_targetName;
}

void Command::setTargetName(const QString& name)
{
    m_targetName = name;
}

QString Command::sharedSourcePath() const
{
    return m_sharedSourcePath;
}

void Command::setSharedSourcePath(const QString& path)
{
    m_sharedSourcePath = QDir::fromNativeSeparators(path);
}

QString Command::sharedTargetPath() const
{
    return m_sharedSourcePath;
}

void Command::setSharedTargetPath(const QString& path)
{
    m_sharedSourcePath = QDir::fromNativeSeparators(path);
}

QString Command::sdkToolsPath() const
{
    return m_toolsPath;
}

void Command::setProjectPath(const QString& path)
{
    m_projectPath =  QDir::fromNativeSeparators(path);
}

QString Command::projectPath() const
{
    return m_projectPath;
}

void Command::setSdkToolsPath(const QString& path)
{
    m_toolsPath = QDir::fromNativeSeparators(path);
}

void Command::setArguments(const QStringList &args)
{
    m_args.clear();
    foreach (const QString& arg,args)
        m_args.append(shellSafeArgument(arg));
}

QStringList Command::arguments() const
{
    return m_args;
}

void Command::setSshParameters(const QSsh::SshConnectionParameters& params)
{
    m_sshConnectionParams = params;
}

QSsh::SshConnectionParameters Command::sshParameters() const
{
    return m_sshConnectionParams;
}

QString Command::deviceName() const
{
    return m_deviceName;
}

void Command::setDeviceName(const QString& device)
{
    m_deviceName = device;
}

QString Command::shellSafeArgument(const QString &argument) const
{
    //unix style
    static QRegExp reg1(QLatin1String("^'(.*)'$"));
    //windows style
    static QRegExp reg2(QLatin1String("^\"(.*)\"$"));

    QString safeArgument = argument;

    //unquote for file exits
    if (reg1.indexIn(argument) != -1) {
        safeArgument = reg1.cap(1);
    }

    if (reg2.indexIn(argument) != -1) {
        safeArgument = reg2.cap(1);
    }

    if (QFile::exists(safeArgument)) {
        safeArgument = QDir::fromNativeSeparators(safeArgument);
        if (safeArgument.indexOf(QLatin1Char(' ')) > -1)
            safeArgument = QLatin1Char('\'') + safeArgument + QLatin1Char('\'');
        return safeArgument;
    }
    return argument;
}

QString Command::remotePathMapping(const QString& command) const
{
    QString result = command;

    result.prepend(QLatin1String("cd '") + QDir::currentPath()
                   + QLatin1String("' && "));

    // First replace shared home and then shared src (error prone!)
    if (!sharedHomePath().isEmpty())
    result.replace(sharedHomePath(), QLatin1String("/home/mersdk/share"));
    if (!sharedSourcePath().isEmpty())
        result.replace(sharedSourcePath(), QLatin1String("/home/src1"));
    result = result.trimmed();
    return result;
}

bool Command::isValid() const
{
    const QString currentPath = QDir::currentPath();
    const QString cleanSharedHome = QDir::fromNativeSeparators(QDir::cleanPath(sharedHomePath()));
    const QString cleanSharedSrc = QDir::fromNativeSeparators(QDir::cleanPath(sharedSourcePath()));

    if (!currentPath.startsWith(cleanSharedHome) && (sharedSourcePath().isEmpty() ||
                                                     !currentPath.startsWith(cleanSharedSrc))) {
        QString message = QString::fromLatin1("Project ERROR: Project is outside of shared home '%1' and shared src '%2'.")
                .arg(QDir::toNativeSeparators(sharedHomePath())).arg(QDir::toNativeSeparators(sharedSourcePath()));
        fprintf(stderr, "%s", qPrintable(message));
        fflush(stderr);
        return false;
    }
    return true;
}
