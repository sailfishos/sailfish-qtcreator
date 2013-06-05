/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include "yamleditorwidget.h"
#include "ui_yamleditorwidget.h"
#include "yamleditor.h"

#include <coreplugin/editormanager/ieditor.h>

namespace Mer {
namespace Internal {

YamlEditorWidget::YamlEditorWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::YamlEditorWidget),
    m_editor(0)
{
    ui->setupUi(this);
}

YamlEditorWidget::~YamlEditorWidget()
{
    delete ui;
}

Core::IEditor *YamlEditorWidget::editor() const
{
    if (!m_editor) {
        m_editor = const_cast<YamlEditorWidget *>(this)->createEditor();
        connect(this, SIGNAL(changed()), m_editor, SIGNAL(changed()));
    }

    return m_editor;
}

YamlEditor *YamlEditorWidget::createEditor()
{
    return new YamlEditor(this);
}

} // Internal
} // Mer
