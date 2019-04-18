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

#pragma once

#include <QWidget>
#include <coreplugin/basefilewizard.h>
#include <coreplugin/icore.h>
#include <projectexplorer/projectexplorer.h>
#include "ui_unittestfileselectpage.h"

namespace SailfishWizards {
namespace Internal {

using namespace Core;
/*!
 * \brief Location page class for the unit test files wizard.
 */
class LocationPageUnitTest : public QWizardPage
{
    Q_OBJECT
public:
    explicit LocationPageUnitTest(const QString &defaultPath);
    bool isComplete() const override;
private:
    void setNameFiles();
    void setNameClass();
    void openFileDialog();
    void openHeaderFileDialog();
    Ui::LocationSelectUnitTest m_pageUi;
    void parseClasses();
};

} // namespace Internal
} // namespace SailfishWizards
