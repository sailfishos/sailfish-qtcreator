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

#include "gcccommand.h"

#include <mer/merconstants.h>

#include <QDir>
#include <QFile>

GccCommand::GccCommand()
{
}

QString GccCommand::name() const
{
    return QLatin1String("gcc");
}

int GccCommand::execute()
{
    if (arguments().contains(QLatin1String("-dumpmachine")))
        m_cacheFile = QLatin1String(Mer::Constants::GCC_DUMPMACHINE);
    else if (arguments().contains(QLatin1String("-dumpversion")))
        m_cacheFile = QLatin1String(Mer::Constants::GCC_DUMPVERSION);
    else if (arguments().contains(QLatin1String("-dM")))
        m_cacheFile = QLatin1String(Mer::Constants::GCC_DUMPMACROS);
    else if (arguments().contains(QLatin1String("-E")) && arguments().contains(QLatin1String("-")))
        m_cacheFile = QLatin1String(Mer::Constants::GCC_DUMPINCLUDES);

    if (!m_cacheFile.isEmpty())
            m_cacheFile.prepend(sdkToolsPath() + QDir::separator());

    if (QFile::exists(m_cacheFile)) {
        QFile cacheFile(m_cacheFile);
        if (!cacheFile.open(QIODevice::ReadOnly)) {
            return 1;
        }
        fprintf(stdout, "%s", cacheFile.readAll().constData());
        fflush(stdout);
        return 0;
    }

    return 1;
//TODO: remote command to cache ?
    /*
    // hack for gcc when querying pre-defined macros and header paths
    QRegExp rx(QLatin1String("\\bgcc\\b.*\\B-\\B"));
    const bool queryPredefinedMacros = rx.indexIn(completeCommand) != -1;
    if (queryPredefinedMacros)
        completeCommand.append(QLatin1String(" < /dev/null"));
    */
}

bool GccCommand::isValid() const
{
    return !sdkToolsPath().isEmpty();
}
