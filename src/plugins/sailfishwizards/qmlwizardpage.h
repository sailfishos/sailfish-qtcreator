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
#include "ui_qmllocalimportpage.h"
#include "ui_qmlfileselectpage.h"
#include "ui_qmloptionpage.h"
#include "dependencymodel.h"

#include <QAbstractListModel>
#include <QStringList>
#include <QSortFilterProxyModel>

namespace SailfishWizards {
namespace Internal {

/*!
   \brief Class is the location page class for the QML file wizard.
   \sa QWizardPage
*/
class LocationPageQml : public QWizardPage
{
    Q_OBJECT
public:
    explicit LocationPageQml(const QString &defaultPath);
    /*!
      * \brief Return default path.
      */
    QString getDefaultPath() const;

private:
    bool isComplete() const override;
    void openFileDialog();
    Ui::LocationSelectQml m_pageUi;
    const QString m_defaultPath;
};

/*!
    \brief Class is the class for adding local imports for the QML file wizard.
    \sa QWizardPage
*/
class LocalImportsPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit LocalImportsPage(QStringList *localList);
private:
    QStringList *m_selectList;
    void openFileDialog();
    void openQmldirDialog();
    void openDirDialog();
    void removeImport();
    Ui::QmlLocalImportPage m_pageUi;
};

/*!
    \brief Options page with the choice
           of the base item and the QtQuick version for the QML file wizard.

    \sa QWizardPage
*/
class OptionPageQml : public QWizardPage
{
    Q_OBJECT
public:
    OptionPageQml(QStringList *localList, DependencyModel *selectModel);
    ~OptionPageQml() override;
    DependencyModel *getExternalLibraryList();
    QStringList *getLocalList();
private:
    Ui::QmlOptionPage m_pageUi;
    QStringList *m_localList;
    DependencyModel *m_externalLibraryList;
    void cleanupPage() override {}
    void initializePage() override;
};

} // namespace Internal
} // namespace SailfishWizards
