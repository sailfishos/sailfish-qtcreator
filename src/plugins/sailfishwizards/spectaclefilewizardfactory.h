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

#include <coreplugin/basefilewizardfactory.h>
#include <coreplugin/basefilewizard.h>
#include "dependencylistmodel.h"

namespace SailfishWizards {
namespace Internal {

using namespace Core;
/*!
 * \brief This class defines the content
 * and operation of the spectacle yaml file creation wizard.
 * \sa BaseFileWizardFactory
 */
class SpectacleFileWizardFactory : public BaseFileWizardFactory
{
public:
    SpectacleFileWizardFactory();
    bool isAvailable(Id platformId) const override;

protected:
    BaseFileWizard *create(QWidget *parent, const WizardDialogParameters &parameters) const override;
    GeneratedFiles generateFiles(const QWizard *wizard, QString *errorMessage) const override;

private:
    QString yamlSuffix() const;
    GeneratedFile generateFile(const QWizard *wizard, QString *errorMessage) const;
    enum PageId {LOCATION, OPTIONS, COMPONENTS, DEPENDENCIES, SUMMARY};
};

} // namespace Internal
} // namespace SailfishWizards
