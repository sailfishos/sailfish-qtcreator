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

namespace ClangBackEnd {

class CMBIPC_EXPORT ProjectPartsDoNotExistMessage
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const ProjectPartsDoNotExistMessage &message);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, ProjectPartsDoNotExistMessage &message);
    friend CMBIPC_EXPORT bool operator==(const ProjectPartsDoNotExistMessage &first, const ProjectPartsDoNotExistMessage &second);
    friend CMBIPC_EXPORT QDebug operator<<(QDebug debug, const ProjectPartsDoNotExistMessage &message);
    friend void PrintTo(const ProjectPartsDoNotExistMessage &message, ::std::ostream* os);
public:
    ProjectPartsDoNotExistMessage() = default;
    ProjectPartsDoNotExistMessage(const Utf8StringVector &projectPartIds);

    const Utf8StringVector &projectPartIds() const;

private:
    Utf8StringVector projectPartIds_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const ProjectPartsDoNotExistMessage &message);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, ProjectPartsDoNotExistMessage &message);
CMBIPC_EXPORT bool operator==(const ProjectPartsDoNotExistMessage &first, const ProjectPartsDoNotExistMessage &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const ProjectPartsDoNotExistMessage &message);
void PrintTo(const ProjectPartsDoNotExistMessage &message, ::std::ostream* os);

DECLARE_MESSAGE(ProjectPartsDoNotExistMessage)
} // namespace ClangBackEnd
