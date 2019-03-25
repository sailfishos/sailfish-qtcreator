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

#include <QProcess>
#include <QTextStream>

namespace Sfdk {

QTextStream &qout();
QTextStream &qerr();

class Pager
{
public:
    Pager();
    ~Pager();
    operator QTextStream &() { return m_stream; }

    static void setEnabled(bool enabled) { s_enabled = enabled; }

private:
    static bool s_enabled;
    QProcess m_pager;
    QTextStream m_stream;
};

QString indent(int level);
void wrapLine(QTextStream &out, int indentLevel, const QString &prefix1, const QString &prefix2,
        const QString &line);
void wrapLine(QTextStream &out, int indentLevel, const QString &line);
void wrapLines(QTextStream &out, int indentLevel, const QStringList &prefix1,
        const QStringList &prefix2, const QString &lines);

} // namespace Sfdk
