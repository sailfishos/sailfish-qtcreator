/****************************************************************************
**
** Copyright (C) 2014-2015,2018 Jolla Ltd.
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

#include "mergeneraloptionspage.h"

#include "merconstants.h"
#include "mergeneraloptionswidget.h"
#include "mericons.h"

#include <sfdk/sdk.h>

#include <utils/qtcassert.h>

#include <QCoreApplication>

using namespace Core;
using namespace Sfdk;

namespace Mer {
namespace Internal {

MerGeneralOptionsPage::MerGeneralOptionsPage(QObject *parent)
    : IOptionsPage(parent)
{
    setCategory(Utils::Id(Constants::MER_OPTIONS_CATEGORY));
    setDisplayCategory(Sdk::osVariant());
    setCategoryIcon(Icons::MER_OPTIONS_CATEGORY);
    setId(Utils::Id(Constants::MER_GENERAL_OPTIONS_ID));
    setDisplayName(QCoreApplication::translate("Mer", Constants::MER_GENERAL_OPTIONS_NAME));
}

QWidget *MerGeneralOptionsPage::widget()
{
    if (m_widget)
      return m_widget;

    m_widget = new MerGeneralOptionsWidget();
    if (m_searchKeywords.isEmpty())
        m_searchKeywords = m_widget->searchKeywords();
    return m_widget;
}

void MerGeneralOptionsPage::apply()
{
    QTC_CHECK(m_widget);

    m_widget->store();
}

void MerGeneralOptionsPage::finish()
{
    delete m_widget;
}

bool MerGeneralOptionsPage::matches(const QRegularExpression &regexp) const
{
    return m_searchKeywords.contains(regexp);
}

} // Internal
} // Mer
