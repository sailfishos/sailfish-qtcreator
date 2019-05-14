/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** Qt Creator module for the pages of the QML file creation wizard.
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

#include <coreplugin/basefilewizard.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include <QAbstractListModel>
#include <QStringList>

#include "dependencymodel.h"
#include "ui_cppclassfileselectpage.h"
#include "ui_cppclassoptionpage.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Location page class for the С++ file wizard.
 */
class LocationPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit LocationPage(const QString &defaultPath);

    /*!
      * \brief Return default path.
      */
    QString getDefaultPath() const;

private:
    bool isComplete() const override;
    void setNameFiles();
    void openFileDialog();
    Ui::LocationSelect m_pageUi;
    const QString m_defaultPath;
};

/*!
 * \brief Options page with the choice
           of the base class for the C++ file wizard.
 */
class OptionPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit OptionPage(DependencyModel *model);
    ~OptionPage() override;
    DependencyModel *getModulList();
private:
    Ui::CppClassOptionPage m_pageUi;
    QStringList *m_localList;
    DependencyModel *m_modulList;
    void initializePage() override;
    void changeBaseTypeItems();
    void cleanupPage() override {}
};

} // namespace Internal
} // namespace SailfishWizards
