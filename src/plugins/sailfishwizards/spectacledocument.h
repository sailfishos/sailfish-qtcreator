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
#include "spectaclefileeditorwidget.h"

namespace SailfishWizards {
namespace Internal {
using namespace Core;

/*!
 * \brief SpectacleDocument class is responsible for saving, opening and reloading spectacle yaml file, writing and reading its text content.
 * \sa IDocument
 */
class SpectacleDocument : public IDocument
{
    Q_OBJECT
public:
    SpectacleDocument(IEditor *editor, SpectacleFileEditorWidget *editorWidget);
    bool isModified() const;
    bool save(QString *errorString, const QString &fileName, bool autoSave = false);
    OpenResult open(QString *errorString, const QString &fileName, const QString &realFileName);
    bool reload(QString *errorString, ReloadFlag flag, ChangeType type);

private:
    SpectacleFileEditorWidget *m_editorWidget;
    bool m_modified;
    QString m_fileName;
    void setModified(bool modified = true);

private slots:
    void requestSave();
};

} // namespace Internal
} // namespace SailfishWizards
