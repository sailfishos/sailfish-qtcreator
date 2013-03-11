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

#ifndef MER_MESSAGESWINDOW_H
#define MER_MESSAGESWINDOW_H

#include <QPlainTextEdit>

namespace Mer {
namespace Internal {

class MessagesWindow : public QPlainTextEdit
{
public:
    explicit MessagesWindow(QWidget *parent = 0);
    void grayOutOldContent();
    void appendText(const QString &textIn);

private:
    bool isScrollbarAtBottom() const;
    QString doNewlineEnforcement(const QString &out);
    void scrollToBottom();

private:
    bool m_enforceNewline;
    bool m_scrollToBottom;
};

} // Internal
} // Mer

#endif // MER_MESSAGESWINDOW_H
