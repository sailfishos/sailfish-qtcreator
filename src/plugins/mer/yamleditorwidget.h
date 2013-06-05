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

#ifndef YAMLEDITORWIDGET_H
#define YAMLEDITORWIDGET_H

#include <QWidget>

namespace Core {
class IEditor;
}

namespace Mer {
namespace Internal {

namespace Ui {
class YamlEditorWidget;
}

class YamlEditor;
class YamlEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit YamlEditorWidget(QWidget *parent = 0);
    ~YamlEditorWidget();

    Core::IEditor *editor() const;

signals:
    void changed();

private:
    YamlEditor *createEditor();

private:
    Ui::YamlEditorWidget *ui;
    mutable Core::IEditor *m_editor;
};

} // Internal
} // Mer

#endif // YAMLEDITORWIDGET_H
