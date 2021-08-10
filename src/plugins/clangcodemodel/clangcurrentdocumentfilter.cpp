/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "clangcurrentdocumentfilter.h"

#include "clangeditordocumentprocessor.h"
#include "clangutils.h"

#include <clangsupport/tokeninfocontainer.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <cplusplus/Icons.h>

#include <cpptools/cpptoolsconstants.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/fuzzymatcher.h>
#include <utils/linecolumn.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

#include <QRegularExpression>

using namespace Utils;

namespace ClangCodeModel {
namespace Internal {

ClangCurrentDocumentFilter::ClangCurrentDocumentFilter()
{
    setId(CppTools::Constants::CURRENT_DOCUMENT_FILTER_ID);
    setDisplayName(CppTools::Constants::CURRENT_DOCUMENT_FILTER_DISPLAY_NAME);
    setDefaultShortcutString(".");
    setPriority(High);
    setDefaultIncludedByDefault(false);

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, &Core::EditorManager::currentEditorChanged,
            this, &ClangCurrentDocumentFilter::onCurrentEditorChanged);
    connect(editorManager, &Core::EditorManager::editorAboutToClose,
            this, &ClangCurrentDocumentFilter::onEditorAboutToClose);
}

static QString addType(const QString &signature, const ClangBackEnd::ExtraInfo &extraInfo)
{
    return signature + QLatin1String(" -> ", 4) + extraInfo.typeSpelling.toString();
}

static Core::LocatorFilterEntry makeEntry(Core::ILocatorFilter *filter,
                                          const ClangBackEnd::TokenInfoContainer &info)
{
    const ClangBackEnd::ExtraInfo &extraInfo = info.extraInfo;
    QString displayName = extraInfo.token;
    LineColumn lineColumn(info.line, info.column);
    Core::LocatorFilterEntry entry(filter, displayName, QVariant::fromValue(lineColumn));
    QString extra;
    ClangBackEnd::HighlightingType mainType = info.types.mainHighlightingType;
    if (mainType == ClangBackEnd::HighlightingType::VirtualFunction
            || mainType == ClangBackEnd::HighlightingType::Function
            || mainType == ClangBackEnd::HighlightingType::GlobalVariable
            || mainType == ClangBackEnd::HighlightingType::Field
            || mainType == ClangBackEnd::HighlightingType::QtProperty) {
        displayName = addType(displayName, extraInfo);
        extra = extraInfo.semanticParentTypeSpelling.toString();
    } else {
        extra = extraInfo.typeSpelling.toString();
    }
    entry.displayName = displayName;
    entry.extraInfo = extra;
    entry.displayIcon = CodeModelIcon::iconForType(iconTypeForToken(info));
    return entry;
}

void ClangCurrentDocumentFilter::prepareSearch(const QString &entry)
{
    Q_UNUSED(entry)
    m_preparedPath = m_currentPath;
}

QList<Core::LocatorFilterEntry> ClangCurrentDocumentFilter::matchesFor(
        QFutureInterface<Core::LocatorFilterEntry> &, const QString &entry)
{
    QList<Core::LocatorFilterEntry> goodEntries;
    if (m_preparedPath.isEmpty())
        return goodEntries;

    FuzzyMatcher::CaseSensitivity caseSesitivity = caseSensitivity(entry) == Qt::CaseSensitive
            ? FuzzyMatcher::CaseSensitivity::CaseSensitive
            : FuzzyMatcher::CaseSensitivity::CaseInsensitive;
    const QRegularExpression regexp = FuzzyMatcher::createRegExp(entry, caseSesitivity);
    if (!regexp.isValid())
        return goodEntries;

    ClangEditorDocumentProcessor *processor = ClangEditorDocumentProcessor::get(m_preparedPath);
    if (!processor)
        return goodEntries;

    using TokInfoContainer = ClangBackEnd::TokenInfoContainer;
    const QVector<TokInfoContainer> &infos = processor->tokenInfos();

    for (const TokInfoContainer &info : infos) {
        if (!info.isGlobalDeclaration())
            continue;
        QRegularExpressionMatch match = regexp.match(info.extraInfo.token);
        if (match.hasMatch())
            goodEntries.push_back(makeEntry(this, info));
    }

    return goodEntries;
}

void ClangCurrentDocumentFilter::accept(Core::LocatorFilterEntry selection,
            QString *, int *, int *) const
{
    if (!m_currentEditor)
        return;
    auto lineColumn = qvariant_cast<LineColumn>(selection.internalData);
    Core::EditorManager::openEditorAt(m_currentPath, lineColumn.line,
                                      lineColumn.column - 1);
}

void ClangCurrentDocumentFilter::reset(Core::IEditor *newCurrent, const QString &path)
{
    m_currentEditor = newCurrent;
    m_currentPath = path;
}

void ClangCurrentDocumentFilter::onEditorAboutToClose(Core::IEditor *editorAboutToClose)
{
    if (!editorAboutToClose)
        return;

    if (m_currentEditor == editorAboutToClose)
        reset();
}

void ClangCurrentDocumentFilter::onCurrentEditorChanged(Core::IEditor *newCurrent)
{
    if (newCurrent) {
        Core::IDocument *document = newCurrent->document();
        QTC_ASSERT(document, reset(); return);
        if (auto *textDocument = qobject_cast<TextEditor::TextDocument *>(document)) {
            reset(newCurrent, textDocument->filePath().toString());
            return;
        }
    }
    reset();
}

} // namespace Internal
} // namespace ClangCodeModel
