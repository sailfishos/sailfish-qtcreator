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

#include "filecontainer.h"

#include <QVector>

namespace ClangBackEnd {

class CMBIPC_EXPORT UnregisterTranslationUnitsForEditorMessage
{
    friend CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const UnregisterTranslationUnitsForEditorMessage &message);
    friend CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, UnregisterTranslationUnitsForEditorMessage &message);
    friend CMBIPC_EXPORT bool operator==(const UnregisterTranslationUnitsForEditorMessage &first, const UnregisterTranslationUnitsForEditorMessage &second);
    friend void PrintTo(const UnregisterTranslationUnitsForEditorMessage &message, ::std::ostream* os);

public:
    UnregisterTranslationUnitsForEditorMessage() = default;
    UnregisterTranslationUnitsForEditorMessage(const QVector<FileContainer> &fileContainers);

    const QVector<FileContainer> &fileContainers() const;

private:
    QVector<FileContainer> fileContainers_;
};

CMBIPC_EXPORT QDataStream &operator<<(QDataStream &out, const UnregisterTranslationUnitsForEditorMessage &message);
CMBIPC_EXPORT QDataStream &operator>>(QDataStream &in, UnregisterTranslationUnitsForEditorMessage &message);
CMBIPC_EXPORT bool operator==(const UnregisterTranslationUnitsForEditorMessage &first, const UnregisterTranslationUnitsForEditorMessage &second);

CMBIPC_EXPORT QDebug operator<<(QDebug debug, const UnregisterTranslationUnitsForEditorMessage &message);
#ifdef CLANGBACKEND_TESTS
void PrintTo(const UnregisterTranslationUnitsForEditorMessage &message, ::std::ostream* os);
#endif

DECLARE_MESSAGE(UnregisterTranslationUnitsForEditorMessage);
} // namespace ClangBackEnd
