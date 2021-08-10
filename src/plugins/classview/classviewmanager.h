/****************************************************************************
**
** Copyright (C) 2016 Denis Mingulov
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

#include <QObject>
#include <QSharedPointer>
#include <QStandardItem>

#include <cplusplus/CppDocument.h>
#include <utils/id.h>

#include "classviewparsertreeitem.h"

namespace ClassView {
namespace Internal {

class ManagerPrivate;

class Manager : public QObject
{
    Q_OBJECT
public:
    explicit Manager(QObject *parent = nullptr);
    ~Manager() override;
    static Manager *instance();

    bool canFetchMore(QStandardItem *item, bool skipRoot = false) const;
    void fetchMore(QStandardItem *item, bool skipRoot = false);
    bool hasChildren(QStandardItem *item) const;

    void gotoLocation(const QString &fileName, int line = 0, int column = 0);
    void gotoLocations(const QList<QVariant> &locations);
    void setFlatMode(bool flat);
    void onWidgetVisibilityIsChanged(bool visibility);

signals:
    void treeDataUpdate(QSharedPointer<QStandardItem> result);

private:
    void initialize();

    inline bool state() const;
    void setState(bool state);

    ManagerPrivate *d;
};

} // namespace Internal
} // namespace ClassView
