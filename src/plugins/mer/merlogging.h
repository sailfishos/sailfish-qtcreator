/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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

#ifndef MERLOGGING_H
#define MERLOGGING_H

#include <QLoggingCategory>

namespace Mer {
namespace Log {

Q_DECLARE_LOGGING_CATEGORY(qmlLive)
Q_DECLARE_LOGGING_CATEGORY(sdks)
Q_DECLARE_LOGGING_CATEGORY(vms)
Q_DECLARE_LOGGING_CATEGORY(vmsQueue)

} // namespace Log
} // namespace Mer

#endif // MERLOGGING_H
