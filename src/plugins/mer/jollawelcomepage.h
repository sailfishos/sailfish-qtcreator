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

#ifndef MERINTERNALJOLLAWELCOMEPAGE_H
#define MERINTERNALJOLLAWELCOMEPAGE_H

#include <utils/iwelcomepage.h>

QT_BEGIN_NAMESPACE
class QDeclarativeEngine;
QT_END_NAMESPACE

namespace Mer {
namespace Internal {

class JollaWelcomePage : public Utils::IWelcomePage
{
    Q_OBJECT

public:
    JollaWelcomePage();

    QUrl pageLocation() const;
    QString title() const;
    int priority() const;
    void facilitateQml(QDeclarativeEngine *engine);
    Id id() const;
};

} // namespace Internal
} // namespace Mer

#endif // MERINTERNALJOLLAWELCOMEPAGE_H
