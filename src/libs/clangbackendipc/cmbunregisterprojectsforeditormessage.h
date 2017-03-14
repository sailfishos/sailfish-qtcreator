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

#include "clangbackendipc_global.h"

#include <utf8stringvector.h>

#include <QDataStream>

namespace ClangBackEnd {

class CMBIPC_EXPORT UnregisterProjectPartsForEditorMessage
{
public:
    UnregisterProjectPartsForEditorMessage() = default;
    UnregisterProjectPartsForEditorMessage(const Utf8StringVector &projectPartIds)
        : projectPartIds_(projectPartIds)
    {
    }

    const Utf8StringVector &projectPartIds() const
    {
        return projectPartIds_;
    }

    friend QDataStream &operator<<(QDataStream &out, const UnregisterProjectPartsForEditorMessage &message)
    {
        out << message.projectPartIds_;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, UnregisterProjectPartsForEditorMessage &message)
    {
        in >> message.projectPartIds_;

        return in;
    }

    friend bool operator==(const UnregisterProjectPartsForEditorMessage &first, const UnregisterProjectPartsForEditorMessage &second)
    {
        return first.projectPartIds_ == second.projectPartIds_;
    }

private:
    Utf8StringVector projectPartIds_;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const UnregisterProjectPartsForEditorMessage &message);
void PrintTo(const UnregisterProjectPartsForEditorMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(UnregisterProjectPartsForEditorMessage);
} // namespace ClangBackEnd
