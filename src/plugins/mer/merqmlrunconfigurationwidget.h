/****************************************************************************
**
** Copyright (C) 2016-2017 Jolla Ltd.
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

#ifndef MERQMLRUNCONFIGURATIONWIDGET_H
#define MERQMLRUNCONFIGURATIONWIDGET_H

#include <QPointer>
#include <QWidget>

class QLabel;

namespace Utils {
    class DetailsWidget;
}

namespace Mer {
namespace Internal {

class MerQmlRunConfiguration;

class MerQmlRunConfigurationWidget : public QWidget
{
    Q_OBJECT

public:
    MerQmlRunConfigurationWidget(MerQmlRunConfiguration *runConfiguration, QWidget *parent = nullptr);

private:
    const QPointer<MerQmlRunConfiguration> m_runConfiguration;
    QLabel *m_disabledIcon;
    QLabel *m_disabledReason;
    Utils::DetailsWidget *m_detailsContainer;
    QLabel *m_command;
};

} // namespace Internal
} // namespace Mer

#endif // MERQMLRUNCONFIGURATIONWIDGET_H
