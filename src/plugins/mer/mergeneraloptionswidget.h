/****************************************************************************
**
** Copyright (C) 2014-2015 Jolla Ltd.
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

#ifndef MERGENERALOPTIONSWIDGET_H
#define MERGENERALOPTIONSWIDGET_H

#include <QWidget>

namespace Mer {
namespace Internal {

namespace Ui {
class MerGeneralOptionsWidget;
}

class MerGeneralOptionsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit MerGeneralOptionsWidget(QWidget *parent = 0);
    ~MerGeneralOptionsWidget() override;

    void store();
    QString searchKeywords() const;

private:
    Ui::MerGeneralOptionsWidget *m_ui;
};

} // Internal
} // Mer

#endif // MERGENERALOPTIONSWIDGET_H
