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
#include <utils/qtcprocess.h>

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QStringList>

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
        m_cacheFile = QLatin1String(Sfdk::Constants::CMAKE_QUERY_CACHE);
    }

    if (!m_cacheFile.isEmpty()) {
        m_cacheFile.prepend(sdkToolsPath() + QDir::separator());

        if (QFile::exists(m_cacheFile)) {
            QFile cacheFile(m_cacheFile);
            if (!cacheFile.open(QIODevice::ReadOnly)) {
                fprintf(stderr, "%s",qPrintable(QString::fromLatin1("Cannot read cached file \"%1\"").arg(m_cacheFile)));
                fflush(stderr);
                return 1;
            }
            fprintf(stdout, "%s", cacheFile.readAll().constData());
            fflush(stdout);
            return 0;
        }

        return 1;
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
    QJsonArray extraGenerators;
    extraGenerators.append(QJsonValue("CodeBlocks"));
    QJsonObject makefileGenerator;
    makefileGenerator["extraGenerators"]=extraGenerators;
    makefileGenerator["name"]=QJsonValue("Unix Makefiles");
    makefileGenerator["platformSupport"]=QJsonValue(false);
    makefileGenerator["toolsetSupport"]=QJsonValue(false);

    QJsonArray generators;
    generators.append(makefileGenerator);

    QJsonObject capabilities;
    capabilities["generators"]=generators;
    capabilities["serverMode"]=QJsonValue(false);

    QJsonDocument capabilityDocument(capabilities);
    return capabilityDocument.toJson();
}
