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

#include <QQuickItem>
#include <QAbstractItemModel>

namespace QmlProfiler {
namespace Internal {

class FlameGraphAttached : public QObject
{
    Q_OBJECT
    Q_PROPERTY(qreal relativeSize READ relativeSize WRITE setRelativeSize
               NOTIFY relativeSizeChanged)
    Q_PROPERTY(qreal relativePosition READ relativePosition WRITE setRelativePosition
               NOTIFY relativePositionChanged)
    Q_PROPERTY(bool dataValid READ isDataValid NOTIFY dataValidChanged)

public:
    FlameGraphAttached(QObject *parent = 0) :
        QObject(parent), m_relativeSize(0), m_relativePosition(0) {}

    Q_INVOKABLE QVariant data(int role) const;

    bool isDataValid() const
    {
        return m_data.isValid();
    }

    qreal relativeSize() const
    {
        return m_relativeSize;
    }

    void setRelativeSize(qreal relativeSize)
    {
        if (relativeSize != m_relativeSize) {
            m_relativeSize = relativeSize;
            emit relativeSizeChanged();
        }
    }

    qreal relativePosition() const
    {
        return m_relativePosition;
    }

    void setRelativePosition(qreal relativePosition)
    {
        if (relativePosition != m_relativePosition) {
            m_relativePosition = relativePosition;
            emit relativePositionChanged();
        }
    }

    void setModelIndex(const QModelIndex &data)
    {
        if (data != m_data) {
            bool validChanged = (data.isValid() != m_data.isValid());
            m_data = data;
            if (validChanged)
                emit dataValidChanged();
            emit dataChanged();
        }
    }

signals:
    void dataChanged();
    void dataValidChanged();
    void relativeSizeChanged();
    void relativePositionChanged();

private:
    QPersistentModelIndex m_data;
    qreal m_relativeSize;
    qreal m_relativePosition;
};

class FlameGraph : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(QQmlComponent* delegate READ delegate WRITE setDelegate NOTIFY delegateChanged)
    Q_PROPERTY(QAbstractItemModel* model READ model WRITE setModel NOTIFY modelChanged)
    Q_PROPERTY(int sizeRole READ sizeRole WRITE setSizeRole NOTIFY sizeRoleChanged)
    Q_PROPERTY(qreal sizeThreshold READ sizeThreshold WRITE setSizeThreshold
               NOTIFY sizeThresholdChanged)
    Q_PROPERTY(int maximumDepth READ maximumDepth WRITE setMaximumDepth
               NOTIFY maximumDepthChanged)
    Q_PROPERTY(int depth READ depth NOTIFY depthChanged)

public:
    FlameGraph(QQuickItem *parent = 0);

    QQmlComponent *delegate() const;
    void setDelegate(QQmlComponent *delegate);

    QAbstractItemModel *model() const;
    void setModel(QAbstractItemModel *model);

    int sizeRole() const;
    void setSizeRole(int sizeRole);

    qreal sizeThreshold() const;
    void setSizeThreshold(qreal sizeThreshold);

    int depth() const;

    int maximumDepth() const
    {
        return m_maximumDepth;
    }

    void setMaximumDepth(int maximumDepth)
    {
        if (maximumDepth != m_maximumDepth) {
            m_maximumDepth = maximumDepth;
            emit maximumDepthChanged();
        }
    }

    static FlameGraphAttached *qmlAttachedProperties(QObject *object);

signals:
    void delegateChanged(QQmlComponent *delegate);
    void modelChanged(QAbstractItemModel *model);
    void sizeRoleChanged(int role);
    void sizeThresholdChanged(qreal threshold);
    void depthChanged(int depth);
    void maximumDepthChanged();

private slots:
    void rebuild();

private:
    QQmlComponent *m_delegate = nullptr;
    QAbstractItemModel *m_model = nullptr;
    int m_sizeRole = 0;
    int m_depth = 0;
    qreal m_sizeThreshold = 0;
    int m_maximumDepth = std::numeric_limits<int>::max();

    int buildNode(const QModelIndex &parentIndex, QObject *parentObject, int depth,
                  int maximumDepth);
    QObject *appendChild(QObject *parentObject, QQuickItem *parentItem, QQmlContext *context,
                         const QModelIndex &childIndex, qreal position, qreal size);
};

} // namespace Internal
} // namespace QmlProfiler

QML_DECLARE_TYPEINFO(QmlProfiler::Internal::FlameGraph, QML_HAS_ATTACHED_PROPERTIES)
