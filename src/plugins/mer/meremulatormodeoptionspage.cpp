/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include "meremulatormodeoptionspage.h"

#include "merconstants.h"
#include "meremulatormodeoptionswidget.h"
#include "mericons.h"

#include <sfdk/sdk.h>

#include <utils/qtcassert.h>

#include <QCoreApplication>

using namespace Core;
using namespace Sfdk;

namespace Mer {
namespace Internal {

MerEmulatorModeOptionsPage::MerEmulatorModeOptionsPage(QObject *parent)
    : IOptionsPage(parent)
{
    setCategory(Utils::Id(Constants::MER_OPTIONS_CATEGORY));
    setDisplayCategory(Sdk::osVariant());
    setCategoryIcon(Icons::MER_OPTIONS_CATEGORY);
    setId(Utils::Id(Constants::MER_EMULATOR_MODE_OPTIONS_ID));
    setDisplayName(QCoreApplication::translate("Mer", Constants::MER_EMULATOR_MODE_OPTIONS_NAME));
}

QWidget *MerEmulatorModeOptionsPage::widget()
{
    if (m_widget)
      return m_widget;

    m_widget = new MerEmulatorModeOptionsWidget();
    if (m_searchKeyWords.isEmpty())
        m_searchKeyWords = m_widget->searchKeywords();
    return m_widget;
}

void MerEmulatorModeOptionsPage::apply()
{
    QTC_CHECK(m_widget);

    m_widget->store();
}

void MerEmulatorModeOptionsPage::finish()
{
    delete m_widget;
}

bool MerEmulatorModeOptionsPage::matches(const QString &key) const
{
    return m_searchKeyWords.contains(key, Qt::CaseInsensitive);
}

} // Internal
} // Mer
