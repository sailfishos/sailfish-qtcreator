
/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Design Tooling
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

#include "animationcurve.h"

#include <QIcon>
#include <QString>

#include <vector>

QT_BEGIN_NAMESPACE
class QIcon;
class QVariant;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeTreeItem;
class PropertyTreeItem;

class TreeItem
{
public:
    using Path = std::vector<QString>;

    virtual bool implicitlyLocked() const { return false; }

    virtual bool implicitlyPinned() const { return false; }

public:
    TreeItem(const QString &name);

    virtual ~TreeItem();

    virtual QIcon icon() const;

    virtual NodeTreeItem *asNodeItem();

    virtual PropertyTreeItem *asPropertyItem();

    unsigned int id() const;

    QString name() const;

    Path path() const;

    bool hasChildren() const;

    bool locked() const;

    bool pinned() const;

    bool compare(const std::vector<QString> &path) const;

    int row() const;

    int column() const;

    int rowCount() const;

    int columnCount() const;

    TreeItem *root() const;

    TreeItem *parent() const;

    TreeItem *child(int row) const;

    TreeItem *find(unsigned int id) const;

    TreeItem *find(const QString &id) const;

    std::vector<TreeItem *> children() const;

    QVariant data(int column) const;

    QVariant headerData(int column) const;

    bool operator==(unsigned int id) const;

    void setId(unsigned int &id);

    void addChild(TreeItem *child);

    void setLocked(bool locked);

    void setPinned(bool pinned);

protected:
    QString m_name;

    unsigned int m_id;

    bool m_locked;

    bool m_pinned;

    TreeItem *m_parent;

    std::vector<TreeItem *> m_children;
};

class NodeTreeItem : public TreeItem
{
public:
    NodeTreeItem(const QString &name, const QIcon &icon, const std::vector<QString> &parentIds);

    NodeTreeItem *asNodeItem() override;

    bool implicitlyLocked() const override;

    bool implicitlyPinned() const override;

    QIcon icon() const override;

    std::vector<PropertyTreeItem *> properties() const;

private:
    QIcon m_icon;

    std::vector<QString> m_parentIds;
};

class PropertyTreeItem : public TreeItem
{
public:
    enum class Component { Generic, R, G, B, A, X, Y, Z, W };

    enum class ValueType {
        Undefined,
        Bool,
        Integer,
        Double,
    };

public:
    PropertyTreeItem(const QString &name, const AnimationCurve &curve, const ValueType &type);

    PropertyTreeItem *asPropertyItem() override;

    bool implicitlyLocked() const override;

    bool implicitlyPinned() const override;

    const NodeTreeItem *parentNodeTreeItem() const;

    ValueType valueType() const;

    Component component() const;

    AnimationCurve curve() const;

    bool hasUnified() const;

    QString unifyString() const;

    void setCurve(const AnimationCurve &curve);

    void setComponent(const Component &comp);

private:
    using TreeItem::addChild;

    ValueType m_type = ValueType::Undefined;

    Component m_component = Component::Generic;

    AnimationCurve m_curve;
};

std::string toString(PropertyTreeItem::ValueType type);

} // End namespace QmlDesigner.
