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

#include "curveeditor.h"
#include "curveeditormodel.h"
#include <abstractview.h>

namespace QmlDesigner {

class TimelineWidget;

class CurveEditorView : public AbstractView
{
    Q_OBJECT

public:
    explicit CurveEditorView(QObject *parent = nullptr);
    ~CurveEditorView() override;

public:
    bool hasWidget() const override;

    WidgetInfo widgetInfo() override;

    void modelAttached(Model *model) override;

    void modelAboutToBeDetached(Model *model) override;

    void nodeRemoved(const ModelNode &removedNode,
                     const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;

    void nodeReparented(const ModelNode &node,
                        const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        PropertyChangeFlags propertyChange) override;

    void auxiliaryDataChanged(const ModelNode &node,
                              const PropertyName &name,
                              const QVariant &data) override;

    void instancePropertyChanged(const QList<QPair<ModelNode, PropertyName>> &propertyList) override;

    void variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;

    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;

    void propertiesRemoved(const QList<AbstractProperty> &propertyList) override;

private:
    QmlTimeline activeTimeline() const;

    void updateKeyframes();
    void updateCurrentFrame(const ModelNode &node);
    void updateStartFrame(const ModelNode &node);
    void updateEndFrame(const ModelNode &node);

    void commitKeyframes(TreeItem *item);
    void commitCurrentFrame(int frame);
    void commitStartFrame(int frame);
    void commitEndFrame(int frame);

private:
    bool m_block;
    CurveEditorModel *m_model;
    CurveEditor *m_editor;
};

} // namespace QmlDesigner
