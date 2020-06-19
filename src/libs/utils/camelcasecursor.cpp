/**************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Copyright (C) 2019 Andre Hartmann <aha_1980@gmx.de>
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

#include "camelcasecursor.h"

#include <QLineEdit>
#include <QPlainTextEdit>

template<typename C, typename E>
bool moveCursor(C *cursor, E *edit, QTextCursor::MoveOperation direction, QTextCursor::MoveMode mode);

template<>
bool moveCursor(QTextCursor *cursor, QPlainTextEdit *, QTextCursor::MoveOperation direction,
                QTextCursor::MoveMode mode)
{
    return cursor->movePosition(direction, mode);
}

template<typename C>
bool moveCursor(C *, QLineEdit *edit, QTextCursor::MoveOperation direction, QTextCursor::MoveMode mode)
{
    bool mark = (mode == QTextCursor::KeepAnchor);
    switch (direction) {
    case QTextCursor::Left:
        edit->cursorBackward(mark);
        break;
    case QTextCursor::WordLeft:
        edit->cursorWordBackward(mark);
        break;
    case QTextCursor::Right:
        edit->cursorForward(mark);
        break;
    case QTextCursor::WordRight:
        edit->cursorWordForward(mark);
        break;
    default:
        return false;
    }
    return edit->cursorPosition() > 0 && edit->cursorPosition() < edit->text().size();
}

template<typename C, typename E>
QChar charUnderCursor(C *cursor, E *edit);

template<>
QChar charUnderCursor(QTextCursor *cursor, QPlainTextEdit *edit)
{
    return edit->document()->characterAt(cursor->position());
}

template<typename C>
QChar charUnderCursor(C *, QLineEdit *edit)
{
    const int pos = edit->cursorPosition();
    if (pos < 0 || pos >= edit->text().length())
        return QChar::Null;

    return edit->text().at(pos);
};

template<typename C, typename E>
int position(C *cursor, E *edit);

template<>
int position(QTextCursor *cursor, QPlainTextEdit *)
{
    return cursor->position();
}

template<typename C>
int position(C *, QLineEdit *edit)
{
    return edit->cursorPosition();
}

enum class Input {
    Upper,
    Lower,
    Underscore,
    Space,
    Other
};

template<typename C, typename E>
bool camelCaseLeft(C *cursor, E *edit, QTextCursor::MoveMode mode)
{
    int state = 0;

    if (!moveCursor(cursor, edit, QTextCursor::Left, mode))
        return false;

    for (;;) {
        QChar c = charUnderCursor(cursor, edit);
        Input input = Input::Other;
        if (c.isUpper())
            input = Input::Upper;
        else if (c.isLower() || c.isDigit())
            input = Input::Lower;
        else if (c == '_')
            input = Input::Underscore;
        else if (c.isSpace() && c != QChar::ParagraphSeparator)
            input = Input::Space;
        else
            input = Input::Other;

        switch (state) {
        case 0:
            switch (input) {
            case Input::Upper:
                state = 1;
                break;
            case Input::Lower:
                state = 2;
                break;
            case Input::Underscore:
                state = 3;
                break;
            case Input::Space:
                state = 4;
                break;
            default:
                moveCursor(cursor, edit, QTextCursor::Right, mode);
                return moveCursor(cursor, edit, QTextCursor::WordLeft, mode);
            }
            break;
        case 1:
            switch (input) {
            case Input::Upper:
                break;
            default:
                return moveCursor(cursor, edit, QTextCursor::Right, mode);
                return true;
            }
            break;
        case 2:
            switch (input) {
            case Input::Upper:
                return true;
            case Input::Lower:
                break;
            default:
                return moveCursor(cursor, edit, QTextCursor::Right, mode);
                return true;
            }
            break;
        case 3:
            switch (input) {
            case Input::Underscore:
                break;
            case Input::Upper:
                state = 1;
                break;
            case Input::Lower:
                state = 2;
                break;
            default:
                moveCursor(cursor, edit, QTextCursor::Right, mode);
                return true;
            }
            break;
        case 4:
            switch (input) {
            case Input::Space:
                break;
            case Input::Upper:
                state = 1;
                break;
            case Input::Lower:
                state = 2;
                break;
            case Input::Underscore:
                state = 3;
                break;
            default:
                return moveCursor(cursor, edit, QTextCursor::Right, mode);
                if (position(cursor, edit) == 0)
                    return true;
                return moveCursor(cursor, edit, QTextCursor::WordLeft, mode);
            }
        }

        if (!moveCursor(cursor, edit, QTextCursor::Left, mode))
            return true;
    }
}

template<typename C, typename E>
bool camelCaseRight(C *cursor, E *edit, QTextCursor::MoveMode mark)
{
    int state = 0;

    for (;;) {
        QChar c = charUnderCursor(cursor, edit);
        Input input = Input::Other;
        if (c.isUpper())
            input = Input::Upper;
        else if (c.isLower() || c.isDigit())
            input = Input::Lower;
        else if (c == '_')
            input = Input::Underscore;
        else if (c.isSpace() && c != QChar::ParagraphSeparator)
            input = Input::Space;
        else
            input = Input::Other;

        switch (state) {
        case 0:
            switch (input) {
            case Input::Upper:
                state = 4;
                break;
            case Input::Lower:
                state = 1;
                break;
            case Input::Underscore:
                state = 6;
                break;
            default:
                return moveCursor(cursor, edit, QTextCursor::WordRight, mark);
            }
            break;
        case 1:
            switch (input) {
            case Input::Upper:
                return true;
            case Input::Lower:
                break;
            case Input::Underscore:
                state = 6;
                break;
            case Input::Space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 2:
            switch (input) {
            case Input::Upper:
                break;
            case Input::Lower:
                moveCursor(cursor, edit, QTextCursor::Left, mark);
                return true;
            case Input::Underscore:
                state = 6;
                break;
            case Input::Space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 4:
            switch (input) {
            case Input::Upper:
                state = 2;
                break;
            case Input::Lower:
                state = 1;
                break;
            case Input::Underscore:
                state = 6;
                break;
            case Input::Space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 6:
            switch (input) {
            case Input::Underscore:
                break;
            case Input::Space:
                state = 7;
                break;
            default:
                return true;
            }
            break;
        case 7:
            switch (input) {
            case Input::Space:
                break;
            default:
                return true;
            }
            break;
        }
        if (!moveCursor(cursor, edit, QTextCursor::Right, mark))
            return false;
    }
}

bool CamelCaseCursor::left(QTextCursor *cursor, QPlainTextEdit *edit, QTextCursor::MoveMode mode)
{
    return camelCaseLeft(cursor, edit, mode);
}

bool CamelCaseCursor::left(QLineEdit *edit, QTextCursor::MoveMode mode)
{
    QTextCursor temp;
    return camelCaseLeft(&temp, edit, mode);
}

bool CamelCaseCursor::right(QTextCursor *cursor, QPlainTextEdit *edit, QTextCursor::MoveMode mode)
{
    return camelCaseRight(cursor, edit, mode);
}

bool CamelCaseCursor::right(QLineEdit *edit, QTextCursor::MoveMode mode)
{
    QTextCursor temp;
    return camelCaseRight(&temp, edit, mode);
}
