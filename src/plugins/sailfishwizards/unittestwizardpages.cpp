/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC
** Contact: https://community.omprussia.ru/open-source
**
** Qt Creator module for the pages of the unit test file creation wizard.
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

#include "unittestwizardpages.h"
#include "sailfishwizardsconstants.h"
#include "fileinfopage.h"
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QPalette>
#include <QTextCodec>

namespace SailfishWizards {
namespace Internal {
/*!
 * \brief The constructor of the location page
 * sets the interface and the initial location of the file.
 * \param defaultPath Default path.
 */
LocationPageUnitTest::LocationPageUnitTest(const QString &defaultPath)
{
    m_pageUi.setupUi(this);
    QString path = defaultPath;
    if (QDir(QDir::toNativeSeparators(defaultPath + "/test")).exists()) {
        path = QDir::toNativeSeparators(defaultPath + "/test");
    } else {
        if (QDir(QDir::toNativeSeparators(defaultPath + "/test")).exists()) {
            path = QDir::toNativeSeparators(defaultPath + "/test");
        }
    }
    m_pageUi.pathEdit->setText(path);

    QObject::connect(m_pageUi.browseButton, &QPushButton::clicked, this,
                     &LocationPageUnitTest::openFileDialog);
    QObject::connect(m_pageUi.nameEdit, &QLineEdit::textChanged, this,
                     &LocationPageUnitTest::isComplete);
    QObject::connect(m_pageUi.nameEdit, &QLineEdit::textChanged, this,
                     &LocationPageUnitTest::setNameFiles);
    QObject::connect(m_pageUi.addFileButton, &QPushButton::clicked, this,
                     &LocationPageUnitTest::openHeaderFileDialog);
    QObject::connect(m_pageUi.headerFileSelectEdit, &QLineEdit::textChanged, this,
                     &LocationPageUnitTest::parseClasses);
    QObject::connect(m_pageUi.classComboBox, &QComboBox::currentTextChanged, this,
                     &LocationPageUnitTest::setNameClass);

    registerField("nameClass*", m_pageUi.nameEdit);
    registerField("path*", m_pageUi.pathEdit);
    registerField("nameH*", m_pageUi.hFileEdit);
    registerField("nameCpp*", m_pageUi.cppFileEdit);
    registerField("selectHeaderFile", m_pageUi.headerFileSelectEdit);
    registerField("testClassName", m_pageUi.classComboBox, "currentText");
}

/*!
 * \brief Sets the class name.
 */
void LocationPageUnitTest::setNameClass()
{
    m_pageUi.nameEdit->setText("Test" + field("testClassName").toString());
}

/*!
 * \brief Sets the class file names.
 */
void LocationPageUnitTest::setNameFiles()
{
    m_pageUi.hFileEdit->setText(field("nameClass").toString().toLower().remove(".h") + ".h");
    m_pageUi.cppFileEdit->setText(field("nameClass").toString().toLower().remove(".cpp") + ".cpp");
}

/*!
 * \brief This is a override function from the \c QWizardPage, responsible for moving to the next page.
 * Returns \c true if the input fields are validated; otherwise  \c false.
 */
bool LocationPageUnitTest::isComplete() const
{
    QDir projectDir(field("path").toString());
    QRegExp nameCheck(SailfishWizards::Constants::REG_EXP_FOR_NAME_FILE);
    return FileInfoPage::validateInput(QFile(field("selectHeaderFile").toString()).exists(),
                                       m_pageUi.headerFileSelectEdit,
                                       m_pageUi.errorLabel, tr("Header file not found."))
           && FileInfoPage::validateInput((QDir(field("path").toString())).exists(), m_pageUi.pathEdit,
                                          m_pageUi.errorLabel, tr("Directory not found"))
           && FileInfoPage::validateInput(nameCheck.exactMatch(field("nameH").toString()), m_pageUi.hFileEdit,
                                          m_pageUi.errorLabel, tr("Invalid characters in the file name."))
           && FileInfoPage::validateInput(!projectDir.entryList().contains(field("nameH").toString())
                                          && !projectDir.entryList().contains(field("nameH").toString() + ".h"),
                                          m_pageUi.hFileEdit, m_pageUi.errorLabel, tr("A file with this name already exists."))
           && FileInfoPage::validateInput(nameCheck.exactMatch(field("nameCpp").toString()),
                                          m_pageUi.cppFileEdit,
                                          m_pageUi.errorLabel, tr("Invalid characters in the file name."))
           && FileInfoPage::validateInput(!projectDir.entryList().contains(field("nameCpp").toString())
                                          && !projectDir.entryList().contains(field("nameCpp").toString() + ".cpp"),
                                          m_pageUi.cppFileEdit, m_pageUi.errorLabel, tr("A file with this name already exists."));
}

/*!
 * \brief Opens a dialog for changing the file location.
 */
void LocationPageUnitTest::openFileDialog()
{
    const QString directory = QFileDialog::getExistingDirectory(this, tr("Select directory"),
                                                          m_pageUi.pathEdit->text(),
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!directory.isEmpty()) {
        m_pageUi.pathEdit->setText(directory);
    }
}

/*!
 * \brief Opens a dialog to select a file.
 */
void LocationPageUnitTest::openHeaderFileDialog()
{
    const QString files = QFileDialog::getOpenFileName(this, tr("Select one"),
                                                 field("path").toString(), tr("Files (*.h)"));
    if (!files.isEmpty()) {
        m_pageUi.headerFileSelectEdit->setText(files);
    }
}

/*!
 * \brief Defines available classes in file.
 */
void LocationPageUnitTest::parseClasses()
{
    QString projectDir =  QDir::toNativeSeparators(QFileInfo(
                                                       m_pageUi.headerFileSelectEdit->text()).path()
                                                   + "/test");
    if (QDir(projectDir).exists()) {
        m_pageUi.pathEdit->setText(projectDir);
    } else {
        projectDir =  QDir::toNativeSeparators(QFileInfo(m_pageUi.headerFileSelectEdit->text()).path() +
                                               "/tests");
        if (QDir(projectDir).exists()) {
            m_pageUi.pathEdit->setText(projectDir);
        }
    }
    m_pageUi.classComboBox->clear();
    QStringList classes;
    QString headerFile = field("selectHeaderFile").toString(), str;
    if (QFile(headerFile).exists()) {
        QFile file(headerFile);
        file.open(QFile::ReadOnly);
        while (!file.atEnd()) {
            str = QTextCodec::codecForName("UTF-8")->toUnicode(file.readLine());
            if (!str.remove("\n").isEmpty() && str.contains("class ")) {
                QRegExp rx(" ");
                QStringList query = str.split(rx);
                if (query.contains("class")) {
                    QString strClass = query.value(1);
                    classes << strClass.remove(":").remove("{").remove(" ");
                }
            }
        }
    }
    m_pageUi.classComboBox->addItems(classes);
}

} // namespace Internal
} // namespace SailfishWizards
