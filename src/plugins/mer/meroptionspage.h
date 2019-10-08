/****************************************************************************
**
** Copyright (C) 2012-2015 Jolla Ltd.
** Copyright (C) 2019 Open Mobile Platform LLC.
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

#ifndef MEROPTIONSPAGE_H
#define MEROPTIONSPAGE_H

#include <coreplugin/dialogs/ioptionspage.h>

#include <QtCore/QPointer>

namespace Mer {
namespace Internal {

class MerOptionsWidget;
class MerOptionsPage : public Core::IOptionsPage
{
    Q_OBJECT
public:
    explicit MerOptionsPage(QObject *parent = 0);

    QWidget *widget() override;
    void apply() override;
    void finish() override;
    bool matches(const QString &key) const override;

    void setBuildEngine(const QUrl &vmUri);

private slots:
    void onUpdateSearchKeys();

private:
    QPointer<MerOptionsWidget> m_widget;
    QString m_searchKeyWords;
};

} // Internal
} // Mer

#endif // MEROPTIONSPAGE_H
