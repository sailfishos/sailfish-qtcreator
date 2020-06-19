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

#include "importmanagerview.h"
#include "importswidget.h"

#include <rewritingexception.h>
#include <qmldesignerplugin.h>

namespace QmlDesigner {

ImportManagerView::ImportManagerView(QObject *parent)
    : AbstractView(parent)
{
}

ImportManagerView::~ImportManagerView() = default;

bool ImportManagerView::hasWidget() const
{
    return true;
}

WidgetInfo ImportManagerView::widgetInfo()
{
    if (m_importsWidget == nullptr) {
        m_importsWidget = new ImportsWidget;
        connect(m_importsWidget.data(), &ImportsWidget::removeImport, this, &ImportManagerView::removeImport);
        connect(m_importsWidget.data(), &ImportsWidget::addImport, this, &ImportManagerView::addImport);

        if (model())
            m_importsWidget->setImports(model()->imports());
    }

    return createWidgetInfo(m_importsWidget, nullptr, QLatin1String("ImportManager"), WidgetInfo::LeftPane, 1, tr("Import Manager"));
}

void ImportManagerView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    if (m_importsWidget) {
        m_importsWidget->setImports(model->imports());
        m_importsWidget->setPossibleImports(model->possibleImports());
        m_importsWidget->setUsedImports(model->usedImports());
    }
}

void ImportManagerView::modelAboutToBeDetached(Model *model)
{
    if (m_importsWidget) {
        m_importsWidget->removeImports();
        m_importsWidget->removePossibleImports();
        m_importsWidget->removeUsedImports();
    }

    AbstractView::modelAboutToBeDetached(model);
}

void ImportManagerView::importsChanged(const QList<Import> &/*addedImports*/, const QList<Import> &/*removedImports*/)
{
    if (m_importsWidget)
        m_importsWidget->setImports(model()->imports());
}

void ImportManagerView::possibleImportsChanged(const QList<Import> &/*possibleImports*/)
{
    QmlDesignerPlugin::instance()->currentDesignDocument()->updateSubcomponentManager();

    if (m_importsWidget)
        m_importsWidget->setPossibleImports(model()->possibleImports());
}

void ImportManagerView::usedImportsChanged(const QList<Import> &/*usedImports*/)
{
    if (m_importsWidget)
        m_importsWidget->setUsedImports(model()->usedImports());
}

void ImportManagerView::removeImport(const Import &import)
{
    try {
        if (model())
            model()->changeImports({}, {import});
    }
    catch (const RewritingException &e) {
        e.showException();
    }
}

void ImportManagerView::addImport(const Import &import)
{
    try {
        if (model())
            model()->changeImports({import}, {});
    }
    catch (const RewritingException &e) {
        e.showException();
    }

    QmlDesignerPlugin::instance()->currentDesignDocument()->updateSubcomponentManager();
}

} // namespace QmlDesigner
