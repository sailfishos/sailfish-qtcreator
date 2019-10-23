/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include <QMetaEnum>
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

class TreePrinter
{
public:
    class Item
    {
    public:
        Item(const QStringList &columns) : columns(columns) {}

        QStringList columns;
        std::vector<Item> children;
    };

    using Tree = std::vector<Item>;

    static Tree build(const QList<QStringList> &table, int idColumn,
            int parentIdColumn);
    static void sort(Tree *tree, int level, int column, bool ascending);
    static void print(QTextStream &out, const Tree &tree, const QList<int> &columns);

private:
    static void print(QTextStream &out, const Item &item,
            const QString &linePrefix, const QList<int> &columns, const QList<int> &widths);
    static TreePrinter::Item *findItem(Tree &tree, const QString &id,
            int idColumn);

    template<typename T>
    static T walk(const Tree &tree, T init, std::function<T(T, int, const Item &)> op)
    {
        T acc = init;
        for (const Item &item : tree)
            acc = walk_helper<T>(item, acc, 0, op);
        return acc;
    }

    template<typename T>
    static T walk_helper(const Item &tree, T init, int depth, std::function<T(T, int, const Item &)> op)
    {
        T acc = op(init, depth, tree);
        for (const Item &item : tree.children)
            acc = walk_helper<T>(item, acc, depth + 1, op);
        return acc;
    }
};

template<typename Enum>
const char *enumName(Enum value) { return QMetaEnum::fromType<Enum>().valueToKey(value); }

} // namespace Sfdk
