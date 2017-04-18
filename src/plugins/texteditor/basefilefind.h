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

#include "texteditor_global.h"
#include "utils/filesearch.h"

#include <coreplugin/find/ifindfilter.h>
#include <coreplugin/find/searchresultwindow.h>

#include <QFuture>

QT_BEGIN_NAMESPACE
class QLabel;
class QComboBox;
QT_END_NAMESPACE

namespace Utils { class FileIterator; }
namespace Core {
class IEditor;
class IFindSupport;
class SearchResult;
class SearchResultItem;
} // namespace Core

namespace TextEditor {

namespace Internal { class BaseFileFindPrivate; }

class TEXTEDITOR_EXPORT FileFindParameters
{
public:
    QString text;
    Core::FindFlags flags;
    QStringList nameFilters;
    QVariant additionalParameters;
    QVariant extensionParameters;
};

class TEXTEDITOR_EXPORT FileFindExtension : public QObject
{
public:
    virtual ~FileFindExtension() {}
    virtual QString title() const = 0;
    virtual QString toolTip() const = 0; // add %1 placeholder where the find flags should be put
    virtual QWidget *widget() const = 0;
    virtual bool isEnabled() const = 0;
    virtual bool isEnabled(const FileFindParameters &parameters) const = 0;
    virtual QVariant parameters() const = 0;
    virtual void readSettings(QSettings *settings) = 0;
    virtual void writeSettings(QSettings *settings) const = 0;
    virtual QFuture<Utils::FileSearchResultList> executeSearch(
            const FileFindParameters &parameters) = 0;
    virtual Core::IEditor *openEditor(const Core::SearchResultItem &item,
                                      const FileFindParameters &parameters) = 0;
};

class TEXTEDITOR_EXPORT BaseFileFind : public Core::IFindFilter
{
    Q_OBJECT

public:
    BaseFileFind();
    ~BaseFileFind();

    bool isEnabled() const;
    bool isReplaceSupported() const { return true; }
    void findAll(const QString &txt, Core::FindFlags findFlags);
    void replaceAll(const QString &txt, Core::FindFlags findFlags);
    void setFindExtension(FileFindExtension *extension);

    /* returns the list of unique files that were passed in items */
    static QStringList replaceAll(const QString &txt,
                                  const QList<Core::SearchResultItem> &items,
                                  bool preserveCase = false);

protected:
    virtual Utils::FileIterator *files(const QStringList &nameFilters,
                                       const QVariant &additionalParameters) const = 0;
    virtual QVariant additionalParameters() const = 0;
    QVariant getAdditionalParameters(Core::SearchResult *search);
    virtual QString label() const = 0; // see Core::SearchResultWindow::startNewSearch
    virtual QString toolTip() const = 0; // see Core::SearchResultWindow::startNewSearch,
                                         // add %1 placeholder where the find flags should be put
    QFuture<Utils::FileSearchResultList> executeSearch(const FileFindParameters &parameters);

    void writeCommonSettings(QSettings *settings);
    void readCommonSettings(QSettings *settings, const QString &defaultFilter);
    QWidget *createPatternWidget();
    void syncComboWithSettings(QComboBox *combo, const QString &setting);
    void updateComboEntries(QComboBox *combo, bool onTop);
    QStringList fileNameFilters() const;
    FileFindExtension *extension() const;

private:
    void displayResult(int index);
    void searchFinished();
    void cancel();
    void setPaused(bool paused);
    void openEditor(const Core::SearchResultItem &item);
    void doReplace(const QString &txt,
                   const QList<Core::SearchResultItem> &items,
                   bool preserveCase);
    void hideHighlightAll(bool visible);
    void searchAgain();
    void recheckEnabled();

    void runNewSearch(const QString &txt, Core::FindFlags findFlags,
                      Core::SearchResultWindow::SearchMode searchMode);
    void runSearch(Core::SearchResult *search);

    Internal::BaseFileFindPrivate *d;
};

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::FileFindParameters)
