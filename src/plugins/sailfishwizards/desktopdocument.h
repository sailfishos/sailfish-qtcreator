/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** This file is part of Qt Creator.
**
** $QT_BEGIN_LICENSE:LGPL$
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included
** in the packaging of this file. Please review the following information
** to ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/lgpl-2.1.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#pragma once

#include "coreplugin/editormanager/ieditor.h"
#include "coreplugin/idocument.h"
#include "desktopeditorwidget.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief DesktopDocument class is responsible for saving, opening and reloading desktop file, writing and reading its text content.
 * \sa IDocument
 */
class DesktopDocument : public Core::IDocument
{
    Q_OBJECT
public:
    DesktopDocument(Core::IEditor *editor, DesktopEditorWidget *editorWidget);
    bool isModified() const override;
    bool save(QString *errorString, const QString &fileName, bool autoSave = false) override;
    Core::IDocument::OpenResult open(QString *errorString, const QString &fileName, const QString &realFileName) override;
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type) override;

private:
    DesktopEditorWidget *m_editorWidget;
    bool m_modified;
    QString m_fileName;
    QString m_applicationPath;
    QHash<QString, QString> m_oldIcons;
    QHash<QString, QString> m_newIcons;
    int m_iconsIndex;

    void setModified(bool modified = true);
    void addIconFiles() const;
    void writeIconsToPro() const;
    void writeIconsToYaml() const;
    void writeIconsToSpec() const;
    void countIconIndex();

private slots:
    void requestSave();
};

} // namespace Internal
} // namespace SailfishWizards
