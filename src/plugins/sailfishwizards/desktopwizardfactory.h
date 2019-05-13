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

# pragma once

#include <coreplugin/basefilewizardfactory.h>
#include <coreplugin/basefilewizard.h>
#include <coreplugin/id.h>
#include "desktopwizardpages.h"

namespace SailfishWizards {
namespace Internal {

using namespace Core;

/*!
 * \brief This class defines the content
 * and operation of the Desktop file creation wizard.
 * \sa BaseFileWizardFactory
 */
class DesktopWizardFactory : public BaseFileWizardFactory
{
public:
    DesktopWizardFactory();
    bool isAvailable(Id platformId) const override;
protected:
    BaseFileWizard *create(QWidget *parent, const WizardDialogParameters &parameters) const override;
    GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;
    bool postGenerateFiles(const QWizard *w, const GeneratedFiles &l,
                           QString *errorMessage) const override;
private:
    QString desktopSuffix() const;
    GeneratedFile generateDesktopFile(const QWizard *wizard, QString *errorMessage) const;
    void addDesktopToProFile(const QWizard *w) const;
    void addIconsToProject(const QWizard *w) const;
    void addDesktopToYamlFile(const QWizard *w) const;
    void addDesktopToSpecFile(const QWizard *w) const;
    void addIconToDesktopFile(const QWizard *w) const;
    enum PageId {LOCATION, OPTIONS, ICONS, SUMMARY};
};

} // namespace Internal
} // namespace SailfishWizards
