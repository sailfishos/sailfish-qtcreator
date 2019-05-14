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

#include "spectaclefilewizardpages.h"
#include "sailfishwizardsconstants.h"
#include <QFile>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QtWidgets>
#include <QWidget>
#include <QWizard>
#include <QDialog>
#include <QDir>
#include <QPushButton>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMetaType>

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief The constructor of the location page
 * sets the interface and the initial location of the file.
 * \param defaultPath Default file location.
 * \param parent The parent object instance.
 */
SpectacleFileSelectionPage::SpectacleFileSelectionPage(const QString &defaultPath, QWidget *parent)
    : QWizardPage(parent)
{
    m_pageUi.setupUi(this);
    m_defaultPath = defaultPath;
    m_pageUi.fileNameInput->setText(QDir(findProjectRoot(m_defaultPath)).dirName());
    m_pageUi.pathLineEdit->setText(QDir::toNativeSeparators(findProjectRoot(m_defaultPath) + "/rpm"));
    QObject::connect(m_pageUi.pathButton, &QPushButton::clicked, this,
                     &SpectacleFileSelectionPage::chooseFileDirectory);
    registerField("fileName*", m_pageUi.fileNameInput);
    registerField("filePath*", m_pageUi.pathLineEdit);
}

/*!
 * \brief Looks for the associated with the file project's root directory,
 * which is used for setting initial file's path.
 * Returns project's directory if there is a project, otherwise returns \c defaultPath of the page.
 */
QString SpectacleFileSelectionPage::findProjectRoot(const QString &path)
{
    QDir currentDir(path);
    QFileInfoList dirContent = currentDir.entryInfoList(QStringList() << "*.pro", QDir::Files);
    while (dirContent.isEmpty() && currentDir != QDir::root()) {
        currentDir.cd("..");
        dirContent = currentDir.entryInfoList(QStringList() << "*.pro", QDir::Files);
    }
    return (currentDir == QDir::root()) ? path : currentDir.path();
}

/*!
 * \brief This is an override function from the \c QWizardPage, responsible for moving to the next page.
 * Returns \c true if the input fields are validated; otherwise  \c false.
 */
bool SpectacleFileSelectionPage::isComplete() const
{

    QRegExp nameCheck(Constants::SPEC_FILE_NAME_REG_EXP);
    return SpectacleFileOptionsPage::validateInput(nameCheck.exactMatch(field("fileName").toString()),
                                                   m_pageUi.fileNameInput,
                                                   m_pageUi.errorLabel, tr("Invalid file name."))
           && SpectacleFileOptionsPage::validateInput(!QFile(BaseFileWizardFactory::buildFileName(
                                                                 field("filePath").toString(), field("fileName").toString(), ".yaml")).exists(),
                                                      m_pageUi.fileNameInput, m_pageUi.errorLabel, tr("This file already exists."))
           && SpectacleFileOptionsPage::validateInput(QDir(field("filePath").toString()).exists(),
                                                      m_pageUi.pathLineEdit, m_pageUi.errorLabel, tr("Directory not found."));
}

/*!
 * \brief Opens a dialog for changing the file location.
 */
void SpectacleFileSelectionPage::chooseFileDirectory()
{
    QString dir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    m_defaultPath,
                                                    QFileDialog::ShowDirsOnly
                                                    | QFileDialog::DontResolveSymlinks);
    if (!dir.isEmpty()) m_pageUi.pathLineEdit->setText(dir);
}

/*!
 * \brief The constructor of the options page,
 * sets the interface.
 * \param parent The parent object instance.
 */
SpectacleFileOptionsPage::SpectacleFileOptionsPage(const QString &defaultPath, QWidget *parent)
    : QWizardPage(parent)
{
    m_pageUi.setupUi(this);
    m_defaultPath = defaultPath;
    m_fullLicenses = QStringList{tr("MIT License"), tr("Apache License version 2.0"), tr("2-Clause BSD License"), tr("3-Clause BSD License"),
                                 tr("GNU General Public License version 2"), tr("GNU General Public License version 3"),
                                 tr("GNU Library General Public License version 2"), tr("GNU Library General Public License version 2.1"),
                                 tr("GNU Library General Public License version 3"), tr("All rights reserved")};
    m_shortLicenses = QStringList {"MIT", "Apache-2.0", "BSD-2-Clause", "BSD-3-Clause", "GPL-2.0", "GPL-3.0", "LGPL-2.0", "LGPL-2.1", "LGPL-3.0", "<Custom>"};
    m_pageUi.licenseComboBox->addItems(m_shortLicenses);
    QObject::connect(m_pageUi.licenseComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &SpectacleFileOptionsPage::updateLicenseLine);
    registerField("version*", m_pageUi.versionLineEdit);
    registerField("licenseBox*", m_pageUi.licenseComboBox);
    registerField("licenseEdit*", m_pageUi.licenseLineEdit);
    registerField("URL*", m_pageUi.urlLineEdit);
    registerField("summary*", m_pageUi.summaryLineEdit);
    registerField("description", m_pageUi.descTextEdit, "plainText");
}

/*!
 * \brief This is an override function from the \c QWizardPage, responsible for moving to the next page.
 * Returns \c true if the input fields are validated; otherwise  \c false.
 */
bool SpectacleFileOptionsPage::isComplete() const
{
    QRegExp versionNumber(Constants::SPEC_VERSION_REG_EXP);
    return validateInput(!field("summary").toString().isEmpty(), m_pageUi.summaryLineEdit,
                         m_pageUi.errorLabel, tr("Summary line should not be empty."))
           && validateInput(versionNumber.exactMatch(field("version").toString()), m_pageUi.versionLineEdit,
                            m_pageUi.errorLabel, tr("The version should consist of alternating numbers and points."))
           && validateInput(QUrl(field("URL").toString(), QUrl::StrictMode).isValid(), m_pageUi.urlLineEdit,
                            m_pageUi.errorLabel, tr("URL is not valid."))
           && validateInput(!field("licenseEdit").toString().isEmpty(), m_pageUi.licenseLineEdit,
                            m_pageUi.errorLabel, tr("License line should not be empty."));
}

/*!
 * \brief Checks if the \a condition is correct. If it's not, sets the color of the given \a widget to red,
 * and writes \a errorMessage to the \a errorLabel.
 * Returns \c true if the \a condition is true; otherwise  \c false.
 * \param condition Condition to check.
 * \param widget Widget to highlight if the \a condition is false.
 * \param errorLabel Label to display the \a errorMessage.
 * \param errorMessage Message describing the cause of the validation failure.
 */
bool SpectacleFileOptionsPage::validateInput(bool condition, QWidget *widget, QLabel *errorLabel,
                                             const QString &errorMessage)
{
    QPalette palette;
    if (condition) {
        palette = QPalette();
        errorLabel->clear();
    } else {
        palette.setColor(QPalette::Text, Qt::red);
        errorLabel->setText(errorMessage);
    }
    widget->setPalette(palette);
    return condition;
}

/*!
 * \brief Returns list of short names of the available licenses.
 */
QStringList SpectacleFileOptionsPage::shortLicenses() const
{
    return m_shortLicenses;
}

/*!
 * \brief Returns list of full names of the available licenses.
 */
QStringList SpectacleFileOptionsPage::fullLicenses() const
{
    return m_fullLicenses;
}

/*!
 * \brief This slot updates license line edit according to the option selected in the license ComboBox.
 * \param item Number of the license full name in the full names list needed to write to the line edit.
 */
void SpectacleFileOptionsPage::updateLicenseLine(int item)
{
    m_pageUi.licenseLineEdit->setText(m_fullLicenses[item]);
    if (item == m_shortLicenses.length() - 1)
        m_pageUi.licenseLineEdit->setEnabled(true);
    else
        m_pageUi.licenseLineEdit->setEnabled(false);

}

/*!
 * \brief This is the constructor of the dependency name building dialog, sets the interface.
 * \param parent The parent object instance.
 */
SpectacleFileDependenciesDialog::SpectacleFileDependenciesDialog(QWidget *parent)
    : QDialog(parent)
{
    m_dialogUi.setupUi(this);
    m_dialogUi.buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);
    m_dialogUi.requirementBox->addItems({"", "<=", ">=", "=", "<", ">"});
    updateInterface();
    QObject::connect(m_dialogUi.nameEdit, &QLineEdit::textChanged, this,
                     &SpectacleFileDependenciesDialog::updateInterface);
    QObject::connect(m_dialogUi.requirementBox, &QComboBox::currentTextChanged, this,
                     &SpectacleFileDependenciesDialog::updateInterface);
    QObject::connect(m_dialogUi.epochEdit, &QLineEdit::textChanged, this,
                     &SpectacleFileDependenciesDialog::updateInterface);
    QObject::connect(m_dialogUi.versionEdit, &QLineEdit::textChanged, this,
                     &SpectacleFileDependenciesDialog::updateInterface);
    QObject::connect(m_dialogUi.releaseEdit, &QLineEdit::textChanged, this,
                     &SpectacleFileDependenciesDialog::updateInterface);

    QObject::connect(parent, SIGNAL(editingRequested()), this, SLOT(initialize()));

    QObject::connect(m_dialogUi.requirementBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
                     &SpectacleFileDependenciesDialog::updateLines);
    QObject::connect(this, &SpectacleFileDependenciesDialog::accepted, this, &SpectacleFileDependenciesDialog::saveResult);
    QObject::connect(this, &SpectacleFileDependenciesDialog::rejected, this, &SpectacleFileDependenciesDialog::clearFields);
}

/*!
 * \brief This slot validates user's input data and updates the interface according to it.
 */
void SpectacleFileDependenciesDialog::updateInterface()
{
    QRegExp epochNumber(Constants::SPEC_EPOCH_EXP);
    bool enabled = SpectacleFileOptionsPage::validateInput(!m_dialogUi.nameEdit->text().isEmpty(),
                                                           m_dialogUi.nameEdit,
                                                           m_dialogUi.errorLabel, tr("Invalid dependency name."))
                   && SpectacleFileOptionsPage::validateInput(m_dialogUi.requirementBox->currentIndex() == 0
                                                              || !m_dialogUi.versionEdit->text().isEmpty(),
                                                              m_dialogUi.versionEdit, m_dialogUi.errorLabel, tr("If there is a demand, there must be a version."))
                   && SpectacleFileOptionsPage::validateInput(m_dialogUi.requirementBox->currentIndex() == 0
                                                              || !m_dialogUi.versionEdit->text().contains("-"),
                                                              m_dialogUi.versionEdit, m_dialogUi.errorLabel, tr("Version can not contain \"-\"."))
                   && SpectacleFileOptionsPage::validateInput(m_dialogUi.requirementBox->currentIndex() == 0
                                                              || epochNumber.exactMatch(m_dialogUi.epochEdit->text()),
                                                              m_dialogUi.epochEdit, m_dialogUi.errorLabel, tr("Epoch must consist of numbers."))
                   && SpectacleFileOptionsPage::validateInput(m_dialogUi.requirementBox->currentIndex() == 0
                                                              || !m_dialogUi.releaseEdit->text().contains("-"),
                                                              m_dialogUi.releaseEdit, m_dialogUi.errorLabel, tr("Release can not contain \"-\"."));
    m_dialogUi.buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(enabled);
}

/*!
 * \brief Initializes dialog input fields. This is called when dialog was opened for editing.
 */
void SpectacleFileDependenciesDialog::initialize()
{
    m_dialogUi.nameEdit->setText(m_initializingInfo.getName());
    m_dialogUi.requirementBox->setCurrentIndex(m_initializingInfo.getRequirement());
    m_dialogUi.versionEdit->setText(m_initializingInfo.getVersion());
    m_dialogUi.epochEdit->setText(m_initializingInfo.getEpoch());
    m_dialogUi.releaseEdit->setText(m_initializingInfo.getRelease());
}

/*!
 * \brief Saves entered data about dependency record.
 */
void SpectacleFileDependenciesDialog::saveResult()
{
    m_result = DependencyRecord(m_dialogUi.nameEdit->text(),
                                static_cast<DependencyRecord::Requirement>(m_dialogUi.requirementBox->currentIndex()),
                                m_dialogUi.versionEdit->text(),
                                m_dialogUi.epochEdit->text(),
                                m_dialogUi.releaseEdit->text());
    clearFields();
    emit resultSaved();
}

/*!
 * \brief Clears dialog input fields.
 */
void SpectacleFileDependenciesDialog::clearFields()
{
    m_dialogUi.nameEdit->clear();
    m_dialogUi.epochEdit->clear();
    m_dialogUi.versionEdit->clear();
    m_dialogUi.releaseEdit->clear();
    m_dialogUi.requirementBox->setCurrentIndex(0);
    m_dialogUi.epochEdit->setPalette(QPalette());
    m_dialogUi.versionEdit->setPalette(QPalette());
    m_dialogUi.releaseEdit->setPalette(QPalette());
}

/*!
 * \brief Updates epoch, version and release line according to the requirement selected in the ComboBox.
 * \param item Index of the selected requirement.
 */
void SpectacleFileDependenciesDialog::updateLines(int item)
{
    m_dialogUi.epochEdit->setEnabled(item != 0);
    m_dialogUi.versionEdit->setEnabled(item != 0);
    m_dialogUi.releaseEdit->setEnabled(item != 0);
}


/*!
 * \brief The constructor of the project components page,
 * sets the interface and dependency lists.
 * \param factoryLists Lists of dependencies for editing by this page.
 * \param parent The parent object instance.
 */
SpectacleFileProjectTypePage::SpectacleFileProjectTypePage(QList<DependencyListModel *>
                                                           *factoryLists, QWidget *parent)
    : QWizardPage (parent)
{
    m_pageUi.setupUi(this);
    m_factoryLists = factoryLists;
    m_expertMode = false;
    loadDependenciesFromJson();
    QList<bool> checks;
    for (ProjectComponent component : m_jsonLists) checks.append(false);
    m_tableModel = new CheckComponentTableModel(m_jsonLists, checks);
    m_pageUi.componentsTable->setModel(m_tableModel);
    QObject::connect(m_pageUi.dependenciesDialogButton, &QPushButton::clicked, this,
                     &SpectacleFileProjectTypePage::enableExpertMode);
    QObject::connect(m_tableModel, &CheckComponentTableModel::dataChanged, this,
                     &SpectacleFileProjectTypePage::saveDependencies);
}

/*!
 * \brief Destructs project components page.
 */
SpectacleFileProjectTypePage::~SpectacleFileProjectTypePage()
{
    delete m_tableModel;
    for (ProjectComponent component : m_jsonLists)
        qDeleteAll(component.getDependencies());
}

/*!
 * \brief Initializes project components page. Enables default Sailfish application dependencies.
 */
void SpectacleFileProjectTypePage::initializePage()
{
    m_tableModel->checkByName("Sailfish Application", true);
}

/*!
 * \brief Defines and returns the id of the next page.
 * The next page id depends on whether the expert mode button was pressed or not.
 */
int SpectacleFileProjectTypePage::nextId() const
{
    if (m_expertMode)
        return wizard()->currentId() + 1;
    else
        return wizard()->currentId() + 2;
}

/*!
 * \brief Simulates the \e next button pressure, when the \e expert button was pressed.
 * The difference between this action and regular \e next button pressing is that
 * \c nextId() method will return id of the expert page instead of summary page.
 * \sa nextId()
 */
void SpectacleFileProjectTypePage::enableExpertMode()
{
    m_expertMode = true;
    emit wizard()->button(BaseFileWizard::WizardButton::NextButton)->clicked();
    m_expertMode = false;
}

/*!
 * \brief Modifies low-level dependency lists, when a project component was added or removed.
 * \param modelIndex Object containing the index of the edited component.
 */
void SpectacleFileProjectTypePage::saveDependencies(const QModelIndex &modelIndex)
{
    int index = modelIndex.row();
    if (m_tableModel->getChecks().value(index)) {
        m_currentComponents.append(m_jsonLists[index]);
        QList<DependencyListModel *> models = m_jsonLists[index].getDependencies();
        for (int i = 0; i < 3; ++i) {
            QList<DependencyRecord> records = models[i]->getDependencyList();
            DependencyListModel *model = m_factoryLists->value(i);
            for (DependencyRecord record : records) {
                if (!model->getDependencyList().contains(record))
                    model->appendRecord(record);
            }
        }
    } else {
        m_currentComponents.removeAll(m_jsonLists[index]);
        for (int i = 0; i < 3; ++i) {
            QList<DependencyRecord> delRecords = m_jsonLists[index].getDependencies().value(
                                                     i)->getDependencyList();
            for (DependencyRecord delRecord : delRecords) {
                bool permission = true;
                for (ProjectComponent component : m_currentComponents) {
                    QList<DependencyListModel *> models = component.getDependencies();
                    QList<DependencyRecord> records = models[i]->getDependencyList();
                    if (records.contains(delRecord)) permission = false;
                }
                if (permission) m_factoryLists->value(i)->removeRecord(delRecord);
            }
        }
    }
    emit listsModified();
}

/*!
 * \brief Clears the project component page.
 * This is called when the \e back button is clicked on this page.
 */
void SpectacleFileProjectTypePage::cleanupPage()
{
    for (DependencyListModel *model : *m_factoryLists) model->clearRecords();
    m_tableModel->clearChecks();
    emit listsModified();
}

/*!
 * \brief The constructor of the dependencies expert page,
 * sets the interface and dependency lists.
 * \param factoryLists Lists of dependencies for editing by this page.
 * \param parent The parent object instance.
 */
SpectacleFileDependenciesPage::SpectacleFileDependenciesPage(QList<DependencyListModel *>
                                                             *factoryLists, QWidget *parent)
    : QWizardPage(parent)
{
    m_pageUi.setupUi(this);
    m_dialog = new SpectacleFileDependenciesDialog(this);
    m_factoryLists = factoryLists;
    m_pkgbrModel = factoryLists->value(PKG_BR);
    m_pkgConfigBrModel = factoryLists->value(PKG_CONFIG_BR);
    m_requiresModel = factoryLists->value(REQUIRES);
    m_pageUi.pkgBrList->setModel(m_pkgbrModel);
    m_pageUi.pkgConfigBrList->setModel(m_pkgConfigBrModel);
    m_pageUi.requiresList->setModel(m_requiresModel);

    QObject::connect(m_pageUi.addPkgBr, &QPushButton::clicked,
                     this, &SpectacleFileDependenciesPage::showAddingDialog);
    QObject::connect(m_pageUi.addPkgConfigBr, &QPushButton::clicked,
                     this, &SpectacleFileDependenciesPage::showAddingDialog);
    QObject::connect(m_pageUi.addRequires, &QPushButton::clicked,
                     this, &SpectacleFileDependenciesPage::showAddingDialog);

    QObject::connect(m_pageUi.deletePkgBr, &QPushButton::clicked,
                     this, &SpectacleFileDependenciesPage::removeFromLists);
    QObject::connect(m_pageUi.deletePkgConfigBr, &QPushButton::clicked,
                     this, &SpectacleFileDependenciesPage::removeFromLists);
    QObject::connect(m_pageUi.deleteRequires, &QPushButton::clicked,
                     this, &SpectacleFileDependenciesPage::removeFromLists);

    QObject::connect(m_pageUi.changePkgBr, &QPushButton::clicked,
                     this, &SpectacleFileDependenciesPage::showEditingDialog);
    QObject::connect(m_pageUi.changePkgConfigBr, &QPushButton::clicked,
                     this, &SpectacleFileDependenciesPage::showEditingDialog);
    QObject::connect(m_pageUi.changeRequires, &QPushButton::clicked,
                     this, &SpectacleFileDependenciesPage::showEditingDialog);

    QObject::connect(m_pageUi.pkgBrList, &QListView::clicked,
            this, &SpectacleFileDependenciesPage::enableButtons);
    QObject::connect(m_pageUi.pkgConfigBrList, &QListView::clicked,
            this, &SpectacleFileDependenciesPage::enableButtons);
    QObject::connect(m_pageUi.requiresList, &QListView::clicked,
            this, &SpectacleFileDependenciesPage::enableButtons);

    QObject::connect(m_dialog, &SpectacleFileDependenciesDialog::resultSaved,
                     this, &SpectacleFileDependenciesPage::editLists);
}

/*!
 * \brief Destructs dependency expert page.
 * Deletes shared dependency lists that were created with the wizard.
 */
SpectacleFileDependenciesPage::~SpectacleFileDependenciesPage()
{
    qDeleteAll(*m_factoryLists);
    delete m_factoryLists;
}

/*!
 * \brief Returns copy of dependency list.
 * \param listType Type of the needed list.
 */
QList<DependencyRecord> SpectacleFileDependenciesPage::dependencies(const DependencyListType &listType)
{
    return m_factoryLists->value(listType)->getDependencyList();
}

/*!
 * \brief Initializes dependencies expert page.
 */
void SpectacleFileDependenciesPage::initializePage()
{
    m_pageUi.changePkgBr->setEnabled(false);
    m_pageUi.deletePkgBr->setEnabled(false);

    m_pageUi.changePkgConfigBr->setEnabled(false);
    m_pageUi.deletePkgConfigBr->setEnabled(false);

    m_pageUi.changeRequires->setEnabled(false);
    m_pageUi.deleteRequires->setEnabled(false);

    m_pageUi.pkgBrList->setCurrentIndex(QModelIndex());
    m_pageUi.pkgConfigBrList->setCurrentIndex(QModelIndex());
    m_pageUi.requiresList->setCurrentIndex(QModelIndex());
}

/*!
 * \brief Enables edit and delete buttons, when some dependency was selected.
 */
void SpectacleFileDependenciesPage::enableButtons()
{
    if (sender() == m_pageUi.pkgBrList) {
        m_pageUi.changePkgBr->setEnabled(true);
        m_pageUi.deletePkgBr->setEnabled(true);
    }
    if (sender() == m_pageUi.pkgConfigBrList) {
        m_pageUi.changePkgConfigBr->setEnabled(true);
        m_pageUi.deletePkgConfigBr->setEnabled(true);
    }
    if (sender() == m_pageUi.requiresList) {
        m_pageUi.changeRequires->setEnabled(true);
        m_pageUi.deleteRequires->setEnabled(true);
    }
}

/*!
 * \brief Shows the dialog for adding new dependency to the list.
 * \sa SpectacleFileDependenciesDialog
 */
void SpectacleFileDependenciesPage::showAddingDialog()
{
    m_initiator = static_cast<QPushButton *>(sender());
    m_isEdit = false;
    m_dialog->exec();
}

/*!
 * \brief Shows and initializes the dialog for editing of the selected dependency.
 * \sa SpectacleFileDependenciesDialog
 */
void SpectacleFileDependenciesPage::showEditingDialog()
{
    m_initiator = static_cast<QPushButton *>(sender());
    QListView *editableView = m_pageUi.pkgBrList;
    DependencyListModel *editableModel = m_pkgbrModel;

    if (m_initiator == m_pageUi.changePkgConfigBr) {
        editableView = m_pageUi.pkgConfigBrList;
        editableModel = m_pkgConfigBrModel;
    }
    if (m_initiator == m_pageUi.changeRequires) {
        editableView = m_pageUi.requiresList;
        editableModel = m_requiresModel;
    }
    if (!editableView->currentIndex().isValid()) return;
    m_dialog->setInitializingInfo(editableModel->getDependencyList().value(
                                      editableView->currentIndex().row()));
    m_isEdit = true;
    emit editingRequested();
    m_dialog->exec();
}

/*!
 * \brief Modifies lists when the dialog result was confirmed.
 * \sa SpectacleFileDependenciesDialog
 */
void SpectacleFileDependenciesPage::editLists()
{
    QListView *editableView = m_pageUi.pkgBrList;
    DependencyListModel *editableModel = m_pkgbrModel;
    if (m_initiator == m_pageUi.addPkgConfigBr || m_initiator == m_pageUi.changePkgConfigBr) {
        editableView = m_pageUi.pkgConfigBrList;
        editableModel = m_pkgConfigBrModel;
    }
    if (m_initiator == m_pageUi.addRequires || m_initiator == m_pageUi.changeRequires) {
        editableView = m_pageUi.requiresList;
        editableModel = m_requiresModel;
    }
    QList<DependencyRecord> editableList = editableModel->getDependencyList();
    if (editableList.contains(m_dialog->getResult())) {
        QMessageBox::warning(this, tr("Dependency error"), tr("This Dependency already exists."));
        return;
    }
    if (m_isEdit) {
        editableList.replace(editableView->currentIndex().row(), m_dialog->getResult());
    } else {
        editableList.append(m_dialog->getResult());
    }
    editableModel->setDependencyList(editableList);
    emit listsModified();
}

/*!
 * \brief Removes the selected dependency from the list when the \e delete button was pressed.
 * Updates the interface after removal.
 */
void SpectacleFileDependenciesPage::removeFromLists()
{
    m_initiator = static_cast<QPushButton *>(sender());
    QModelIndex currentIndex;
    if (m_initiator == m_pageUi.deletePkgBr) {
        currentIndex = m_pageUi.pkgBrList->currentIndex();
        m_pkgbrModel->removeRecordAt(currentIndex.row());
        if (currentIndex.row() != 0) m_pageUi.pkgBrList->setCurrentIndex(currentIndex.sibling(
                                                                                 currentIndex.row() - 1, currentIndex.column()));
        if (m_pkgbrModel->getDependencyList().isEmpty()) {
            m_pageUi.changePkgBr->setEnabled(false);
            m_pageUi.deletePkgBr->setEnabled(false);
            m_pageUi.pkgBrList->setCurrentIndex(QModelIndex());
        }
    }
    if (m_initiator == m_pageUi.deletePkgConfigBr) {
        currentIndex = m_pageUi.pkgConfigBrList->currentIndex();
        m_pkgConfigBrModel->removeRecordAt(currentIndex.row());
        if (currentIndex.row() != 0) m_pageUi.pkgConfigBrList->setCurrentIndex(currentIndex.sibling(
                                                                                       currentIndex.row() - 1, currentIndex.column()));
        if (m_pkgConfigBrModel->getDependencyList().isEmpty()) {
            m_pageUi.changePkgConfigBr->setEnabled(false);
            m_pageUi.deletePkgConfigBr->setEnabled(false);
            m_pageUi.pkgConfigBrList->setCurrentIndex(QModelIndex());
        }
    }
    if (m_initiator == m_pageUi.deleteRequires) {
        currentIndex = m_pageUi.requiresList->currentIndex();
        m_requiresModel->removeRecordAt(currentIndex.row());
        if (currentIndex.row() != 0) m_pageUi.requiresList->setCurrentIndex(currentIndex.sibling(
                                                                                    currentIndex.row() - 1, currentIndex.column()));
        if (m_requiresModel->getDependencyList().isEmpty()) {
            m_pageUi.changeRequires->setEnabled(false);
            m_pageUi.deleteRequires->setEnabled(false);
            m_pageUi.requiresList->setCurrentIndex(QModelIndex());
        }
    }
    emit listsModified();
}

/*!
 * \brief Loads project components from the resource file.
 */
void SpectacleFileProjectTypePage::loadDependenciesFromJson()
{
    QString value;
    QFile jsonFile(":/sailfishwizards/data/yaml-dependencies.json");
    jsonFile.open(QIODevice::ReadOnly | QIODevice::Text);
    value += QTextCodec::codecForName("UTF-8")->toUnicode(jsonFile.readAll());
    jsonFile.close();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(value.toUtf8());
    QJsonObject jsonObject = jsonDoc.object();
    for (QJsonObject::Iterator iter = jsonObject.begin(); iter != jsonObject.end(); ++iter) {
        QJsonObject project = iter.value().toObject();
        QJsonObject pkgBrObj = project["PkgBr"].toObject();
        QJsonObject pkgConfigBrObj = project["PkgConfigBr"].toObject();
        QJsonObject requiresObj = project["Requires"].toObject();
        QJsonObject jsonObjects[] = {pkgBrObj, pkgConfigBrObj, requiresObj};
        QList<DependencyListModel *> projectDependencies;
        for (QJsonObject obj : jsonObjects) {
            projectDependencies.append(new DependencyListModel);
            for (QJsonObject::Iterator pkgIter = obj.begin(); pkgIter != obj.end(); ++pkgIter) {
                QJsonObject val = pkgIter.value().toObject();
                QList<QString> requirements = {"", "<=", ">=", "=", "<", ">"};
                DependencyRecord record = DependencyRecord(pkgIter.key(),
                                                           static_cast<DependencyRecord::Requirement>(requirements.indexOf(val["requirement"].toString())),
                                                           val["version"].toString(), val["epoch"].toString(), val["release"].toString());
                projectDependencies.last()->appendRecord(record);
            }
        }
        m_jsonLists.append(ProjectComponent(iter.key(), projectDependencies));
    }
}

} // namespace Internal
} // namespace SailfishWizards
