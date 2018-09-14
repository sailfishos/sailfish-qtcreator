/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include "qmakecommand.h"

#include "merremoteprocess.h"

#include <mer/merconstants.h>

#include <QDir>
#include <QFile>
#include <QStringList>

QMakeCommand::QMakeCommand()
{

}

QString QMakeCommand::name() const
{
    return QLatin1String("qmake");
}

int QMakeCommand::execute()
{
    if (arguments().contains(QLatin1String("-version")))
        m_cacheFile = QLatin1String(Mer::Constants::QMAKE_VERSION);
    else if (arguments().contains(QLatin1String("-query")))
        m_cacheFile = QLatin1String(Mer::Constants::QMAKE_QUERY);

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

    QString evalCommand;
    QString filterCommand; 
    const int evalIndex = arguments().indexOf(QLatin1String("-env"));
    if (evalIndex != -1) {
        //Creating special pro file
        const QString ppath = QString("/home/%1/query_var").arg(sshParameters().userName);
        const int varIndex = evalIndex + 1;

        auto args = arguments();
        evalCommand = QString("echo \'%1\' > %2;").
                arg((args.size() > varIndex) ?
                        //Evaluate var in second parameter
                        QLatin1String("log($$escape_expand($$eval($$VAR)\\n))") :
                        //Evaluate all vars
                        QLatin1String("for(var, $$list($$enumerate_vars())) log($$escape_expand($$var=$$eval($$var)\\n\\r))")).
                arg(ppath);

        //Tansforming args
        args.replace(evalIndex, ppath);

        if (args.size() > varIndex)
            args.replace(varIndex, "VAR=" + args.at(varIndex));

        setArguments(args);

        //Redirect stderr to stdout and suppress stdout
        filterCommand = "2>&1>/dev/null";
    }

    const QString projectPathParameter = projectPath().isEmpty()
        ? QString()
        : QLatin1String(" -p ") + QLatin1Char('\'') + projectPath() +   QLatin1Char('\'');

    QString command = evalCommand + QLatin1String("mb2 --pedantic") +
                      projectPathParameter +
                      QLatin1String(" -t ") +
                      targetName() +
                      QLatin1Char(' ') + arguments().join(QLatin1Char(' ')) + QLatin1Char(' ') + filterCommand;

    MerRemoteProcess process;
    process.setSshParameters(sshParameters());
    process.setCommand(remotePathMapping(command));
    return process.executeAndWait();

//TODO: remote command to cache ?
//    if (ok && !m_currentCacheFile.isEmpty()) {
//        QFile file(m_currentCacheFile);
//        if (!file.open(QIODevice::WriteOnly)) {
//            printError(QString::fromLatin1("Cannot write file \"%1\"").arg(m_currentCacheFile));
//            QCoreApplication::exit(1);
//        }
//        if (m_currentCacheFile.endsWith(QLatin1String(Mer::Constants::QMAKE_QUERY)))
//            m_currentCacheData.replace(":/", QString(QLatin1Char(':') + m_merSysRoot
//                                                     + QLatin1Char('/')).toUtf8());
//        file.write(m_currentCacheData);
//        fprintf(stdout, "%s", m_currentCacheData.constData());
//        fflush(stdout);
//    }
}

bool QMakeCommand::isValid() const
{
    return Command::isValid() && !targetName().isEmpty() && !sdkToolsPath().isEmpty();
}
