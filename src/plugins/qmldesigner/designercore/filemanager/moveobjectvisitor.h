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

#include "qmlrewriter.h"

namespace QmlDesigner {
namespace Internal {

class MoveObjectVisitor: public QMLRewriter
{
public:
    MoveObjectVisitor(QmlDesigner::TextModifier &modifier,
                      quint32 objectLocation,
                      const QmlDesigner::PropertyName &targetPropertyName,
                      bool targetIsArrayBinding,
                      quint32 targetParentObjectLocation,
                      const PropertyNameList &propertyOrder);

    bool operator ()(QmlJS::AST::UiProgram *ast);

protected:
    virtual bool visit(QmlJS::AST::UiArrayBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);

private:
    void doMove(const TextModifier::MoveInfo &moveInfo);

private:
    QList<QmlJS::AST::Node *> parents;
    quint32 objectLocation;
    PropertyName targetPropertyName;
    bool targetIsArrayBinding;
    quint32 targetParentObjectLocation;
    PropertyNameList propertyOrder;

    QmlJS::AST::UiProgram *program;
};

} // namespace Internal
} // namespace QmlDesigner
