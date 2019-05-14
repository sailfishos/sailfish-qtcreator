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

#include <QWizardPage>
#include <QDir>
#include <QSortFilterProxyModel>
#include "spectaclefilewizardfactory.h"
#include "checkcomponenttablemodel.h"
#include "ui_spectacledependencydialog.h"
#include "ui_spectacledependenciespage.h"
#include "ui_spectacleoptionspage.h"
#include "ui_spectaclefileselectpage.h"
#include "ui_spectacleprojecttypepage.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Class is the location page class for the spectacle file wizard.
 * \sa QWizardPage
 */
class SpectacleFileSelectionPage : public QWizardPage
{
    Q_OBJECT
public:
    SpectacleFileSelectionPage(const QString &defaultPath, QWidget *parent = nullptr);
    bool isComplete() const override;
    static QString findProjectRoot(const QString &path);

private:
    Ui::SpectacleFileSelectPage m_pageUi;
    QString m_defaultPath;

private slots:
    void chooseFileDirectory();
};

/*!
 * \brief Class describes options page for the spectacle file wizard with the choice of license, summary, description, URL and version of the file's package.
 * \sa QWizardPage
 */
class SpectacleFileOptionsPage : public QWizardPage
{
    Q_OBJECT
public:
    SpectacleFileOptionsPage(const QString &defaultPath, QWidget *parent = nullptr);
    bool isComplete() const override;
    static bool validateInput(bool condition, QWidget *widget, QLabel *errorLabel,
                              const QString &errorMessage);
    Ui::SpectacleOptionsPage Ui()
    {
        return m_pageUi;
    }
    QStringList shortLicenses() const;
    QStringList fullLicenses() const;

private:
    Ui::SpectacleOptionsPage m_pageUi;
    QString m_defaultPath;
    QStringList m_fullLicenses;
    QStringList m_shortLicenses;

private slots:
    void updateLicenseLine(int);
};

/*!
 * \brief This enum type specifies spectacle file's dependency lists:
 * package names required for build, configuration files names required for build and runtime dependencies.
 */
enum DependencyListType {PKG_BR, PKG_CONFIG_BR, REQUIRES};

/*!
 * \brief Dialog for building dependency record as it should be on the dependency list.
 * \sa QDialog
 */
class SpectacleFileDependenciesDialog : public QDialog
{
    Q_OBJECT
public:
    SpectacleFileDependenciesDialog(QWidget *parent = nullptr);
    const DependencyRecord &getResult() const
    {
        return m_result;
    }
    void setInitializingInfo(const DependencyRecord &record)
    {
        m_initializingInfo = record;
    }

private slots:
    void saveResult();
    void updateLines(int);
    void initialize();
    void updateInterface();
    void clearFields();

signals:
    void versionRequirementSelected(bool);
    void resultSaved();

private:
    Ui::SpectacleDependencyDialog m_dialogUi;
    DependencyRecord m_result;
    DependencyRecord m_initializingInfo;
};

/*!
 * \brief Class describes dependencies expert page for the spectacle file wizard,
 * which allows to manage the low-level dependencies of the project associated with created file.
 * \sa QWizardPage
 */
class SpectacleFileDependenciesPage : public QWizardPage
{
    Q_OBJECT
public:
    SpectacleFileDependenciesPage(QList<DependencyListModel *> *factoryLists, QWidget *parent = nullptr);
    ~SpectacleFileDependenciesPage() override;
    Ui::SpectacleDependenciesPage Ui()
    {
        return m_pageUi;
    }
    QList<DependencyRecord> dependencies(const DependencyListType &listType);

private:
    Ui::SpectacleDependenciesPage m_pageUi;
    SpectacleFileDependenciesDialog *m_dialog;
    DependencyListModel *m_pkgbrModel;
    DependencyListModel *m_pkgConfigBrModel;
    DependencyListModel *m_requiresModel;
    QPushButton *m_initiator;
    bool m_isEdit;
    QList<DependencyListModel *> *m_factoryLists;

protected:
    void initializePage() override;

private slots:
    void showAddingDialog();
    void showEditingDialog();
    void editLists();
    void removeFromLists();
    void enableButtons();

signals:
    void editingRequested();
    void listsModified();
};

/*!
 * \brief Class describes dependencies page for the spectacle file wizard,
 * which allows you to select the necessary for the development and launch of the project software components.
 * \sa QWizardPage
 */
class SpectacleFileProjectTypePage : public QWizardPage
{
    Q_OBJECT
public:
    SpectacleFileProjectTypePage(QList<DependencyListModel *> *factoryLists, QWidget *parent = nullptr);
    ~SpectacleFileProjectTypePage() override;
    void setFactoryLists(DependencyListModel *pkgBrList, DependencyListModel *pkgBrConfigList,
                         DependencyListModel *requiresList);
    int nextId() const override;
    CheckComponentTableModel *getTableModel()
    {
        return m_tableModel;
    }
    Ui::SpectacleProjectTypePage Ui()
    {
        return m_pageUi;
    }
    QList<ProjectComponent> getJsonLists() const
    {
        return m_jsonLists;
    }

private:
    Ui::SpectacleProjectTypePage m_pageUi;
    QList<DependencyListModel *> *m_factoryLists;
    bool m_expertMode;
    QList<ProjectComponent> m_jsonLists;
    CheckComponentTableModel *m_tableModel;
    QList<ProjectComponent> m_currentComponents;
    void loadDependenciesFromJson();

protected:
    void cleanupPage() override;
    void initializePage() override;

private slots:
    void enableExpertMode();
    void saveDependencies(const QModelIndex &modelIndex);

signals:
    void listsModified();
};

} // namespace Internal
} // namespace SailfishWizards
