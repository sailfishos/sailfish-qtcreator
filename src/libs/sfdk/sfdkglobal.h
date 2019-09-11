/****************************************************************************
**
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

#pragma once

#include <QLoggingCategory>

#include <memory>

#if defined(SFDK_LIBRARY)
#  define SFDK_EXPORT Q_DECL_EXPORT
#else
#  define SFDK_EXPORT Q_DECL_IMPORT
#endif

QT_BEGIN_NAMESPACE
#if QT_VERSION < QT_VERSION_CHECK(5, 12, 0)
// Allow using Qt's own PIMPL related macros with std::unique_ptr
template <typename T> static inline T *qGetPtrHelper(const std::unique_ptr<T> &p) { return p.get(); }
#endif
QT_END_NAMESPACE

namespace Sfdk {
inline namespace Log {

Q_DECLARE_LOGGING_CATEGORY(lib)
Q_DECLARE_LOGGING_CATEGORY(vms)
Q_DECLARE_LOGGING_CATEGORY(vmsQueue)
Q_DECLARE_LOGGING_CATEGORY(engine)

} // namespace Log
} // namespace Sfdk
