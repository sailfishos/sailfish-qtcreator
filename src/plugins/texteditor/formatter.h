/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#pragma once

#include "refactoringchanges.h"

#include <utils/changeset.h>

#include <QFutureWatcher>
#include <QString>

QT_BEGIN_NAMESPACE
class QChar;
class QTextCursor;
QT_END_NAMESPACE

namespace TextEditor {

class TabSettings;

class Formatter
{
public:
    Formatter() = default;
    virtual ~Formatter() = default;

    virtual QFutureWatcher<Utils::ChangeSet> *format(
        const QTextCursor & /*cursor*/, const TextEditor::TabSettings & /*tabSettings*/)
    {
        return nullptr;
    }

    virtual bool isElectricCharacter(const QChar & /*ch*/) const { return false; }
    virtual bool supportsAutoFormat() const { return false; }
    virtual QFutureWatcher<Utils::ChangeSet> *autoFormat(
        const QTextCursor & /*cursor*/, const TextEditor::TabSettings & /*tabSettings*/)
    {
        return nullptr;
    }

    virtual bool supportsFormatOnSave() const { return false; }
    virtual QFutureWatcher<Utils::ChangeSet> *formatOnSave(
        const QTextCursor & /*cursor*/, const TextEditor::TabSettings & /*tabSettings*/)
    {
        return nullptr;
    }
};

} // namespace TextEditor
