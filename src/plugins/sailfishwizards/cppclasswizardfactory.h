/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** Qt Creator class for creating a wizard for creating C++ files.
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

namespace SailfishWizards {
namespace Internal {

using namespace Core;

/*!
 * \brief This class defines the content
 * and operation of the C++ file creation wizard.
 * \sa BaseFileWizardFactory
 */
class CppClassWizardFactory: public BaseFileWizardFactory
{
public:
    CppClassWizardFactory();
    bool isAvailable(Id platformId) const override;
protected:
    BaseFileWizard *create(QWidget *parent, const WizardDialogParameters &parameters) const override;
    GeneratedFiles generateFiles(const QWizard *w, QString *errorMessage) const override;
private:
    GeneratedFile generateHFile(const QWizard *wizard, QString *errorMessage, const QString &nameClass,
                                const QStringList &nameSpaceClass) const;
    GeneratedFile generateCppFile(const QWizard *wizard, QString *errorMessage, const QString &nameClass,
                                  const QStringList &nameSpaceClass) const;
    bool postGenerateFiles(const QWizard *w, const GeneratedFiles &l,
                           QString *errorMessage) const override;
    QString hSuffix() const;
    QString cppSuffix() const;
    void parseNameClass(const QString &nameClass, QString &outNameClass, QStringList &outNameSpaceClass) const;
    DependencyModel *loadAllExternalLibraries() const;
    enum PageId {LOCATION, EXTERNAL_LIBRARIES, OPTIONS};
};

} // namespace Internal
} // namespace SailfishWizards
