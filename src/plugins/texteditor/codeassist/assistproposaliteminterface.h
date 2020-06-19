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

#include "textdocumentmanipulatorinterface.h"

#include <utils/declarationmacros.h>

QT_BEGIN_NAMESPACE
class QIcon;
class QString;
QT_END_NAMESPACE

#include <QString>

namespace TextEditor {

class TEXTEDITOR_EXPORT AssistProposalItemInterface
{
public:
    // We compare proposals by enum values, be careful changing their values
    enum class ProposalMatch
    {
        Full = 0,
        Exact = 1,
        Prefix = 2,
        Infix = 3,
        None = 4
    };

    AssistProposalItemInterface() = default;
    virtual ~AssistProposalItemInterface() noexcept = default;

    UTILS_DELETE_MOVE_AND_COPY(AssistProposalItemInterface)

    virtual QString text() const = 0;
    virtual bool implicitlyApplies() const = 0;
    virtual bool prematurelyApplies(const QChar &typedCharacter) const = 0;
    virtual void apply(TextDocumentManipulatorInterface &manipulator, int basePosition) const = 0;
    virtual QIcon icon() const = 0;
    virtual QString detail() const = 0;
    virtual bool isKeyword() const { return false; }
    virtual Qt::TextFormat detailFormat() const { return Qt::AutoText; }
    virtual bool isSnippet() const = 0;
    virtual bool isValid() const = 0;
    virtual quint64 hash() const = 0; // it is only for removing duplicates
    virtual bool requiresFixIts() const { return false; }

    inline int order() const { return m_order; }
    inline void setOrder(int order) { m_order = order; }
    inline ProposalMatch proposalMatch() { return m_proposalMatch; }
    inline void setProposalMatch(ProposalMatch match) { m_proposalMatch = match; }

private:
    int m_order = 0;
    ProposalMatch m_proposalMatch = ProposalMatch::None;
};

} // namespace TextEditor
