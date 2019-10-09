/****************************************************************************
**
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#include "meremulatoroptionspage.h"

#include "merconstants.h"
#include "meremulatoroptionswidget.h"
#include "merplugin.h"
#include "mericons.h"

#include <utils/qtcassert.h>

#include <QCoreApplication>

using namespace Core;

namespace Mer {
namespace Internal {

MerEmulatorOptionsPage::MerEmulatorOptionsPage(QObject *parent)
    : IOptionsPage(parent)
{
    setCategory(Core::Id(Constants::MER_OPTIONS_CATEGORY));
    setDisplayCategory(QCoreApplication::translate("Mer", Constants::MER_OPTIONS_CATEGORY_TR));
    setCategoryIcon(Icons::MER_OPTIONS_CATEGORY);
    setId(Core::Id(Constants::MER_EMULATOR_OPTIONS_ID));
    setDisplayName(QCoreApplication::translate("Mer", Constants::MER_EMULATOR_OPTIONS_NAME));
}

QWidget *MerEmulatorOptionsPage::widget()
{
    if (m_widget)
      return m_widget;

    m_widget = new MerEmulatorOptionsWidget();
    connect(m_widget.data(), &MerEmulatorOptionsWidget::updateSearchKeys,
            this, &MerEmulatorOptionsPage::onUpdateSearchKeys);
    m_searchKeyWords = m_widget->searchKeyWordMatchString();
    return m_widget;
}

void MerEmulatorOptionsPage::apply()
{
    QTC_CHECK(m_widget);

    m_widget->store();

    // ICore::saveSettingsRequested seems to be only emitted when the Options dialog is closed
    MerPlugin::saveSettings();
}

void MerEmulatorOptionsPage::finish()
{
    delete m_widget;
}

bool MerEmulatorOptionsPage::matches(const QString &key) const
{
    return m_searchKeyWords.contains(key, Qt::CaseInsensitive);
}

void MerEmulatorOptionsPage::onUpdateSearchKeys()
{
    m_searchKeyWords = m_widget->searchKeyWordMatchString();
}

void MerEmulatorOptionsPage::setEmulator(const QUrl &vmUri)
{
    if (m_widget)
        m_widget->setEmulator(vmUri);
}

} // Internal
} // Mer
