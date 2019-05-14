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

#include <QObject>
#include <coreplugin/basefilewizard.h>
#include "ui_jsfileselectpage.h"

namespace SailfishWizards {
namespace Internal {

/*!
 * \brief Main page class for the JS library file wizard.
 */
class FileInfoPage : public QWizardPage
{
    Q_OBJECT
public:
    FileInfoPage(const QString &defaultPath);
    static bool validateInput(bool condition, QWidget *widget, QLabel *errorLabel,
                              const QString &errorMessage);
private:
    bool isComplete() const override;
    void openFileDialog();
    Ui::JSFileSelectPage m_pageUi;
};

} // namespace Internal
} // namespace SailfishWizards
