/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "qttestconfiguration.h"
#include "qttestconstants.h"
#include "qttestoutputreader.h"
#include "qttestsettings.h"
#include "qttest_utils.h"
#include "../autotestplugin.h"
#include "../itestframework.h"
#include "../testsettings.h"

#include <utils/stringutils.h>

namespace Autotest {
namespace Internal {

TestOutputReader *QtTestConfiguration::outputReader(const QFutureInterface<TestResultPtr> &fi,
                                                    QProcess *app) const
{
    auto qtSettings = dynamic_cast<QtTestSettings *>(framework()->testSettings());
    const QtTestOutputReader::OutputMode mode = qtSettings && qtSettings->useXMLOutput
            ? QtTestOutputReader::XML
            : QtTestOutputReader::PlainText;
    return new QtTestOutputReader(fi, app, buildDirectory(), projectFile(), mode, TestType::QtTest);
}

QStringList QtTestConfiguration::argumentsForTestRunner(QStringList *omitted) const
{
    QStringList arguments;
    if (AutotestPlugin::settings()->processArgs) {
        arguments.append(QTestUtils::filterInterfering(
                             runnable().commandLineArguments.split(' ', Qt::SkipEmptyParts),
                             omitted, false));
    }
    auto qtSettings = dynamic_cast<QtTestSettings *>(framework()->testSettings());
    if (!qtSettings)
        return arguments;
    if (qtSettings->useXMLOutput)
        arguments << "-xml";
    if (!testCases().isEmpty())
        arguments << testCases();

    const QString &metricsOption = QtTestSettings::metricsTypeToOption(qtSettings->metrics);
    if (!metricsOption.isEmpty())
        arguments << metricsOption;

    if (qtSettings->verboseBench)
        arguments << "-vb";

    if (qtSettings->logSignalsSlots)
        arguments << "-vs";

    if (isDebugRunMode() && qtSettings->noCrashHandler)
        arguments << "-nocrashhandler";

    return arguments;
}

Utils::Environment QtTestConfiguration::filteredEnvironment(const Utils::Environment &original) const
{
    return QTestUtils::prepareBasicEnvironment(original);
}

} // namespace Internal
} // namespace Autotest
