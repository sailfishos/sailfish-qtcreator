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

#include "qmlwizardpage.h"
#include "fileinfopage.h"
#include "sailfishprojectdependencies.h"
#include "sailfishwizardsconstants.h"

#include <QDir>
#include <QJsonDocument>
#include <QFileDialog>
#include <QFileSystemModel>
#include <QMessageBox>
#include <QTextCodec>
#include <QtAlgorithms>

namespace SailfishWizards {
namespace Internal {

using namespace Core;

/*!
 * \brief Constructor for the local import page, sets the page interface and local import list for layout.
 * \param localList is a pointer to a list of local imports with which to interact.
 */
LocalImportsPage::LocalImportsPage(QStringList *localList)
{
    localList->clear();
    m_selectList = localList;
    m_pageUi.setupUi(this);
    QObject::connect(m_pageUi.addFileButton, &QPushButton::clicked, this,
                     &LocalImportsPage::openFileDialog);
    QObject::connect(m_pageUi.removeButton, &QPushButton::clicked, this,
                     &LocalImportsPage::removeImport);
    QObject::connect(m_pageUi.addDirButton, &QPushButton::clicked, this,
                     &LocalImportsPage::openDirDialog);
}

/*!
 * \brief Opens a dialog for the user to add files to the list of local imports.
 */
void LocalImportsPage::openFileDialog()
{
    QStringList files = QFileDialog::getOpenFileNames(this,
                                                      tr("Select one or more JavaScript files to import"),
                                                      field("path").toString(), tr("JavaScript files (*.js)"));
    if (!files.isEmpty()) {
        m_selectList->append(files);
        m_selectList->removeDuplicates();
        std::sort(m_selectList->begin(), m_selectList->end());
        QDir filedir(field("path").toString());
        m_pageUi.listImports->clear();
        for (int i = 0; i < m_selectList->size(); i++) {
            m_pageUi.listImports->addItem(filedir.relativeFilePath(m_selectList->value(i)));
        }
    }
}

/*!
 * \brief Opens a dialog for the user to add directories to the list of local imports.
 */
void LocalImportsPage::openDirDialog()
{
    QString files = QFileDialog::getExistingDirectory(this,
                                                      tr("Select directory containing QML files to import"),
                                                      field("path").toString());
    if (files == field("path").toString()) {
        QMessageBox message;
        message.setText(
            tr("You do not need to add the target directory to local imports, because it is included by default."));
        message.setIcon(QMessageBox::Icon::Information);
        message.exec();
    } else if (!files.isEmpty()) {
        m_selectList->append(files);
        m_selectList->removeDuplicates();
        std::sort(m_selectList->begin(), m_selectList->end());
        QDir filedir(field("path").toString());
        m_pageUi.listImports->clear();
        for (int i = 0; i < m_selectList->size(); i++) {
            m_pageUi.listImports->addItem(m_selectList->value(i));
        }
    }
}

/*!
 * \brief Deletes the selected local import.
 */
void LocalImportsPage::removeImport()
{
    m_selectList->removeAt(m_pageUi.listImports->currentRow());
    m_pageUi.listImports->takeItem(m_pageUi.listImports->currentRow());
}

/*!
 * \brief The constructor of the location page
 * sets the interface and the initial location of the file.
 *
 * \param defaultPath
 */
LocationPageQml::LocationPageQml(const QString &defaultPath) :
    m_defaultPath(defaultPath)
{
    m_pageUi.setupUi(this);
    m_pageUi.pathEdit->setText(m_defaultPath);
    QObject::connect(m_pageUi.browseButton, &QPushButton::clicked, this,
                     &LocationPageQml::openFileDialog);
    QObject::connect(m_pageUi.nameEdit, &QLineEdit::textChanged, this, &LocationPageQml::isComplete);
    registerField("name*", m_pageUi.nameEdit);
    registerField("path*", m_pageUi.pathEdit);
}

QString LocationPageQml::getDefaultPath() const
{
    return m_defaultPath;
}

/*!
 * \brief This is a override function from the \c QWizardPage, responsible for moving to the next page.
 * Returns \c true if the input fields are validated; otherwise  \c false.
 */
bool LocationPageQml::isComplete() const
{
    QDir projectDir(field("path").toString());
    QRegExp nameCheck(SailfishWizards::Constants::REG_EXP_FOR_NAME_FILE);
    return FileInfoPage::validateInput((QDir(field("path").toString())).exists(), m_pageUi.pathEdit,
                                       m_pageUi.errorLabel, tr("Directory not found"))
           && FileInfoPage::validateInput(nameCheck.exactMatch(field("name").toString()), m_pageUi.nameEdit,
                                          m_pageUi.errorLabel, tr("Invalid characters used when entering a file name."))
           && FileInfoPage::validateInput(!projectDir.entryList().contains(field("name").toString())
                                          && !projectDir.entryList().contains(field("name").toString() + ".qml"),
                                          m_pageUi.nameEdit, m_pageUi.errorLabel, tr("A file with this name already exists."));
}

/*!
 * \brief Opens a dialog for changing the file location.
 */
void LocationPageQml::openFileDialog()
{
    QString directory = QFileDialog::getExistingDirectory(this, tr("Select directory"),
                                                          m_pageUi.pathEdit->text(),
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!directory.isEmpty()) {
        m_pageUi.pathEdit->setText(directory);
    }
}

/*!
 * \brief Constructs option page sets interface,
 * sets the page interface, the QtQuick version, the base item.
 * \param localList
 * \param selectModel
 */
OptionPageQml::OptionPageQml(QStringList *localList, DependencyModel *selectModel)
{
    m_pageUi.setupUi(this);
    m_localList = localList;
    m_externalLibraryList = selectModel;
    m_pageUi.baseTypeComboBox->addItem("Item");
    QStringList strList = {"2.6", "2.5", "2.4", "2.3", "2.2", "2.1", "2.0", "1.0"};
    m_pageUi.VersionComboBox->addItems(strList);
    registerField("versionQtQuick", m_pageUi.VersionComboBox, "currentText");
    registerField("baseType", m_pageUi.baseTypeComboBox, "currentText");
}

/*!
 * \brief Destructs the qml option page. Deletes lists that were created by factory with the wizard.
 */
OptionPageQml::~OptionPageQml()
{
    delete m_localList;
    delete m_externalLibraryList;
}

/*!
 * \brief The predefined function from QWizardPage,
 * called when the "next" button on the previous page is clicked,
 * determines the available basic types.
 */
void OptionPageQml::initializePage()
{
    QString currentItem = m_pageUi.baseTypeComboBox->currentText();
    m_pageUi.baseTypeComboBox->clear();
    m_localList->removeDuplicates();
    QString str, strFile;
    QStringList itemsList, localListWithHomeDir = *m_localList;
    localListWithHomeDir.append(field("path").toString());
    for (int i = 0; i < localListWithHomeDir.size(); i++) {
        str = localListWithHomeDir.value(i);
        if (!str.isEmpty()) {
            if (QDir(str).exists()) {
                QDir dir(str);
                for (QString strTemp : dir.entryList()) {
                    if (!strTemp.contains(".") && !QDir(QDir::toNativeSeparators(str + "/" + strTemp)).exists()) {
                        QFile qmldirFile(QDir::toNativeSeparators(str + "/" + strTemp));
                        qmldirFile.open(QFile::ReadOnly);
                        while (!qmldirFile.atEnd()) {
                            strFile = QTextCodec::codecForName("UTF-8")->toUnicode(qmldirFile.readLine());
                            if (!strFile.remove("\n").isEmpty()) {
                                QRegExp rx(" ");
                                if (strFile.contains(".qml")) {
                                    QStringList query = strFile.split(rx);
                                    itemsList << query.value(0);
                                }
                            }
                        }
                        qmldirFile.close();
                    }
                }
                dir.setNameFilters({"*.qml"});
                for (QString strTemp : dir.entryList()) {
                    itemsList << QFileInfo(strTemp).baseName();
                }
            }
        }
    }
    for (int j = 0; j < m_externalLibraryList->size(); j++) {
        itemsList.append(m_externalLibraryList->getTypesList(j));
    }
    itemsList << "Item";
    itemsList.removeDuplicates();
    std::sort(itemsList.begin(), itemsList.end());
    m_pageUi.baseTypeComboBox->addItems(itemsList);
    if (itemsList.indexOf(currentItem) != -1) {
        m_pageUi.baseTypeComboBox->setCurrentIndex(itemsList.indexOf(currentItem));
    } else {
        m_pageUi.baseTypeComboBox->setCurrentIndex(itemsList.indexOf("Item"));
    }
}

/*!
 * \brief Returns pointer to the options page external library list.
 * \sa DependencyModel
 */
DependencyModel *OptionPageQml::getExternalLibraryList()
{
    return m_externalLibraryList;
}

/*!
 * \brief Returns pointer to the options page local list.
 */
QStringList *OptionPageQml::getLocalList()
{
    return m_localList;
}

} // namespace Internal
} // namespace SailfishWizards
