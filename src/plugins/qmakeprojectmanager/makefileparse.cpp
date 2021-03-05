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

#include "makefileparse.h"

#include <qtsupport/qtversionmanager.h>
#include <qtsupport/baseqtversion.h>
#include <utils/qtcprocess.h>

#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace Utils;;

using QtSupport::QtVersionManager;
using QtSupport::BaseQtVersion;

namespace QmakeProjectManager {
namespace Internal {

static QString findQMakeLine(const QString &makefile, const QString &key)
{
    QFile fi(makefile);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        QTextStream ts(&fi);
        while (!ts.atEnd()) {
            const QString line = ts.readLine();
            if (line.startsWith(key))
                return line;
        }
    }
    return QString();
}

/// This function trims the "#Command /path/to/qmake" from the line
static QString trimLine(const QString &line)
{

    // Actually the first space after #Command: /path/to/qmake
    const int firstSpace = line.indexOf(QLatin1Char(' '), 11);
    return line.mid(firstSpace).trimmed();
}

void MakeFileParse::parseArgs(const QString &args, const QString &project,
                              QList<QMakeAssignment> *assignments,
                              QList<QMakeAssignment> *afterAssignments)
{
    const QRegularExpression regExp(QLatin1String("^([^\\s\\+-]*)\\s*(\\+=|=|-=|~=)(.*)$"));
    bool after = false;
    bool ignoreNext = false;
    m_unparsedArguments = args;
    QtcProcess::ArgIterator ait(&m_unparsedArguments);
    while (ait.next()) {
        if (ignoreNext) {
            // Ignoring
            ignoreNext = false;
            ait.deleteArg();
        } else if (ait.value() == project) {
            ait.deleteArg();
        } else if (ait.value() == QLatin1String("-after")) {
            after = true;
            ait.deleteArg();
        } else if (ait.value().contains(QLatin1Char('='))) {
            const QRegularExpressionMatch match = regExp.match(ait.value());
            if (match.hasMatch()) {
                QMakeAssignment qa;
                qa.variable = match.captured(1);
                qa.op = match.captured(2);
                qa.value = match.captured(3).trimmed();
                if (after)
                    afterAssignments->append(qa);
                else
                    assignments->append(qa);
            } else {
                qDebug()<<"regexp did not match";
            }
            ait.deleteArg();
        } else if (ait.value() == QLatin1String("-o")) {
            ignoreNext = true;
            ait.deleteArg();
#if defined(Q_OS_WIN32)
        } else if (ait.value() == QLatin1String("-win32")) {
#elif defined(Q_OS_MAC)
        } else if (ait.value() == QLatin1String("-macx")) {
#elif defined(Q_OS_QNX6)
        } else if (ait.value() == QLatin1String("-qnx6")) {
#else
        } else if (ait.value() == QLatin1String("-unix")) {
#endif
            ait.deleteArg();
        }
    }
}

void dumpQMakeAssignments(const QList<QMakeAssignment> &list)
{
    foreach (const QMakeAssignment &qa, list) {
        qCDebug(MakeFileParse::logging()) << "    " << qa.variable << qa.op << qa.value;
    }
}

QList<QMakeAssignment> MakeFileParse::parseAssignments(const QList<QMakeAssignment> &assignments)
{
    bool foundSeparateDebugInfo = false;
    bool foundForceDebugInfo = false;
    QList<QMakeAssignment> filteredAssignments;
    foreach (const QMakeAssignment &qa, assignments) {
        if (qa.variable == QLatin1String("CONFIG")) {
            QStringList values = qa.value.split(QLatin1Char(' '));
            QStringList newValues;
            foreach (const QString &value, values) {
                if (value == QLatin1String("debug")) {
                    if (qa.op == QLatin1String("+=")) {
                        m_qmakeBuildConfig.explicitDebug = true;
                        m_qmakeBuildConfig.explicitRelease = false;
                    } else {
                        m_qmakeBuildConfig.explicitDebug = false;
                        m_qmakeBuildConfig.explicitRelease = true;
                    }
                } else if (value == QLatin1String("release")) {
                    if (qa.op == QLatin1String("+=")) {
                        m_qmakeBuildConfig.explicitDebug = false;
                        m_qmakeBuildConfig.explicitRelease = true;
                    } else {
                        m_qmakeBuildConfig.explicitDebug = true;
                        m_qmakeBuildConfig.explicitRelease = false;
                    }
                } else if (value == QLatin1String("debug_and_release")) {
                    if (qa.op == QLatin1String("+=")) {
                        m_qmakeBuildConfig.explicitBuildAll = true;
                        m_qmakeBuildConfig.explicitNoBuildAll = false;
                    } else {
                        m_qmakeBuildConfig.explicitBuildAll = false;
                        m_qmakeBuildConfig.explicitNoBuildAll = true;
                    }
                } else if (value == QLatin1String("x86")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.archConfig = QMakeStepConfig::X86;
                    else
                        m_config.archConfig = QMakeStepConfig::NoArch;
                } else if (value == QLatin1String("x86_64")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.archConfig = QMakeStepConfig::X86_64;
                    else
                        m_config.archConfig = QMakeStepConfig::NoArch;
                } else if (value == QLatin1String("ppc")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.archConfig = QMakeStepConfig::PowerPC;
                    else
                        m_config.archConfig = QMakeStepConfig::NoArch;
                } else if (value == QLatin1String("ppc64")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.archConfig = QMakeStepConfig::PowerPC64;
                    else
                        m_config.archConfig = QMakeStepConfig::NoArch;
                } else if (value == QLatin1String("iphonesimulator")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.osType = QMakeStepConfig::IphoneSimulator;
                    else
                        m_config.osType = QMakeStepConfig::NoOsType;
                } else if (value == QLatin1String("iphoneos")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.osType = QMakeStepConfig::IphoneOS;
                    else
                        m_config.osType = QMakeStepConfig::NoOsType;
                } else if (value == QLatin1String("qml_debug")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.linkQmlDebuggingQQ2 = TriState::Enabled;
                    else
                        m_config.linkQmlDebuggingQQ2 = TriState::Disabled;
                } else if (value == QLatin1String("qtquickcompiler")) {
                    if (qa.op == QLatin1String("+="))
                        m_config.useQtQuickCompiler = TriState::Enabled;
                    else
                        m_config.useQtQuickCompiler = TriState::Disabled;
                } else if (value == QLatin1String("force_debug_info")) {
                    if (qa.op == QLatin1String("+="))
                        foundForceDebugInfo = true;
                    else
                        foundForceDebugInfo = false;
                } else if (value == QLatin1String("separate_debug_info")) {
                    if (qa.op == QLatin1String("+=")) {
                        foundSeparateDebugInfo = true;
                        m_config.separateDebugInfo = TriState::Enabled;
                    } else {
                        foundSeparateDebugInfo = false;
                        m_config.separateDebugInfo = TriState::Disabled;
                    }
                } else {
                    newValues.append(value);
                }
            }
            if (!newValues.isEmpty()) {
                QMakeAssignment newQA = qa;
                newQA.value = newValues.join(QLatin1Char(' '));
                filteredAssignments.append(newQA);
            }
        } else {
            filteredAssignments.append(qa);
        }
    }

    if (foundForceDebugInfo && foundSeparateDebugInfo) {
        m_config.separateDebugInfo = TriState::Enabled;
    } else if (foundForceDebugInfo) {
        // Found only force_debug_info, so readd it
        QMakeAssignment newQA;
        newQA.variable = QLatin1String("CONFIG");
        newQA.op = QLatin1String("+=");
        newQA.value = QLatin1String("force_debug_info");
        filteredAssignments.append(newQA);
    } else if (foundSeparateDebugInfo) {
        // Found only separate_debug_info, so readd it
        QMakeAssignment newQA;
        newQA.variable = QLatin1String("CONFIG");
        newQA.op = QLatin1String("+=");
        newQA.value = QLatin1String("separate_debug_info");
        filteredAssignments.append(newQA);
    }
    return filteredAssignments;
}

static FilePath findQMakeBinaryFromMakefile(const QString &makefile)
{
    QFile fi(makefile);
    if (fi.exists() && fi.open(QFile::ReadOnly)) {
        QTextStream ts(&fi);
        const QRegularExpression r1(QLatin1String("^QMAKE\\s*=(.*)$"));
        while (!ts.atEnd()) {
            QString line = ts.readLine();
            const QRegularExpressionMatch match = r1.match(line);
            if (match.hasMatch()) {
                QFileInfo qmake(match.captured(1).trimmed());
                QString qmakePath = qmake.filePath();
                if (!QString::fromLatin1(QTC_HOST_EXE_SUFFIX).isEmpty()
                        && !qmakePath.endsWith(QLatin1String(QTC_HOST_EXE_SUFFIX))) {
                    qmakePath.append(QLatin1String(QTC_HOST_EXE_SUFFIX));
                }
                // Is qmake still installed?
                QFileInfo fi(qmakePath);
                if (fi.exists())
                    return FilePath::fromFileInfo(fi);
            }
        }
    }
    return FilePath();
}

MakeFileParse::MakeFileParse(const QString &makefile, Mode mode) : m_mode(mode)
{
    qCDebug(logging()) << "Parsing makefile" << makefile;
    if (!QFileInfo::exists(makefile)) {
        qCDebug(logging()) << "**doesn't exist";
        m_state = MakefileMissing;
        return;
    }

    // Qt Version!
    m_qmakePath = findQMakeBinaryFromMakefile(makefile);
    qCDebug(logging()) << "  qmake:" << m_qmakePath;

    QString project = findQMakeLine(makefile, QLatin1String("# Project:")).trimmed();
    if (project.isEmpty()) {
        m_state = CouldNotParse;
        qCDebug(logging()) << "**No Project line";
        return;
    }

    project.remove(0, project.indexOf(QLatin1Char(':')) + 1);
    project = project.trimmed();

    // Src Pro file
    m_srcProFile = QDir::cleanPath(QFileInfo(makefile).absoluteDir().filePath(project));
    qCDebug(logging()) << "  source .pro file:" << m_srcProFile;

    QString command = findQMakeLine(makefile, QLatin1String("# Command:")).trimmed();
    if (command.isEmpty()) {
        m_state = CouldNotParse;
        qCDebug(logging()) << "**No Command line found";
        return;
    }

    command = trimLine(command);
    parseCommandLine(command, project);

    m_state = Okay;
}

MakeFileParse::MakefileState MakeFileParse::makeFileState() const
{
    return m_state;
}

Utils::FilePath MakeFileParse::qmakePath() const
{
    return m_qmakePath;
}

QString MakeFileParse::srcProFile() const
{
    return m_srcProFile;
}

QMakeStepConfig MakeFileParse::config() const
{
    return m_config;
}


QString MakeFileParse::unparsedArguments() const
{
    return m_unparsedArguments;
}

BaseQtVersion::QmakeBuildConfigs MakeFileParse::effectiveBuildConfig(BaseQtVersion::QmakeBuildConfigs defaultBuildConfig) const
{
    BaseQtVersion::QmakeBuildConfigs buildConfig = defaultBuildConfig;
    if (m_qmakeBuildConfig.explicitDebug)
        buildConfig = buildConfig | BaseQtVersion::DebugBuild;
    else if (m_qmakeBuildConfig.explicitRelease)
        buildConfig = buildConfig & ~BaseQtVersion::DebugBuild;
    if (m_qmakeBuildConfig.explicitBuildAll)
        buildConfig = buildConfig | BaseQtVersion::BuildAll;
    else if (m_qmakeBuildConfig.explicitNoBuildAll)
        buildConfig = buildConfig &~ BaseQtVersion::BuildAll;
    return buildConfig;
}

const QLoggingCategory &MakeFileParse::logging()
{
    static const QLoggingCategory category("qtc.qmakeprojectmanager.import", QtWarningMsg);
    return category;
}

void MakeFileParse::parseCommandLine(const QString &command, const QString &project)
{

    QList<QMakeAssignment> assignments;
    QList<QMakeAssignment> afterAssignments;
    // Split up args into assignments and other arguments, writes m_unparsedArguments
    parseArgs(command, project, &assignments, &afterAssignments);
    qCDebug(logging()) << "  Initial assignments:";
    dumpQMakeAssignments(assignments);

    // Filter out CONFIG arguments we know into m_qmakeBuildConfig and m_config
    const QList<QMakeAssignment> filteredAssignments = parseAssignments(assignments);
    qCDebug(logging()) << "  After parsing";
    dumpQMakeAssignments(filteredAssignments);

    qCDebug(logging()) << "  Explicit Debug" << m_qmakeBuildConfig.explicitDebug;
    qCDebug(logging()) << "  Explicit Release" << m_qmakeBuildConfig.explicitRelease;
    qCDebug(logging()) << "  Explicit BuildAll" << m_qmakeBuildConfig.explicitBuildAll;
    qCDebug(logging()) << "  Explicit NoBuildAll" << m_qmakeBuildConfig.explicitNoBuildAll;
    qCDebug(logging()) << "  TargetArch" << m_config.archConfig;
    qCDebug(logging()) << "  OsType" << m_config.osType;
    qCDebug(logging()) << "  LinkQmlDebuggingQQ2"
                       << (m_config.linkQmlDebuggingQQ2 == TriState::Enabled);
    qCDebug(logging()) << "  Qt Quick Compiler"
                       <<  (m_config.useQtQuickCompiler == TriState::Enabled);
    qCDebug(logging()) << "  Separate Debug Info"
                       << (m_config.separateDebugInfo == TriState::Enabled);

    // Create command line of all unfiltered arguments
    const QList<QMakeAssignment> &assignmentsToUse = m_mode == Mode::FilterKnownConfigValues
            ? filteredAssignments : assignments;
    foreach (const QMakeAssignment &qa, assignmentsToUse)
        QtcProcess::addArg(&m_unparsedArguments, qa.variable + qa.op + qa.value);
    if (!afterAssignments.isEmpty()) {
        QtcProcess::addArg(&m_unparsedArguments, QLatin1String("-after"));
        foreach (const QMakeAssignment &qa, afterAssignments)
            QtcProcess::addArg(&m_unparsedArguments, qa.variable + qa.op + qa.value);
    }
}

} // Internal
} // QmakeProjectManager

// Unit tests:

#ifdef WITH_TESTS
#   include <QTest>

#   include "qmakeprojectmanagerplugin.h"

#   include "projectexplorer/outputparser_test.h"

using namespace QmakeProjectManager::Internal;
using namespace ProjectExplorer;

void QmakeProjectManagerPlugin::testMakefileParser_data()
{
    QTest::addColumn<QString>("command");
    QTest::addColumn<QString>("project");
    QTest::addColumn<QString>("unparsedArguments");
    QTest::addColumn<int>("archConfig");
    QTest::addColumn<int>("osType");
    QTest::addColumn<bool>("linkQmlDebuggingQQ2");
    QTest::addColumn<bool>("useQtQuickCompiler");
    QTest::addColumn<bool>("separateDebugInfo");
    QTest::addColumn<int>("effectiveBuildConfig");

    QTest::newRow("Qt 5.7")
            << QString::fromLatin1("-spec linux-g++ CONFIG+=debug CONFIG+=qml_debug -o Makefile ../untitled7/untitled7.pro")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.7 extra1")
            << QString::fromLatin1("SOMETHING=ELSE -spec linux-g++ CONFIG+=debug CONFIG+=qml_debug -o Makefile ../untitled7/untitled7.pro")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.7 extra2")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE CONFIG+=debug CONFIG+=qml_debug -o Makefile ../untitled7/untitled7.pro")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.7 extra3")
            << QString::fromLatin1("-spec linux-g++ CONFIG+=debug SOMETHING=ELSE CONFIG+=qml_debug -o Makefile ../untitled7/untitled7.pro")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.7 extra4")
            << QString::fromLatin1("-spec linux-g++ CONFIG+=debug CONFIG+=qml_debug SOMETHING=ELSE -o Makefile ../untitled7/untitled7.pro")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.7 extra5")
            << QString::fromLatin1("-spec linux-g++ CONFIG+=debug CONFIG+=qml_debug -o Makefile SOMETHING=ELSE ../untitled7/untitled7.pro")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.7 extra6")
            << QString::fromLatin1("-spec linux-g++ CONFIG+=debug CONFIG+=qml_debug -o Makefile ../untitled7/untitled7.pro SOMETHING=ELSE")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.8")
            << QString::fromLatin1("-o Makefile ../untitled7/untitled7.pro -spec linux-g++ CONFIG+=debug CONFIG+=qml_debug")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.8 extra1")
            << QString::fromLatin1("SOMETHING=ELSE -o Makefile ../untitled7/untitled7.pro -spec linux-g++ CONFIG+=debug CONFIG+=qml_debug")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.8 extra2")
            << QString::fromLatin1("-o Makefile SOMETHING=ELSE ../untitled7/untitled7.pro -spec linux-g++ CONFIG+=debug CONFIG+=qml_debug")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.8 extra3")
            << QString::fromLatin1("-o Makefile ../untitled7/untitled7.pro SOMETHING=ELSE -spec linux-g++ CONFIG+=debug CONFIG+=qml_debug")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.8 extra4")
            << QString::fromLatin1("-o Makefile ../untitled7/untitled7.pro -spec linux-g++ SOMETHING=ELSE CONFIG+=debug CONFIG+=qml_debug")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.8 extra5")
            << QString::fromLatin1("-o Makefile ../untitled7/untitled7.pro -spec linux-g++ CONFIG+=debug SOMETHING=ELSE CONFIG+=qml_debug")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
    QTest::newRow("Qt 5.8 extra6")
            << QString::fromLatin1("-o Makefile ../untitled7/untitled7.pro -spec linux-g++ CONFIG+=debug CONFIG+=qml_debug SOMETHING=ELSE")
            << QString::fromLatin1("../untitled7/untitled7.pro")
            << QString::fromLatin1("-spec linux-g++ SOMETHING=ELSE")
            << static_cast<int>(QMakeStepConfig::NoArch) << static_cast<int>(QMakeStepConfig::NoOsType)
            << true << false << false << 2;
}

void QmakeProjectManagerPlugin::testMakefileParser()
{
    QFETCH(QString, command);
    QFETCH(QString, project);
    QFETCH(QString, unparsedArguments);
    QFETCH(int, archConfig);
    QFETCH(int, osType);
    QFETCH(bool, linkQmlDebuggingQQ2);
    QFETCH(bool, useQtQuickCompiler);
    QFETCH(bool, separateDebugInfo);
    QFETCH(int, effectiveBuildConfig);

    MakeFileParse parser("/tmp/something", MakeFileParse::Mode::FilterKnownConfigValues);
    parser.parseCommandLine(command, project);

    QCOMPARE(Utils::QtcProcess::splitArgs(parser.unparsedArguments()),
             Utils::QtcProcess::splitArgs(unparsedArguments));
    QCOMPARE(parser.effectiveBuildConfig({}), effectiveBuildConfig);

    const QMakeStepConfig qmsc = parser.config();
    QCOMPARE(qmsc.archConfig, static_cast<QMakeStepConfig::TargetArchConfig>(archConfig));
    QCOMPARE(qmsc.osType, static_cast<QMakeStepConfig::OsType>(osType));
    QCOMPARE(qmsc.linkQmlDebuggingQQ2 == TriState::Enabled, linkQmlDebuggingQQ2);
    QCOMPARE(qmsc.useQtQuickCompiler == TriState::Enabled, useQtQuickCompiler);
    QCOMPARE(qmsc.separateDebugInfo == TriState::Enabled, separateDebugInfo);
}
#endif
