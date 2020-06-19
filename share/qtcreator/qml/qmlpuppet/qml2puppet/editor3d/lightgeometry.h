/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#ifdef QUICK3D_MODULE

#include <QtQuick3D/private/qquick3dgeometry_p.h>
#include <QtQuick3D/private/qquick3dabstractlight_p.h>

namespace QmlDesigner {
namespace Internal {

class LightGeometry : public QQuick3DGeometry
{
    Q_OBJECT
    Q_PROPERTY(QQuick3DAbstractLight *light READ light WRITE setLight NOTIFY lightChanged)

public:
    LightGeometry();
    ~LightGeometry() override;

    QQuick3DAbstractLight *light() const;

public Q_SLOTS:
    void setLight(QQuick3DAbstractLight *light);

Q_SIGNALS:
    void lightChanged();

protected:
    QSSGRenderGraphObject *updateSpatialNode(QSSGRenderGraphObject *node) override;

private:
    void fillVertexData(QByteArray &vertexData, QByteArray &indexData,
                        QVector3D &minBounds, QVector3D &maxBounds);
    QQuick3DAbstractLight *m_light = nullptr;
};

}
}

QML_DECLARE_TYPE(QmlDesigner::Internal::LightGeometry)

#endif // QUICK3D_MODULE
