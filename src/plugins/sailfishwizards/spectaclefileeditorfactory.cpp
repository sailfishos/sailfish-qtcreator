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

#include "spectaclefileeditorfactory.h"
#include "sailfishwizardsconstants.h"
#include "spectaclefileeditor.h"
#include "spectaclefilewizardpages.h"
#include <QDebug>

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Constructor for the editor factory object.
 * Sets the yaml file mimeType and editor display name.
 */
SpectacleFileEditorFactory::SpectacleFileEditorFactory()
{
    setId("SpectacleFileEditor");
    setDisplayName(tr("Spectacle editor"));
    addMimeType(Constants::YAML_MIMETYPE);
}

/*!
 * \brief Returns new spectacle yaml file editor.
 * This is called each time a file is opened with spectacle editor.
 * \sa SpectacleFileEditor
 */
IEditor *SpectacleFileEditorFactory::createEditor()
{
    return new SpectacleFileEditor;
}

} // namespace Internal
} // namespace SailfishWizards
