/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "filecontainer.h"

#include <QVector>

namespace ClangBackEnd {

class UnregisterUnsavedFilesForEditorMessage
{
public:
    UnregisterUnsavedFilesForEditorMessage() = default;
    UnregisterUnsavedFilesForEditorMessage(const QVector<FileContainer> &fileContainers)
        : fileContainers_(fileContainers)
    {
    }

    const QVector<FileContainer> &fileContainers() const
    {
        return fileContainers_;
    }

    friend QDataStream &operator<<(QDataStream &out, const UnregisterUnsavedFilesForEditorMessage &message)
    {
        out << message.fileContainers_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, UnregisterUnsavedFilesForEditorMessage &message)
    {
        in >> message.fileContainers_;

        return in;
    }

    friend bool operator==(const UnregisterUnsavedFilesForEditorMessage &first, const UnregisterUnsavedFilesForEditorMessage &second)
    {
        return first.fileContainers_ == second.fileContainers_;
    }


private:
    QVector<FileContainer> fileContainers_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const UnregisterUnsavedFilesForEditorMessage &message);
void PrintTo(const UnregisterUnsavedFilesForEditorMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(UnregisterUnsavedFilesForEditorMessage)
} // namespace ClangBackEnd
