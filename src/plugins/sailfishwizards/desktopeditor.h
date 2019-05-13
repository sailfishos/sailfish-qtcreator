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
#include "desktopdocument.h"
#include "desktopeditorwidget.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief This class describes editor of desktop files. It consists of two parts: \b document and \b widget.
 * \sa IEditor, DesktopEditorWidget, DesktopDocument
 */
class DesktopEditor : public Core::IEditor
{
public:
    DesktopEditor();
    ~DesktopEditor();
    Core::IDocument *document() const override;
    QWidget *toolBar() override;
    QWidget *widget() const override;

private:
    DesktopEditorWidget *m_editorWidget;
    Core::IDocument *m_editorDocument;
};

} // namespace Internal
} // namespace SailfishWizards
