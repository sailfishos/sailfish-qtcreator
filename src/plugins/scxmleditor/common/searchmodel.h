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

#include "scxmldocument.h"
#include "scxmltag.h"

#include <QAbstractTableModel>

namespace ScxmlEditor {

namespace Common {

class SearchModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    enum AttributeRole {
        FilterRole = Qt::UserRole + 1
    };

    SearchModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    PluginInterface::ScxmlTag *tag(const QModelIndex &ind);
    void setFilter(const QString &filter);
    void setDocument(PluginInterface::ScxmlDocument *document);

private:
    void tagChange(PluginInterface::ScxmlDocument::TagChange change, PluginInterface::ScxmlTag *tag, const QVariant &value);
    void resetModel();

    PluginInterface::ScxmlDocument *m_document = nullptr;
    QVector<PluginInterface::ScxmlTag*> m_allTags;
    QString m_strFilter;
};

} // namespace Common
} // namespace ScxmlEditor
