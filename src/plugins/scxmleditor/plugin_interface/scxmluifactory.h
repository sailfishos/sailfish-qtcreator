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

#include "mytypes.h"

#include <QObject>

namespace ScxmlEditor {

namespace PluginInterface {

class ISCEditor;
class ScxmlDocument;

class ScxmlUiFactory : public QObject
{
    Q_OBJECT

public:
    ScxmlUiFactory(QObject *parent = nullptr);
    ~ScxmlUiFactory() override;

    // Inform document changes
    void documentChanged(DocumentChangeType type, ScxmlDocument *doc);

    // Inform plugins to refresh
    void refresh();

    // Get registered objects
    QObject *object(const QString &type) const;

    // Register objects
    void unregisterObject(const QString &type, QObject *object);
    void registerObject(const QString &type, QObject *object);

    // Check is active
    bool isActive(const QString &type, const QObject *object) const;

private:
    void initPlugins();

    QVector<ISCEditor*> m_plugins;
    QMap<QString, QObject*> m_objects;
};

} // namespace PluginInterface
} // namespace ScxmlEditor
