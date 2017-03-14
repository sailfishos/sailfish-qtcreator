/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "quicktoolbarsettingspage.h"
#include "qmljseditorconstants.h"

#include <qmljstools/qmljstoolsconstants.h>
#include <coreplugin/icore.h>

#include <QSettings>
#include <QTextStream>
#include <QCheckBox>


using namespace QmlJSEditor;
using namespace QmlJSEditor::Internal;

QuickToolBarSettings::QuickToolBarSettings()
    : enableContextPane(false),
    pinContextPane(false)
{}

void QuickToolBarSettings::set()
{
    if (get() != *this)
        toSettings(Core::ICore::settings());
}

void QuickToolBarSettings::fromSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML));
    enableContextPane = settings->value(
            QLatin1String(QmlJSEditor::Constants::QML_CONTEXTPANE_KEY), QVariant(false)).toBool();
    pinContextPane = settings->value(
                QLatin1String(QmlJSEditor::Constants::QML_CONTEXTPANEPIN_KEY), QVariant(false)).toBool();
    settings->endGroup();
}

void QuickToolBarSettings::toSettings(QSettings *settings) const
{
    settings->beginGroup(QLatin1String(QmlJSEditor::Constants::SETTINGS_CATEGORY_QML));
    settings->setValue(QLatin1String(QmlJSEditor::Constants::QML_CONTEXTPANE_KEY), enableContextPane);
    settings->setValue(QLatin1String(QmlJSEditor::Constants::QML_CONTEXTPANEPIN_KEY), pinContextPane);
    settings->endGroup();
}

bool QuickToolBarSettings::equals(const QuickToolBarSettings &other) const
{
    return  enableContextPane == other.enableContextPane
            && pinContextPane == other.pinContextPane;
}


QuickToolBarSettingsPageWidget::QuickToolBarSettingsPageWidget(QWidget *parent) :
    QWidget(parent)
{
    m_ui.setupUi(this);
}

QuickToolBarSettings QuickToolBarSettingsPageWidget::settings() const
{
    QuickToolBarSettings ds;
    ds.enableContextPane = m_ui.textEditHelperCheckBox->isChecked();
    ds.pinContextPane = m_ui.textEditHelperCheckBoxPin->isChecked();
    return ds;
}

void QuickToolBarSettingsPageWidget::setSettings(const QuickToolBarSettings &s)
{
    m_ui.textEditHelperCheckBox->setChecked(s.enableContextPane);
    m_ui.textEditHelperCheckBoxPin->setChecked(s.pinContextPane);
}

QuickToolBarSettings QuickToolBarSettings::get()
{
    QuickToolBarSettings settings;
    settings.fromSettings(Core::ICore::settings());
    return settings;
}

QuickToolBarSettingsPage::QuickToolBarSettingsPage() :
    m_widget(0)
{
    setId("C.QmlToolbar");
    setDisplayName(tr("Qt Quick ToolBar"));
    setCategory(Constants::SETTINGS_CATEGORY_QML);
    setDisplayCategory(QCoreApplication::translate("QmlJSEditor",
        QmlJSEditor::Constants::SETTINGS_TR_CATEGORY_QML));
    setCategoryIcon(Utils::Icon(QmlJSTools::Constants::SETTINGS_CATEGORY_QML_ICON));
}

QWidget *QuickToolBarSettingsPage::widget()
{
    if (!m_widget) {
        m_widget = new QuickToolBarSettingsPageWidget;
        m_widget->setSettings(QuickToolBarSettings::get());
    }
    return m_widget;
}

void QuickToolBarSettingsPage::apply()
{
    if (!m_widget) // page was never shown
        return;
    m_widget->settings().set();
}

void QuickToolBarSettingsPage::finish()
{
    delete m_widget;
}
