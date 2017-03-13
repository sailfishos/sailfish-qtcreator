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

#pragma once

#include "projectexplorer_export.h"

#include <QIcon>
#include <QWidget>

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT PropertiesPanel
{
    Q_DISABLE_COPY(PropertiesPanel)

public:
    PropertiesPanel() = default;
    ~PropertiesPanel() { delete m_widget; }

    QString displayName() const { return m_displayName; }
    QIcon icon() const { return m_icon; }
    QWidget *widget() const { return m_widget; }

    void setDisplayName(const QString &name) { m_displayName = name; }
    void setIcon(const QIcon &icon) { m_icon = icon; }
    void setWidget(QWidget *widget) { m_widget = widget; }

private:
    QString m_displayName;
    QWidget *m_widget = nullptr;
    QIcon m_icon;
};

} // namespace ProjectExplorer
