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

#include "fileinfopage.h"
#include <QFileDialog>

#include "sailfishwizardsconstants.h"
#include <coreplugin/basefilewizardfactory.h>


namespace SailfishWizards {
namespace Internal {

using namespace Core;

/*!
 * \brief The constructor of the page
 * sets the interface and the initial location of the file.
 * \param defaultPath Default file locations
 */
FileInfoPage::FileInfoPage(const QString &defaultPath)
{
    m_pageUi.setupUi(this);
    m_pageUi.pathLineEdit->setText(defaultPath);

    QObject::connect(m_pageUi.pathButton, &QPushButton::clicked, this, &FileInfoPage::openFileDialog);
    QObject::connect(m_pageUi.fileNameInput, &QLineEdit::textChanged, this, &FileInfoPage::isComplete);

    registerField("name*", m_pageUi.fileNameInput);
    registerField("path*", m_pageUi.pathLineEdit);
    registerField("pragma", m_pageUi.pragmaInsertCheckBox);
}

/*!
 * \brief This is an override function from the \c QWizardPage, responsible for moving to the next page.
 * Returns \c true if the input fields are validated; otherwise \c false.
 */
bool FileInfoPage::isComplete() const
{
    QRegExp nameCheck(SailfishWizards::Constants::REG_EXP_FOR_NAME_FILE);
    return validateInput(nameCheck.exactMatch(field("name").toString()), m_pageUi.fileNameInput,
                         m_pageUi.errorLabel, tr("Invalid file name."))
           && validateInput(!QFile(BaseFileWizardFactory::buildFileName(field("path").toString(),
                                                                        field("name").toString(), ".js")).exists(),
                            m_pageUi.fileNameInput, m_pageUi.errorLabel, tr("This file already exists."))
           && validateInput(QDir(field("path").toString()).exists(), m_pageUi.pathLineEdit,
                            m_pageUi.errorLabel,
                            tr("Directory not found."));
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
bool FileInfoPage::validateInput(bool condition, QWidget *widget, QLabel *errorLabel,
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
 * \brief Opens a dialog for changing the file location.
 */
void FileInfoPage::openFileDialog()
{
    QString directory = QFileDialog::getExistingDirectory(this, tr("Select directory"),
                                                          m_pageUi.pathLineEdit->text(),
                                                          QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    if (!directory.isEmpty())
        m_pageUi.pathLineEdit->setText(directory);
}

} // namespace Internal
} // namespace SailfishWizards
