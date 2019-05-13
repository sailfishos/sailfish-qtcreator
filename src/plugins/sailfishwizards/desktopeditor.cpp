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

#include "desktopeditor.h"

namespace SailfishWizards {
namespace Internal {

using Core::IDocument;

/*!
 * \brief Constructor for the editor object.
 * Initializes this editor's \b document and \b widget.
 * \sa DesktopEditorWidget, DesktopDocument
 */
DesktopEditor::DesktopEditor()
{
    m_editorWidget = new DesktopEditorWidget;
    m_editorDocument = new DesktopDocument(this, m_editorWidget);
    setWidget(m_editorWidget);
}

/*!
 * \brief Destructor for the editor object.
 * Releases the allocated memory.
 */
DesktopEditor::~DesktopEditor()
{
    delete m_editorWidget;
}

/*!
 * \brief Returns editor's document object.
 * \sa DesktopDocument
 */
IDocument *DesktopEditor::document() const
{
    return m_editorDocument;
}

/*!
 * \brief Returns editor's toolBar.
 * This is reimplemented mandatory function, but we don't need toolBar for our purposes, which is why it returns \c nullptr.
 * \sa IEditor
 */
QWidget *DesktopEditor::toolBar()
{
    return nullptr;
}

/*!
 * \brief Returns editor's widget.
 * \sa DesktopEditorWidget
 */
QWidget *DesktopEditor::widget() const
{
    return m_editorWidget;
}

} // namespace Internal
} // namespace SailfishWizards
