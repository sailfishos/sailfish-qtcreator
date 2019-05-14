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

#include "spectacledocument.h"
#include <QFile>
#include <QFileInfo>
#include <QDebug>
#include <QSignalMapper>
#include "utils/fileutils.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Constructor for the document object.
 * \param editor Editor associated with this document.
 * \param editorWidget Widget for interacting with the document and changing it's data
 */
SpectacleDocument::SpectacleDocument(IEditor *editor, SpectacleFileEditorWidget *editorWidget)
    : IDocument(editor), m_editorWidget(editorWidget)
{
    setId(Id("SpectacleDocument"));
    connect(editorWidget, &SpectacleFileEditorWidget::contentModified, this, &SpectacleDocument::requestSave);
}

/*!
 * \brief Sets if the document is \a modified.
 * Emits \e changed() signal, when the state of the document changes.
 * \param modified Value for setting to the document's status.
 * \sa isModified()
 */
void SpectacleDocument::setModified(bool modified)
{
    if (m_modified == modified)
        return;
    m_modified = modified;
    emit changed();
}

/*!
 * \brief Returns \c true if document was modified, otherwise returns false.
 */
bool SpectacleDocument::isModified() const
{
    return m_modified;
}

/*!
 * \brief Saves the modified document data. Returns \c true if data were saved, otherwise returns false.
 * \param errorMessage Output parameter for saving message about possible errors.
 * \param fileName Path to the editable file.
 * \param autoSave Flag, which says whether saving happens automatically or not.
 * \c true if saving happens automatically, otherwise \c false.
 */
bool SpectacleDocument::save(QString *errorMessage, const QString &fileName, bool autoSave)
{
    Q_UNUSED(autoSave);
    Q_UNUSED(errorMessage);
    const Utils::FileName fileNameToUse = fileName.isEmpty() ? filePath() : Utils::FileName::fromString(
                                              fileName);
    QFile file(fileNameToUse.toString());
    if (file.open(QFile::WriteOnly)) {
        m_fileName = file.fileName();
        QString content = m_editorWidget->content();
        file.write(content.toLocal8Bit());
        setModified(false);
        setFilePath(fileNameToUse);
        return true;
    }
    return false;
}

/*!
 * \brief Opens the file and initializes widget with the file's data.
 * Returns whether the file was opened and read successfully.
 * \param errorString Output parameter for saving message about possible errors.
 * \param fileName Path to the associated file.
 * \param realFileName Name of the auto save that should be loaded if the editor is opened from an auto save file.
 * It's the same as \a fileName if the editor is opened from a regular file.
 * \sa OpenResult
 */
IDocument::OpenResult SpectacleDocument::open(QString *errorString, const QString &fileName,
                                              const QString &realFileName)
{
    Q_UNUSED(errorString);
    Q_UNUSED(realFileName);
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        m_fileName = file.fileName();
        setFilePath(Utils::FileName::fromString(fileName));
        QString path = QFileInfo(fileName).absolutePath();
        m_editorWidget->setContent(file.readAll());
        setModified(false);
        return OpenResult::Success;
    }
    return OpenResult::ReadError;
}

/*!
 * \brief Reopens the file that is already opened.
 * Returns \c true if the file was opened and read successfully, otherwise returns \c false.
 * \param errorString Output parameter for saving message about possible errors.
 */
bool SpectacleDocument::reload(QString *errorString, ReloadFlag flag, ChangeType type)
{
    Q_UNUSED(flag);
    Q_UNUSED(type);
    m_editorWidget->clearDependencies();
    if (open(errorString, m_fileName, m_fileName) == OpenResult::Success)
        return true;
    else
        return false;
}

/*!
 * \brief This slot sets the modified status to \c true, when the associated signal is emitted.
 * The slot wraps the proper \c setModified() method, because of its non-parameter prototype, which is much more comfortable to use when connecting signals.
 * \sa setModified()
 */
void SpectacleDocument::requestSave()
{
    setModified();
}

} // namespace Internal
} // namespace SailfishWizards
