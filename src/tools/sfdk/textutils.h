/****************************************************************************
**
** Copyright (C) 2019,2021 Jolla Ltd.
** Copyright (C) 2019,2020 Open Mobile Platform LLC.
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

#include <memory>

QT_BEGIN_NAMESPACE
class QFile;
QT_END_NAMESPACE

namespace Sfdk {

QTextStream &qout();
QTextStream &qerr();

std::unique_ptr<QFile> binaryOut(FILE *out);

bool isConnectedToTerminal();

class Pager
{
public:
    Pager();
    ~Pager();
    operator QTextStream &() { return m_stream; }

    static bool isEnabled() { return s_enabled; }
    static void setEnabled(bool enabled) { s_enabled = enabled; }

private:
    static bool s_enabled;
    QProcess m_pager;
    QTextStream m_stream;
};

class AsciiMan
{
public:
    static const int BOLD_INDENT_CORRECTION = -2;

public:
    AsciiMan(QTextStream &out);
    ~AsciiMan();
    operator QTextStream &() { return m_in; }

public:
    void flush();

    static void render(QTextStream &in, QTextStream &out);

    static void header(QTextStream &out, const QString &title);
    static void section(QTextStream &out, const QString &title);
    static void subsection(QTextStream &out, const QString &title);
    static void nameSection(QTextStream &out);
    static void synopsisSection(QTextStream &out);
    static void verseBegin(QTextStream &out);
    static void verseEnd(QTextStream &out);
    static void labeledListItemBegin(QTextStream &out, const QString &label);
    static void labeledListItemEnd(QTextStream &out);
    static QString boldLeadingWords(const QString &string);

private:
    QByteArray m_buf;
    QTextStream m_in;
    QTextStream &m_out;
};

class LineEndPostprocessingMessageHandler
{
public:
    LineEndPostprocessingMessageHandler();
    ~LineEndPostprocessingMessageHandler();

private:
    static void handler(QtMsgType type, const QMessageLogContext &context, const QString &msg);

    static int counter;
    static QtMessageHandler defaultHandler;
};

QString indent(int level);
void wrapLine(QTextStream &out, int indentLevel, const QString &prefix1, const QString &prefix2,
        const QString &line);
void wrapLine(QTextStream &out, int indentLevel, const QString &line);
void wrapLines(QTextStream &out, int indentLevel, const QStringList &prefix1,
        const QStringList &prefix2, const QString &lines, int indentCorrection = 0);

bool expandCompacted(const QString &string, QStringList *expanded);

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
    static void sort(Tree *tree, int column, bool ascending);
    static void print(QTextStream &out, const Tree &tree, const QList<int> &columns);

private:
    static void print(QTextStream &out, const Item &item,
            const QString &linePrefix, const QList<int> &columns, const QList<int> &widths);
    static TreePrinter::Item *findItem(Tree &tree, const QString &id,
            int idColumn);
    static QList<QStringList> topoSort(const QList<QStringList> &table, int idColumn,
            int parentIdColumn);

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
