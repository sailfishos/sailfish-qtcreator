/****************************************************************************
**
** Copyright (C) 2016 BogDan Vatra <bog_dan_ro@yahoo.com>
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

#include "androidqtversion.h"
#include "androidconstants.h"
#include "androidconfigurations.h"
#include "androidmanager.h"

#include <utils/environment.h>
#include <utils/hostosinfo.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtsupportconstants.h>
#include <qtsupport/qtversionmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projecttree.h>

#include <proparser/profileevaluator.h>

#include <QRegularExpression>

using namespace ProjectExplorer;

namespace Android {
namespace Internal {

AndroidQtVersion::AndroidQtVersion()
    : QtSupport::BaseQtVersion()
    , m_guard(std::make_unique<QObject>())
{
    QObject::connect(AndroidConfigurations::instance(),
                     &AndroidConfigurations::aboutToUpdate,
                     m_guard.get(),
                     [this] { resetCache(); });
}

bool AndroidQtVersion::isValid() const
{
    if (!BaseQtVersion::isValid())
        return false;
    if (qtAbis().isEmpty())
        return false;
    return true;
}

QString AndroidQtVersion::invalidReason() const
{
    QString tmp = BaseQtVersion::invalidReason();
    if (tmp.isEmpty()) {
        if (AndroidConfigurations::currentConfig().ndkLocation(this).isEmpty())
            return tr("NDK is not configured in Devices > Android.");
        if (AndroidConfigurations::currentConfig().sdkLocation().isEmpty())
            return tr("SDK is not configured in Devices > Android.");
        if (qtAbis().isEmpty())
            return tr("Failed to detect the ABIs used by the Qt version. Check the settings in "
                      "Devices > Android for errors.");
    }
    return tmp;
}

bool AndroidQtVersion::supportsMultipleQtAbis() const
{
    return qtVersion() >= QtSupport::QtVersionNumber{5, 14}
           && qtVersion() < QtSupport::QtVersionNumber{6, 0};
}

Abis AndroidQtVersion::detectQtAbis() const
{
    auto androidAbi2Abi = [](const QString &androidAbi) {
        if (androidAbi == ProjectExplorer::Constants::ANDROID_ABI_ARM64_V8A) {
            return Abi{Abi::Architecture::ArmArchitecture,
                       Abi::OS::LinuxOS,
                       Abi::OSFlavor::AndroidLinuxFlavor,
                       Abi::BinaryFormat::ElfFormat,
                       64, androidAbi};
        } else if (androidAbi == ProjectExplorer::Constants::ANDROID_ABI_ARMEABI_V7A) {
            return Abi{Abi::Architecture::ArmArchitecture,
                       Abi::OS::LinuxOS,
                       Abi::OSFlavor::AndroidLinuxFlavor,
                       Abi::BinaryFormat::ElfFormat,
                       32, androidAbi};
        } else if (androidAbi == ProjectExplorer::Constants::ANDROID_ABI_X86_64) {
            return Abi{Abi::Architecture::X86Architecture,
                       Abi::OS::LinuxOS,
                       Abi::OSFlavor::AndroidLinuxFlavor,
                       Abi::BinaryFormat::ElfFormat,
                       64, androidAbi};
        } else if (androidAbi == ProjectExplorer::Constants::ANDROID_ABI_X86) {
            return Abi{Abi::Architecture::X86Architecture,
                       Abi::OS::LinuxOS,
                       Abi::OSFlavor::AndroidLinuxFlavor,
                       Abi::BinaryFormat::ElfFormat,
                       32, androidAbi};
        } else {
            return Abi{Abi::Architecture::UnknownArchitecture,
                       Abi::OS::LinuxOS,
                       Abi::OSFlavor::AndroidLinuxFlavor,
                       Abi::BinaryFormat::ElfFormat,
                       0, androidAbi};
        }
    };
    Abis abis;
    for (const auto &abi : androidAbis())
        abis << androidAbi2Abi(abi);
    return abis;
}

void AndroidQtVersion::addToEnvironment(const Kit *k, Utils::Environment &env) const
{
    BaseQtVersion::addToEnvironment(k, env);

    const AndroidConfig &config = AndroidConfigurations::currentConfig();
    // this env vars are used by qmake mkspecs to generate makefiles (check QTDIR/mkspecs/android-g++/qmake.conf for more info)
    env.set(QLatin1String("ANDROID_NDK_HOST"), config.toolchainHost(this));
    env.set(QLatin1String("ANDROID_NDK_ROOT"), config.ndkLocation(this).toUserOutput());
    env.set(QLatin1String("ANDROID_NDK_PLATFORM"),
            config.bestNdkPlatformMatch(qMax(minimumNDK(), AndroidManager::minimumSDK(k)), this));
}

Utils::Environment AndroidQtVersion::qmakeRunEnvironment() const
{
    Utils::Environment env = Utils::Environment::systemEnvironment();
    env.set(QLatin1String("ANDROID_NDK_ROOT"),
            AndroidConfigurations::currentConfig().ndkLocation(this).toUserOutput());
    return env;
}

QString AndroidQtVersion::description() const
{
    //: Qt Version is meant for Android
    return tr("Android");
}

const QStringList &AndroidQtVersion::androidAbis() const
{
    ensureMkSpecParsed();
    return m_androidAbis;
}

int AndroidQtVersion::minimumNDK() const
{
    ensureMkSpecParsed();
    return m_minNdk;
}

Utils::FilePath AndroidQtVersion::androidDeploymentSettings(const Target *target)
{
    // Try to fetch the file name from node data as provided by qmake and Qbs
    const QString buildKey = target->activeBuildKey();
    const ProjectNode *node = target->project()->findNodeForBuildKey(buildKey);
    if (node) {
        const QString nameFromData = node->data(Constants::AndroidDeploySettingsFile).toString();
        if (!nameFromData.isEmpty())
            return Utils::FilePath::fromUserInput(nameFromData);
    }
    // If unavailable, construct the name by ourselves (CMake)
    const BaseQtVersion *qt = QtSupport::QtKitAspect::qtVersion(target->kit());
    const bool isQt6 = qt && qt->qtVersion() >= QtSupport::QtVersionNumber{6, 0, 0};
    return target->activeBuildConfiguration()->buildDirectory().pathAppended(
                isQt6 ? QString::fromLatin1("android-%1-deployment-settings.json").arg(buildKey)
                      : QLatin1String("android_deployment_settings.json"));
}

void AndroidQtVersion::parseMkSpec(ProFileEvaluator *evaluator) const
{
    m_androidAbis = evaluator->values("ALL_ANDROID_ABIS");
    if (m_androidAbis.isEmpty())
        m_androidAbis = QStringList{evaluator->value("ANDROID_TARGET_ARCH")};
    const QString androidPlatform = evaluator->value("ANDROID_PLATFORM");
    if (!androidPlatform.isEmpty()) {
        const QRegularExpression regex("android-(\\d+)");
        const QRegularExpressionMatch match = regex.match(androidPlatform);
        if (match.hasMatch()) {
            bool ok = false;
            int tmp = match.captured(1).toInt(&ok);
            if (ok)
                m_minNdk = tmp;
        }
    }
    BaseQtVersion::parseMkSpec(evaluator);
}

QSet<Utils::Id> AndroidQtVersion::availableFeatures() const
{
    QSet<Utils::Id> features = QtSupport::BaseQtVersion::availableFeatures();
    features.insert(QtSupport::Constants::FEATURE_MOBILE);
    features.remove(QtSupport::Constants::FEATURE_QT_CONSOLE);
    features.remove(QtSupport::Constants::FEATURE_QT_WEBKIT);
    return features;
}

QSet<Utils::Id> AndroidQtVersion::targetDeviceTypes() const
{
    return {Constants::ANDROID_DEVICE_TYPE};
}


// Factory

AndroidQtVersionFactory::AndroidQtVersionFactory()
{
    setQtVersionCreator([] { return new AndroidQtVersion; });
    setSupportedType(Constants::ANDROIDQT);
    setPriority(90);

    setRestrictionChecker([](const SetupData &setup) {
        return !setup.config.contains("android-no-sdk")
                && (setup.config.contains("android")
                    || setup.platforms.contains("android"));
    });
}

} // Internal
} // Android
