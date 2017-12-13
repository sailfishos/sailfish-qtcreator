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

#include <QDataStream>

namespace ClangBackEnd {
namespace V2 {


class SourceLocationContainer
{
public:
    SourceLocationContainer() = default;
    SourceLocationContainer(uint fileHash,
                            uint line,
                            uint column,
                            uint offset)
        : m_fileHash(fileHash),
          m_line(line),
          m_column(column),
          m_offset(offset)
    {
    }

    uint fileHash() const
    {
        return m_fileHash;
    }

    uint line() const
    {
        return m_line;
    }

    uint column() const
    {
        return m_column;
    }

    uint offset() const
    {
        return m_offset;
    }

    friend QDataStream &operator<<(QDataStream &out, const SourceLocationContainer &container)
    {
        out << container.m_fileHash;
        out << container.m_line;
        out << container.m_column;
        out << container.m_offset;

        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, SourceLocationContainer &container)
    {
        in >> container.m_fileHash;
        in >> container.m_line;
        in >> container.m_column;
        in >> container.m_offset;

        return in;
    }

    friend bool operator==(const SourceLocationContainer &first, const SourceLocationContainer &second)
    {
        return !(first != second);
    }

    friend bool operator!=(const SourceLocationContainer &first, const SourceLocationContainer &second)
    {
        return first.m_line != second.m_line
            || first.m_column != second.m_column
            || first.m_fileHash != second.m_fileHash;
    }

    friend bool operator<(const SourceLocationContainer &first,
                          const SourceLocationContainer &second)
    {
        return std::tie(first.m_fileHash, first.m_line, first.m_column)
             < std::tie(second.m_fileHash, second.m_line, second.m_column);
    }

    SourceLocationContainer clone() const
    {
        return SourceLocationContainer(m_fileHash, m_line, m_column, m_offset);
    }

private:
    uint m_fileHash = 0;
    uint m_line = 1;
    uint m_column = 1;
    uint m_offset = 0;
};

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const SourceLocationContainer &container);
std::ostream &operator<<(std::ostream &os, const SourceLocationContainer &container);

} // namespace V2
} // namespace ClangBackEnd

