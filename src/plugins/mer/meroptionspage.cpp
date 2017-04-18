/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
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

#include "meroptionspage.h"

#include "merconstants.h"
#include "meroptionswidget.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>

using namespace Core;

namespace Mer {
namespace Internal {

MerOptionsPage::MerOptionsPage(QObject *parent)
    : IOptionsPage(parent)
{
    setCategory(Core::Id(Constants::MER_OPTIONS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("Mer", Constants::MER_OPTIONS_CATEGORY_TR));
    setCategoryIcon(Utils::Icon(Constants::MER_OPTIONS_CATEGORY_ICON));
    setId(Core::Id(Constants::MER_OPTIONS_ID));
    setDisplayName(QCoreApplication::translate("Mer", Constants::MER_OPTIONS_NAME));
}

QWidget *MerOptionsPage::widget()
{
    if (m_widget)
      return m_widget;

    m_widget = new MerOptionsWidget();
    connect(m_widget.data(), &MerOptionsWidget::updateSearchKeys,
            this, &MerOptionsPage::onUpdateSearchKeys);
    m_searchKeyWords = m_widget->searchKeyWordMatchString();
    return m_widget;
}

void MerOptionsPage::apply()
{
    QTC_CHECK(m_widget);

    m_widget->store();
}

void MerOptionsPage::finish()
{
    delete m_widget;
}

bool MerOptionsPage::matches(const QString &key) const
{
    return m_searchKeyWords.contains(key, Qt::CaseInsensitive);
}

void MerOptionsPage::onUpdateSearchKeys()
{
    m_searchKeyWords = m_widget->searchKeyWordMatchString();
}

void MerOptionsPage::setSdk(const QString &vmName)
{
    if (m_widget)
        m_widget->setSdk(vmName);
}

} // Internal
} // Mer
