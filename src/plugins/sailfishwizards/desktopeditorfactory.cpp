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

#include "desktopeditorfactory.h"
#include "sailfishwizardsconstants.h"
#include "desktopeditor.h"
#include "desktopeditorwidget.h"

namespace SailfishWizards {
namespace Internal {

using Core::IEditor;
/*!
 * \brief Constructor for the editor factory object.
 * Sets the desktop file mimeType and editor display name.
 */
DesktopEditorFactory::DesktopEditorFactory()
{
    setId("DesktopFileEditor");
    setDisplayName(tr("Desktop editor"));
    addMimeType(Constants::DESKTOP_MIMETYPE);
}

/*!
 * \brief Returns new desktop file editor.
 * This is called each time a file is opened with desktop editor.
 * \sa DesktopEditor
 */
IEditor *DesktopEditorFactory::createEditor()
{
    return new DesktopEditor;
}

} // namespace Internal
} // namespace SailfishWizards
