/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "addmersdkoperation.h"
#include "addkeysoperation.h"
#include "findvalueoperation.h"
#include "rmkeysoperation.h"
#include "getoperation.h"
#include "../../plugins/mer/merconstants.h"
#include <iostream>

const char MER_PARAM_VM_NAME[] = "--vm-name";
const char MER_PARAM_AUTODETECTED[] = "--autodetected";
const char MER_PARAM_SHARED_HOME[] = "--shared-home";
const char MER_PARAM_SHARED_TARGETS[] = "--shared-targets";
const char MER_PARAM_SHARED_SSH[] = "--shared-ssh";
const char MER_PARAM_SHARED_SRC[] = "--shared-src";
const char MER_PARAM_SHARED_CONFIG[] = "--shared-config";
const char MER_PARAM_HOST[] = "--host";
const char MER_PARAM_USERNAME[] = "--username";
const char MER_PARAM_PRIVATE_KEY_FILE[] = "--private-key-file";
const char MER_PARAM_SSH_PORT[] = "--ssh-port";
const char MER_PARAM_WWW_PORT[] = "--www-port";
const char MER_PARAM_HEADLESS[] = "--headless";
const char MER_PARAM_INSTALLDIR[] = "--installdir";
const char MER_PARAM_VERSION[] = "--version";
const char MER_PARAM_MEMORY_SIZE[] = "--memory";
const char MER_PARAM_CPU_COUNT[] = "--cpu-count";
const char MER_PARAM_VDI_CAPACITY[] = "--vdi-capacity";

AddMerSdkOperation::AddMerSdkOperation()
    : m_version(0)
    , m_autoDetected(true)
    , m_sshPort(0)
    , m_wwwPort(0)
    , m_headless(false)
    , m_memorySizeMb(0)
    , m_cpuCount(0)
    , m_vdiCapacityMb(0)
{
}

QString AddMerSdkOperation::name() const
{
    return QLatin1String("addMerSdk");
}

QString AddMerSdkOperation::helpText() const
{
    return QLatin1String("add mer sdk to Qt Creator configuration");
}

QString AddMerSdkOperation::argumentsHelpText() const
{
    const QString indent = QLatin1String("    ");
    return indent + QLatin1String(MER_PARAM_INSTALLDIR) + QLatin1String(" <DIR>            mer sdk install directory (required).\n")
         + indent + QLatin1String(MER_PARAM_VM_NAME) + QLatin1String(" <NAME>              mer sdk virtual machine name (required).\n")
         + indent + QLatin1String(MER_PARAM_VERSION) + QLatin1String(" <NUMBER>            configuration version (required).\n")
         + indent + QLatin1String(MER_PARAM_AUTODETECTED) + QLatin1String(" <BOOL>         is sdk autodetected.\n")
         + indent + QLatin1String(MER_PARAM_SHARED_HOME) + QLatin1String(" <PATH>          shared \"home\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_SHARED_TARGETS) + QLatin1String(" <PATH>       shared \"targets\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_SHARED_SSH) + QLatin1String(" <PATH>           shared \"ssh\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_SHARED_SRC) + QLatin1String(" <PATH>           shared \"src\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_SHARED_CONFIG) + QLatin1String(" <PATH>        shared \"config\" folder (required).\n")
         + indent + QLatin1String(MER_PARAM_HOST) + QLatin1String(" <NAME>                 mersdk ssh hostname (required).\n")
         + indent + QLatin1String(MER_PARAM_USERNAME) + QLatin1String(" <NAME>             mersdk ssh username (required).\n")
         + indent + QLatin1String(MER_PARAM_PRIVATE_KEY_FILE) + QLatin1String(" <FILE>     mersdk private key file (required).\n")
         + indent + QLatin1String(MER_PARAM_SSH_PORT) + QLatin1String(" <NUMBER>           mersdk ssh port (required).\n")
         + indent + QLatin1String(MER_PARAM_WWW_PORT) + QLatin1String(" <NUMBER>           mersdk www port (required).\n")
         + indent + QLatin1String(MER_PARAM_MEMORY_SIZE) + QLatin1String(" <MB>            mersdk virtual machine memory size (required).\n")
         + indent + QLatin1String(MER_PARAM_CPU_COUNT) + QLatin1String(" <NUMBER>          mersdk virtual machine cpu count (required).\n")
         + indent + QLatin1String(MER_PARAM_VDI_CAPACITY) + QLatin1String(" <MB>           mersdk virtual machine vdi size (required).\n")
         + indent + QLatin1String(MER_PARAM_HEADLESS) + QLatin1String("                    set headless mode.\n");
}

bool AddMerSdkOperation::setArguments(const QStringList &args)
{

    for (int i = 0; i < args.count(); ++i) {
        const QString current = args.at(i);
        const QString next = ((i + 1) < args.count()) ? args.at(i + 1) : QString();

        if (current == QLatin1String(MER_PARAM_INSTALLDIR)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_installDir = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_VM_NAME)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_name = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_VERSION)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_version = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_AUTODETECTED)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_autoDetected = next == QLatin1String("true");
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SHARED_HOME)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedHomePath = next;
            continue;
        }


        if (current == QLatin1String(MER_PARAM_SHARED_TARGETS)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedTargetsPath = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SHARED_SSH)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedSshPath = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SHARED_SRC)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedSrcPath = next;
            continue;
        }


        if (current == QLatin1String(MER_PARAM_SHARED_CONFIG)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sharedConfigPath = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_HOST)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_host = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_USERNAME)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_userName = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_PRIVATE_KEY_FILE)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_privateKeyFile = next;
            continue;
        }

        if (current == QLatin1String(MER_PARAM_SSH_PORT)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_sshPort = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_WWW_PORT)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_wwwPort = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_MEMORY_SIZE)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_memorySizeMb = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_CPU_COUNT)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_cpuCount = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_VDI_CAPACITY)) {
            if (next.isNull())
                return false;
            ++i; // skip next;
            m_vdiCapacityMb = next.toInt();
            continue;
        }

        if (current == QLatin1String(MER_PARAM_HEADLESS)) {
            m_headless = true;
            continue;
        }
    }

    const char MISSING[] = " parameter missing.";
    bool error = false;
    if (m_installDir.isEmpty()) {
        std::cerr << MER_PARAM_INSTALLDIR << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_name.isEmpty()) {
        std::cerr << MER_PARAM_VM_NAME << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_version == 0) {
        std::cerr << MER_PARAM_VERSION << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedHomePath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_HOME << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedTargetsPath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_TARGETS << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedSshPath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_SSH << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedSrcPath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_SRC << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sharedConfigPath.isEmpty()) {
        std::cerr << MER_PARAM_SHARED_CONFIG << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_host.isEmpty()) {
        std::cerr << MER_PARAM_HOST << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_userName.isEmpty()) {
        std::cerr << MER_PARAM_USERNAME << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_privateKeyFile.isEmpty()) {
        std::cerr << MER_PARAM_PRIVATE_KEY_FILE << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_sshPort == 0) {
        std::cerr << MER_PARAM_SSH_PORT << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_wwwPort == 0) {
        std::cerr << MER_PARAM_WWW_PORT << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_memorySizeMb == 0) {
        std::cerr << MER_PARAM_MEMORY_SIZE << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_cpuCount == 0) {
        std::cerr << MER_PARAM_CPU_COUNT << MISSING << std::endl << std::endl;
        error = true;
    }
    if (m_vdiCapacityMb == 0) {
        std::cerr << MER_PARAM_VDI_CAPACITY << MISSING << std::endl << std::endl;
        error = true;
    }

    return !error;
}

int AddMerSdkOperation::execute() const
{
    QVariantMap map = load(QLatin1String("mersdk"));
    if (map.isEmpty())
        map = initializeSdks(m_version);

    map.insert(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY), m_version);
    map.insert(QLatin1String(Mer::Constants::MER_SDK_INSTALLDIR), m_installDir);
    const QVariantMap result = addSdk(map, m_name, m_autoDetected, m_sharedHomePath, m_sharedTargetsPath, m_sharedSshPath,
                                      m_sharedSrcPath, m_sharedConfigPath, m_host, m_userName, m_privateKeyFile, m_sshPort,
                                      m_wwwPort, m_headless, m_memorySizeMb, m_cpuCount, m_vdiCapacityMb);

    if (result.isEmpty() || map == result)
        return 2;

    return save(result,QLatin1String("mersdk")) ? 0 : 3;
}

QVariantMap AddMerSdkOperation::initializeSdks(int version)
{
    QVariantMap map;
    map.insert(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY), version);
    map.insert(QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY), 0);
    return map;
}

QVariantMap AddMerSdkOperation::addSdk(const QVariantMap &map,
                                          const QString &sdkName,
                                          bool autodetected,
                                          const QString &sharedHomePath,
                                          const QString &sharedTargetsPath,
                                          const QString &sharedSshPath,
                                          const QString &sharedSrcPath,
                                          const QString &sharedConfigPath,
                                          const QString &host,
                                          const QString &userName,
                                          const QString &privateKeyFile,
                                          quint16 sshPort,
                                          quint16 wwwPort,
                                          bool headless,
                                          int memorySizeMb,
                                          int cpuCount,
                                          int vdiCapacityMb)
{
    QStringList valueKeys = FindValueOperation::findValue(map, QVariant(sdkName));
    bool hasTarget = false;
    foreach (const QString &t, valueKeys) {
        if (t.endsWith(QLatin1Char('/') + QLatin1String(Mer::Constants::VM_NAME))) {
            hasTarget = true;
            break;
        }
    }
    if (hasTarget) {
        std::cerr << "Error: Name " << qPrintable(sdkName) << " already defined as sdk." << std::endl;
        return QVariantMap();
    }

    bool ok;
    const int count = GetOperation::get(map, QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY)).toInt(&ok);
    if (!ok || count < 0) {
        std::cerr << "Error: Count found sdks, file seems wrong." << std::endl;
        return QVariantMap();
    }

    const QString sdk = QString::fromLatin1(Mer::Constants::MER_SDK_DATA_KEY) + QString::number(count);

    // remove old count

    QVariantMap cleaned = RmKeysOperation::rmKeys(map,  QStringList() << QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY));

    KeyValuePairList data;
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::VM_NAME), QVariant(sdkName));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::AUTO_DETECTED), QVariant(autodetected));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::SHARED_HOME), QVariant(sharedHomePath));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::SHARED_TARGET), QVariant(sharedTargetsPath));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::SHARED_SSH), QVariant(sharedSshPath));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::SHARED_SRC), QVariant(sharedSrcPath));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::SHARED_CONFIG), QVariant(sharedConfigPath));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::HOST), QVariant(host));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::USERNAME), QVariant(userName));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::PRIVATE_KEY_FILE), QVariant(privateKeyFile));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::SSH_PORT), QVariant(sshPort));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::WWW_PORT), QVariant(wwwPort));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::HEADLESS), QVariant(headless));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::MEMORY_SIZE_MB), QVariant(memorySizeMb));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::CPU_COUNT), QVariant(cpuCount));
    data << KeyValuePair(QStringList() << sdk << QLatin1String(Mer::Constants::VDI_CAPACITY_MB), QVariant(vdiCapacityMb));
    data << KeyValuePair(QStringList() << QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY), QVariant(count + 1));

    return AddKeysOperation::addKeys(cleaned, data);
}

#ifdef WITH_TESTS
bool AddMerSdkOperation::test() const
{
    QVariantMap map = initializeSdks(2);

    if (map.count() != 2
            || !map.contains(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY)).toInt() != 2
            || !map.contains(QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY)).toInt() != 0)
        return false;

    map = addSdk(map, QLatin1String("testSdk"), true,
                 QLatin1String("/test/sharedHomePath"),
                 QLatin1String("/test/sharedTargetPath"),
                 QLatin1String("/test/sharedSshPath"),
                 QLatin1String("/test/sharedSrcPath"),
                 QLatin1String("/test/sharedConfigPath"),
                 QLatin1String("host"),
                 QLatin1String("user"),
                 QLatin1String("/test/privateKey"),22,80,false,512,2,10000);

    const QString sdk = QString::fromLatin1(Mer::Constants::MER_SDK_DATA_KEY) + QString::number(0);


    if (map.count() != 3
            || !map.contains(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_SDK_FILE_VERSION_KEY)).toInt() != 2
            || !map.contains(QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY))
            || map.value(QLatin1String(Mer::Constants::MER_SDK_COUNT_KEY)).toInt() != 1
            || !map.contains(sdk))
        return false;

    QVariantMap sdkMap= map.value(sdk).toMap();
    if (sdkMap.count() != 16
            || !sdkMap.contains(QLatin1String(Mer::Constants::VM_NAME))
            || sdkMap.value(QLatin1String(Mer::Constants::VM_NAME)).toString() != QLatin1String("testSdk")
            || !sdkMap.contains(QLatin1String(Mer::Constants::AUTO_DETECTED))
            || sdkMap.value(QLatin1String(Mer::Constants::AUTO_DETECTED)).toBool() != true
            || !sdkMap.contains(QLatin1String(Mer::Constants::SHARED_HOME))
            || sdkMap.value(QLatin1String(Mer::Constants::SHARED_HOME)).toString() != QLatin1String("/test/sharedHomePath")
            || !sdkMap.contains(QLatin1String(Mer::Constants::SHARED_TARGET))
            || sdkMap.value(QLatin1String(Mer::Constants::SHARED_TARGET)).toString() != QLatin1String("/test/sharedTargetPath")
            || !sdkMap.contains(QLatin1String(Mer::Constants::SHARED_SSH))
            || sdkMap.value(QLatin1String(Mer::Constants::SHARED_SSH)).toString() != QLatin1String("/test/sharedSshPath")
            || !sdkMap.contains(QLatin1String(Mer::Constants::SHARED_SRC))
            || sdkMap.value(QLatin1String(Mer::Constants::SHARED_SRC)).toString() != QLatin1String("/test/sharedSrcPath")
            || !sdkMap.contains(QLatin1String(Mer::Constants::SHARED_CONFIG))
            || sdkMap.value(QLatin1String(Mer::Constants::SHARED_CONFIG)).toString() != QLatin1String("/test/sharedConfigPath")
            || !sdkMap.contains(QLatin1String(Mer::Constants::HOST))
            || sdkMap.value(QLatin1String(Mer::Constants::HOST)).toString() != QLatin1String("host")
            || !sdkMap.contains(QLatin1String(Mer::Constants::USERNAME))
            || sdkMap.value(QLatin1String(Mer::Constants::USERNAME)).toString() != QLatin1String("user")
            || !sdkMap.contains(QLatin1String(Mer::Constants::PRIVATE_KEY_FILE))
            || sdkMap.value(QLatin1String(Mer::Constants::PRIVATE_KEY_FILE)).toString() != QLatin1String("/test/privateKey")
            || !sdkMap.contains(QLatin1String(Mer::Constants::SSH_PORT))
            || sdkMap.value(QLatin1String(Mer::Constants::SSH_PORT)).toInt() != 22
            || !sdkMap.contains(QLatin1String(Mer::Constants::WWW_PORT))
            || sdkMap.value(QLatin1String(Mer::Constants::WWW_PORT)).toInt() != 80
            || !sdkMap.contains(QLatin1String(Mer::Constants::HEADLESS))
            || sdkMap.value(QLatin1String(Mer::Constants::HEADLESS)).toBool() != false
            || !sdkMap.contains(QLatin1String(Mer::Constants::MEMORY_SIZE_MB))
            || sdkMap.value(QLatin1String(Mer::Constants::MEMORY_SIZE_MB)).toInt() != 512
            || !sdkMap.contains(QLatin1String(Mer::Constants::CPU_COUNT))
            || sdkMap.value(QLatin1String(Mer::Constants::CPU_COUNT)).toInt() != 2
            || !sdkMap.contains(QLatin1String(Mer::Constants::VDI_CAPACITY_MB))
            || sdkMap.value(QLatin1String(Mer::Constants::VDI_CAPACITY_MB)).toInt() != 10000)
        return false;

    return true;
}
#endif
