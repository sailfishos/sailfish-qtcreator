/****************************************************************************
**
** Copyright (C) 2019 Jolla Ltd.
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

#include "sfdkconstants.h"
#include "sfdkglobal.h"

namespace Sfdk {

namespace {

int exitAbnormalCode()
{
    if (qEnvironmentVariableIsEmpty(Constants::EXIT_ABNORMAL_ENV_VAR))
        return Constants::EXIT_ABNORMAL_DEFAULT_CODE;

    bool ok;
    int code = qEnvironmentVariableIntValue(Constants::EXIT_ABNORMAL_ENV_VAR, &ok);
    if (!ok) {
        qWarning(sfdk) << "Value of" << Constants::EXIT_ABNORMAL_ENV_VAR
            << "environment variable is not an integer";
        return Constants::EXIT_ABNORMAL_DEFAULT_CODE;
    }
    if (code < 0 || code > 255) {
        qWarning(sfdk) << "Value of" << Constants::EXIT_ABNORMAL_ENV_VAR
            << "environment variable not in range 0..255";
        return Constants::EXIT_ABNORMAL_DEFAULT_CODE;
    }
    return code;
}

} // namespace anonymous

int SFDK_EXIT_ABNORMAL = exitAbnormalCode();

QProcessEnvironment addQpaPlatformMinimal(const QProcessEnvironment &environment)
{
    QProcessEnvironment environment_ = environment;

    if (!qEnvironmentVariableIsSet(Constants::NO_QPA_PLATFORM_MINIMAL_ENV_VAR))
        environment_.insert("QT_QPA_PLATFORM", "minimal");

    return environment_;
}

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
