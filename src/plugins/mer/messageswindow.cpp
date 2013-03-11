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

#include "messageswindow.h"

#include <QScrollBar>

namespace Mer {
namespace Internal {

// This is a simpler version of Core::OutputWindow

MessagesWindow::MessagesWindow(QWidget *parent)
    : QPlainTextEdit(parent)
    , m_enforceNewline(false)
    , m_scrollToBottom(false)
{
}

void MessagesWindow::grayOutOldContent()
{
    QTextCursor cursor = textCursor();
    cursor.movePosition(QTextCursor::End);
    QTextCharFormat endFormat = cursor.charFormat();

    cursor.select(QTextCursor::Document);

    QTextCharFormat format;
    const QColor bkgColor = palette().base().color();
    const QColor fgdColor = palette().text().color();
    double bkgFactor = 0.50;
    double fgdFactor = 1.-bkgFactor;
    format.setForeground(QColor((bkgFactor * bkgColor.red() + fgdFactor * fgdColor.red()),
                                (bkgFactor * bkgColor.green() + fgdFactor * fgdColor.green()),
                                (bkgFactor * bkgColor.blue() + fgdFactor * fgdColor.blue()) ));
    cursor.mergeCharFormat(format);

    cursor.movePosition(QTextCursor::End);
    cursor.setCharFormat(endFormat);
    cursor.insertBlock(QTextBlockFormat());
}

void MessagesWindow::appendText(const QString &textIn)
{
    QTextCursor cursor = QTextCursor(document());
    cursor.movePosition(QTextCursor::End);
    cursor.beginEditBlock();
    cursor.insertText(doNewlineEnforcement(textIn));

    cursor.endEditBlock();
    scrollToBottom();
}

bool MessagesWindow::isScrollbarAtBottom() const
{
    return verticalScrollBar()->value() == verticalScrollBar()->maximum();
}

QString MessagesWindow::doNewlineEnforcement(const QString &out)
{
    m_scrollToBottom = true;
    QString s = out;
    if (m_enforceNewline) {
        s.prepend(QLatin1Char('\n'));
        m_enforceNewline = false;
    }

    if (s.endsWith(QLatin1Char('\n'))) {
        m_enforceNewline = true; // make appendOutputInline put in a newline next time
        s.chop(1);
    }

    return s;
}

void MessagesWindow::scrollToBottom()
{
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
    // QPlainTextEdit destroys the first calls value in case of multiline
    // text, so make sure that the scroll bar actually gets the value set.
    // Is a noop if the first call succeeded.
    verticalScrollBar()->setValue(verticalScrollBar()->maximum());
}

} // Internal
} // Mer
