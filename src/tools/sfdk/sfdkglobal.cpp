/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "sfdkglobal.h"

namespace Sfdk {
namespace Log {

/*
 * Conventions:
 *
 * - Output that is the expected outcome of execution
 *
 *   Use qout() with i18n
 *
 * - Debugging messages
 *
 *   Use qCDebug() w/o i18n
 *
 * - Internal errors
 *   - Installation errors
 *   - Errors with data files not meant for direct editing
 *
 *   Use qCWarning()/qCCritical() w/o i18n
 *
 * - Environment errors
 *   - Errors originating from other programs
 *
 *   Use qCWarning()/qCCritical() with i18n
 *
 * - Informational messages
 *   - Displayed unless '--quiet' is used
 *
 *   Use qCInfo() with i18n
 *
 * - Bad usage errors
 *
 *   Use qerr() with i18n
 *
 * - Failure summary
 *   - There should be just one such a message during execution, ideally the last output of the
 *     program
 *   - May cover previously issued internal/environment error messages
 *
 *   Use qerr() with i18n
 */

Q_LOGGING_CATEGORY(sfdk, "sfdk", QtInfoMsg)

} // namespace Log
} // namespace Sfdk
