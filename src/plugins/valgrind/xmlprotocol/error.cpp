/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Frank Osterfeld, KDAB (frank.osterfeld@kdab.com)
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "error.h"
#include "frame.h"
#include "stack.h"
#include "suppression.h"

#include <QSharedData>
#include <QString>
#include <QTextStream>
#include <QVector>

#include <algorithm>

namespace Valgrind {
namespace XmlProtocol {

class Error::Private : public QSharedData
{
public:
    qint64 unique = 0;
    qint64 tid = 0;
    QString what;
    int kind = 0;
    QVector<Stack> stacks;
    Suppression suppression;
    quint64 leakedBytes = 0;
    qint64 leakedBlocks = 0;
    qint64 hThreadId = -1;

    bool operator==(const Private &other) const
    {
        return unique == other.unique
                && tid == other.tid
                && what == other.what
                && kind == other.kind
                && stacks == other.stacks
                && suppression == other.suppression
                && leakedBytes == other.leakedBytes
                && leakedBlocks == other.leakedBlocks
                && hThreadId == other.hThreadId;
    }
};

Error::Error() :
    d(new Private)
{
}

Error::~Error() = default;

Error::Error(const Error &other) = default;

void Error::swap(Error &other)
{
    std::swap(d, other.d);
}

Error &Error::operator=(const Error &other)
{
    Error tmp(other);
    swap(tmp);
    return *this;
}

bool Error::operator ==(const Error &other) const
{
    return *d == *other.d;
}

bool Error::operator !=(const Error &other) const
{
    return !(*d == *other.d);
}

Suppression Error::suppression() const
{
    return d->suppression;
}

void Error::setSuppression(const Suppression &supp)
{
    d->suppression = supp;
}

qint64 Error::unique() const
{
    return d->unique;
}

void Error::setUnique(qint64 unique)
{
    d->unique = unique;
}

qint64 Error::tid() const
{
    return d->tid;
}

void Error::setTid(qint64 tid)
{
    d->tid = tid;
}

quint64 Error::leakedBytes() const
{
    return d->leakedBytes;
}

void Error::setLeakedBytes(quint64 l)
{
    d->leakedBytes = l;
}

qint64 Error::leakedBlocks() const
{
    return d->leakedBlocks;
}

void Error::setLeakedBlocks(qint64 b)
{
    d->leakedBlocks = b;
}

QString Error::what() const
{
    return d->what;
}

void Error::setWhat(const QString &what)
{
    d->what = what;
}

int Error::kind() const
{
    return d->kind;
}

void Error::setKind(int k)
{
    d->kind = k;
}

QVector<Stack> Error::stacks() const
{
    return d->stacks;
}

void Error::setStacks(const QVector<Stack> &stacks)
{
    d->stacks = stacks;
}

void Error::setHelgrindThreadId(qint64 id)
{
    d->hThreadId = id;
}

qint64 Error::helgrindThreadId() const
{
    return d->hThreadId;
}

QString Error::toXml() const
{
    QString xml;
    QTextStream stream(&xml);
    stream << "<error>\n";
    stream << "  <unique>" << d->unique << "</unique>\n";
    stream << "  <tid>" << d->tid << "</tid>\n";
    stream << "  <kind>" << d->kind << "</kind>\n";
    if (d->leakedBlocks > 0 && d->leakedBytes > 0) {
        stream << "  <xwhat>\n"
               << "    <text>" << d->what << "</text>\n"
               << "    <leakedbytes>" << d->leakedBytes << "</leakedbytes>\n"
               << "    <leakedblocks>" << d->leakedBlocks << "</leakedblocks>\n"
               << "  </xwhat>\n";
    } else {
        stream << "  <what>" << d->what << "</what>\n";
    }

    foreach (const Stack &stack, d->stacks) {
        if (!stack.auxWhat().isEmpty())
            stream << "  <auxwhat>" << stack.auxWhat() << "</auxwhat>\n";
        stream << "  <stack>\n";

        foreach (const Frame &frame, stack.frames()) {
            stream << "    <frame>\n";
            stream << "      <ip>0x" << QString::number(frame.instructionPointer(), 16) << "</ip>\n";
            if (!frame.object().isEmpty())
                stream << "      <obj>" << frame.object() << "</obj>\n";
            if (!frame.functionName().isEmpty())
                stream << "      <fn>" << frame.functionName() << "</fn>\n";
            if (!frame.directory().isEmpty())
                stream << "      <dir>" << frame.directory() << "</dir>\n";
            if (!frame.fileName().isEmpty())
                stream << "      <file>" << frame.fileName() << "</file>\n";
            if (frame.line() != -1)
                stream << "      <line>" << frame.line() << "</line>";
            stream << "    </frame>\n";
        }

        stream << "  </stack>\n";
    }

    stream << "</error>\n";

    return xml;
}

} // namespace XmlProtocol
} // namespace Valgrind
