/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#pragma once

#include <QCoreApplication>
#include <QSet>
#include <QString>

QT_BEGIN_NAMESPACE
class QProcess;
QT_END_NAMESPACE

namespace Sfdk {

class Session
{
    Q_DECLARE_TR_FUNCTIONS(Sfdk::Session)

public:
    Session();
    ~Session();

    static bool isValid();
    static QString id();
    static bool isSessionActive(const QString &id);

private:
    static QString getSessionId(const QString &processId = {});
    static bool finishedOk(QProcess *process);
#ifdef Q_OS_WIN
    struct WindowsPidToCygwinPid;
    static QString firstCygwinAncestorPid();
#endif
    static QString procfsStat(const QString &pid, int field, bool silent);

private:
    static Session *s_instance;
    QString m_id;
    QSet<QString> m_activeSessions;
};

} // namespace Sfdk
