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

#include "jollawelcomepage.h"

namespace Mer {
namespace Internal {

JollaWelcomePage::JollaWelcomePage()
{
}

QUrl JollaWelcomePage::pageLocation() const
{
    return QUrl(QLatin1String("qrc:/mer/welcomepage/gettingstarted.qml"));
}

QString JollaWelcomePage::title() const
{
    return tr("Getting Started");
}

int JollaWelcomePage::priority() const
{
    return 1;
}

void JollaWelcomePage::facilitateQml(QDeclarativeEngine *engine)
{
    Q_UNUSED(engine)
}

Utils::IWelcomePage::Id JollaWelcomePage::id() const
{
    return GettingStarted;
}

} // namespace Internal
} // namespace Mer
