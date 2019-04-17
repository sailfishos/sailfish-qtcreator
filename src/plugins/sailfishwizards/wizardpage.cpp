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

#include "wizardpage.h"

namespace SailfishWizards {
namespace Internal {
/*!
 * \brief The constructor sets the model of all external libraries and selected external libraries.
 * \param allModel All external libraries.
 * \param selectModel Selected libraries.
 */
ExternalLibraryPage::ExternalLibraryPage(DependencyModel *allModel, DependencyModel *selectModel)
{
    selectModel->clear();
    m_allExternalLibrary = new DependencyModel;
    m_dialog = new QDialog;
    for (int i = 0; i < allModel->size(); i++)
        m_allExternalLibrary->addExternalLibrary(allModel->getExternalLibraryByIndex(i));

    m_dialogUi.setupUi(m_dialog);
    m_dialogUi.dialogListModule->setModel(m_allExternalLibrary);
    m_selectExternalLibrary = selectModel;
    connect(m_dialogUi.addButton, &QPushButton::clicked, this,
            &ExternalLibraryPage::addExternalLibrarye);
    connect(m_dialogUi.cancelButton, &QPushButton::clicked, m_dialog, &QDialog::close);
    m_pageUi.setupUi(this);
    m_pageUi.listModul->setModel(m_selectExternalLibrary);
    connect(m_pageUi.AddModuleButton, &QPushButton::clicked, this,
            &ExternalLibraryPage::openExternalLibraryeDialog);
    connect(m_pageUi.RemoveModuleButton, &QPushButton::clicked, this,
            &ExternalLibraryPage::removeExternalLibrarye);
}

/*!
 * \brief Opens a dialog to select external libraries.
 */
void ExternalLibraryPage::openExternalLibraryeDialog()
{
    m_dialog->exec();
}

/*!
 * \brief Adds the selected library to the list of selected.
 */
void ExternalLibraryPage::addExternalLibrarye()
{
    QModelIndex index = m_dialogUi.dialogListModule->currentIndex();
    if (!(index.row() < 0 || index.row() > m_allExternalLibrary->size())) {
        ExternalLibrary newExternalLibrary = m_allExternalLibrary->getExternalLibrary(index);
        m_selectExternalLibrary->addExternalLibrary(newExternalLibrary);
    }
    m_allExternalLibrary->removeRows(index.row(), 1);
    m_dialog->close();
}

/*!
 * \brief Removes the selected library from the selected list.
 */
void ExternalLibraryPage::removeExternalLibrarye()
{
    QModelIndex index = m_pageUi.listModul->currentIndex();
    if (!(index.row() < 0 || index.row() > m_selectExternalLibrary->size())) {
        ExternalLibrary newExternalLibrary = m_selectExternalLibrary->getExternalLibrary(index);
        m_allExternalLibrary->addExternalLibrary(newExternalLibrary);
    }
    m_selectExternalLibrary->removeRows(index.row(), 1);
}

/*!
 * \brief External library destructor
 */
ExternalLibraryPage::~ExternalLibraryPage()
{
    delete m_allExternalLibrary;
    delete m_dialog;
}

/*!
 * \brief Constructor dialog for file selection, sets the design of the dialogue.
 * \param parent element
 * \param parent widget for file selection dialog
 */
ChoiceFileDialog::ChoiceFileDialog(QWidget *parent)
    : QDialog(parent)
{
    m_dialogUi.setupUi(this);
    connect(m_dialogUi.applyButton, &QPushButton::clicked, this, &QDialog::close);
}

/*!
 * \brief Returns the index of the selected item from the file list.
 * \param fileList list of files.
 */
int ChoiceFileDialog::getIndexFile(QStringList *fileList)
{
    m_dialogUi.comboBoxFiles->clear();
    m_dialogUi.comboBoxFiles->addItems(*fileList);
    exec();
    return m_dialogUi.comboBoxFiles->currentIndex();
}

} // namespace Internal
} // namespace SailfishWizards
