/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** Qt Creator class for creating a wizard for creating QML files.
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

#include <coreplugin/basefilewizardfactory.h>
#include <coreplugin/basefilewizard.h>
#include "dependencymodel.h"
#include "qmlwizardpage.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief This class defines the content
 * and operation of the QML file creation wizard.
 * \sa BaseFileWizardFactory
 */
class QmlWizardFactory : public Core::BaseFileWizardFactory
{
public:
    QmlWizardFactory();
    bool isAvailable(Core::Id platformId) const override;

protected:
    Core::BaseFileWizard *create(QWidget *parent, const Core::WizardDialogParameters &parameters) const override;
    Core::GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;
    bool postGenerateFiles(const QWizard *w, const Core::GeneratedFiles &l,
                           QString *errorMessage) const override;
private:
    QString qmlSuffix() const;
    DependencyModel *loadAllExternalLibraries() const;
    enum PageId {LOCATION, LOCAL_IMPORTS, EXTERNAL_LIBRARIES, OPTIONS};
};

} // namespace Internal
} // namespace SailfishWizards
