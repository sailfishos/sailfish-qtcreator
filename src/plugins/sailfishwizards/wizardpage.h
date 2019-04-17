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

#include <coreplugin/basefilewizard.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include "ui_externallibrarydialog.h"
#include "ui_externallibrarypage.h"
#include "ui_choicefiledialog.h"
#include "dependencymodel.h"

namespace SailfishWizards {
namespace Internal {
/*!
 * \brief External libraries page for QML and C ++ wizard.
 */
class ExternalLibraryPage : public QWizardPage
{
    Q_OBJECT
public:
    ExternalLibraryPage(DependencyModel *allModel, DependencyModel *selectModel);
    ~ExternalLibraryPage();

private:
    DependencyModel *m_allExternalLibrary;
    DependencyModel *m_selectExternalLibrary;
    QDialog *m_dialog;
    Ui::CppClassModulePage m_pageUi;
    Ui::DialogModule m_dialogUi;
    void openExternalLibraryeDialog();
    void addExternalLibrarye();
    void removeExternalLibrarye();
};

/*!
 * \brief File selection dialog.
 */
class ChoiceFileDialog: public QDialog
{
    Q_OBJECT
public:
    ChoiceFileDialog(QWidget *parent = nullptr);
    int getIndexFile(QStringList *fileList);

private:
    Ui::DialogChoiceFile m_dialogUi;
};

} // namespace Internal
} // namespace SailfishWizards
