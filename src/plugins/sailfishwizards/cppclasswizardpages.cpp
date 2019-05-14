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

#include "cppclasswizardpages.h"
#include "sailfishwizardsconstants.h"
#include "fileinfopage.h"

#include <QFileDialog>
#include <QPalette>

namespace SailfishWizards {
namespace Internal {
/*!
 * \brief The constructor of the location page
 * sets the interface and the initial location of the file.
 * \param defaultPath Default file locations
 */
LocationPage::LocationPage(const QString &defaultPath) :
    m_defaultPath(defaultPath)
{
    m_pageUi.setupUi(this);
    m_pageUi.pathEdit->setText(m_defaultPath);

    QObject::connect(m_pageUi.browseButton, &QPushButton::clicked, this, &LocationPage::openFileDialog);
    QObject::connect(m_pageUi.nameEdit, &QLineEdit::textChanged, this, &LocationPage::isComplete);
    QObject::connect(m_pageUi.nameEdit, &QLineEdit::textChanged, this, &LocationPage::setNameFiles);

    registerField("nameClass*", m_pageUi.nameEdit);
    registerField("path*", m_pageUi.pathEdit);
    registerField("nameH*", m_pageUi.hFileEdit);
    registerField("nameCpp*", m_pageUi.cppFileEdit);
}

QString LocationPage::getDefaultPath() const
{
    return m_defaultPath;
}

/*!
 * \brief Automatically sets file names based on class name.
 */
void LocationPage::setNameFiles()
{
    QRegExp rx("::");
    QString name;
    if (field("nameClass").toString().contains("::")) {
        name = field("nameClass").toString().split(rx).last().toLower().remove(".h").remove(" ");
    } else {
        name = field("nameClass").toString().toLower().remove(".h").remove(" ");
    }
    m_pageUi.hFileEdit->setText(name + ".h");
    m_pageUi.cppFileEdit->setText(name + ".cpp");
}

/*!
 * \brief This is an override function from the \c QWizardPage, responsible for moving to the next page.
 * Returns \c true if the input fields are validated; otherwise \c false.
 */
bool LocationPage::isComplete() const
{
    QDir projectDir(field("path").toString());
    QRegExp nameCheck(SailfishWizards::Constants::REG_EXP_FOR_NAME_FILE);
    QRegExp nameClassCheck(SailfishWizards::Constants::REG_EXP_FOR_NAME_CLASS);
    return FileInfoPage::validateInput(nameClassCheck.exactMatch(field("nameClass").toString()),
                                        m_pageUi.nameEdit,
                                        m_pageUi.errorLabel, tr("Invalid characters used when entering a class name."))
            && FileInfoPage::validateInput((QDir(field("path").toString())).exists(), m_pageUi.pathEdit,
                                           m_pageUi.errorLabel, tr("Directory not found"))
            && FileInfoPage::validateInput(nameCheck.exactMatch(field("nameH").toString()), m_pageUi.hFileEdit,
                                           m_pageUi.errorLabel, tr("Invalid characters used when entering a file name."))
            && FileInfoPage::validateInput(!projectDir.entryList().contains(field("nameH").toString())
                                           && !projectDir.entryList().contains(field("nameH").toString() + ".h"),
                                           m_pageUi.hFileEdit, m_pageUi.errorLabel, tr("A file with this name already exists."))
            && FileInfoPage::validateInput(nameCheck.exactMatch(field("nameCpp").toString()),
                                           m_pageUi.cppFileEdit,
                                           m_pageUi.errorLabel, tr("Invalid characters used when entering a file name."))
            && FileInfoPage::validateInput(!projectDir.entryList().contains(field("nameCpp").toString())
                                           && !projectDir.entryList().contains(field("nameCpp").toString() + ".cpp"),
                                           m_pageUi.cppFileEdit, m_pageUi.errorLabel, tr("A file with this name already exists."));
}

/*!
 * \brief Opens a dialog for changing the file location.
 */
void LocationPage::openFileDialog()
{
    QString directory = QFileDialog::getExistingDirectory(this, tr("Select directory"),
                                                          m_pageUi.pathEdit->text(),
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!directory.isEmpty()) {
        m_pageUi.pathEdit->setText(directory);
    }
}

/*!
 * \brief Parameter Page Designer, sets the page interface.
 * \param model A pointer to the model with a list of selected external libraries.
 */
OptionPage::OptionPage(DependencyModel *model)
{
    m_pageUi.setupUi(this);
    m_localList = new QStringList;
    m_modulList = model;
    registerField("baseClass", m_pageUi.baseTypeComboBox, "currentText");
}

/*!
 * \brief Destructs option page.
 */
OptionPage::~OptionPage()
{
    delete m_localList;
}

/*!
 * \brief Returns pointer to the list of external libraries available on this page.
 */
DependencyModel *OptionPage::getModulList()
{
    return m_modulList;
}

/*!
 * \brief The predefined function from QWizardPage,
 * called when the "next" button on the previous page is clicked.
 */
void OptionPage::initializePage()
{
    changeBaseTypeItems();
}

/*!
 * \brief Defines a list of available base classes.
 */
void OptionPage::changeBaseTypeItems()
{
    QString currentItem = m_pageUi.baseTypeComboBox->currentText();
    m_pageUi.baseTypeComboBox->clear();
    m_localList->removeDuplicates();
    QString str;
    QStringList itemsList = {""};
    for (int i = 0; i < m_localList->size(); i++) {
        str = m_localList->value(i);
        if (!str.isEmpty())
            itemsList << QFileInfo(str).baseName();
    }
    for (int j = 0; j < m_modulList->size(); j++) {
        itemsList << m_modulList->getExternalLibraryByIndex(j).getTypesList();
    }
    itemsList << "QObject";
    itemsList.removeDuplicates();
    std::sort(itemsList.begin(), itemsList.end());
    m_pageUi.baseTypeComboBox->addItems(itemsList);
    if (itemsList.indexOf(currentItem) != -1)
        m_pageUi.baseTypeComboBox->setCurrentIndex(itemsList.indexOf(currentItem));
}

} // namespace Internal
} // namespace SailfishWizards
