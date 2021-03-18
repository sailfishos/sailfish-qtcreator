/****************************************************************************
**
** Copyright (C) 2020 Open Mobile Platform LLC.
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

#include "merbuildconfigurationaspect.h"

#include "ui_merbuildconfigurationwidget.h"
#include "merconstants.h"
#include "mersdkkitaspect.h"
#include "mersettings.h"
#include "mersigninguserselectiondialog.h"

#include <sfdk/buildengine.h>
#include <sfdk/sdk.h>
#include <sfdk/sfdkconstants.h>
#include <sfdk/utils.h>

#include <coreplugin/messagemanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsystem.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/namedwidget.h>
#include <projectexplorer/project.h>
#include <utils/detailswidget.h>
#include <utils/algorithm.h>
#include <utils/utilsicons.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/variablechooser.h>

#include <QAction>
#include <QCompleter>
#include <QFormLayout>
#include <QInputDialog>
#include <QVBoxLayout>

using namespace Core;
using namespace ProjectExplorer;
using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

namespace {
const char SPEC_FILE_PATH[] = "MerSpecFileAspect.Path";
const char SFDK_CONFIGURATION_OPTIONS[] = "MerSfdkConfigurationAspect.Options";
const char MER_BUILD_CONFIGURATION_SIGN_PACKAGES[] = "MerBuildConfiguration.SignPackages";
const char MER_BUILD_CONFIGURATION_SIGNING_USER[] = "MerBuildConfiguration.SigningUser";
const char MER_BUILD_CONFIGURATION_SIGNING_PASSPHRASE_FILE[] = "MerBuildConfiguration.SigningPassphraseFile";
} // namespace anonymous

/*!
 * \class MerBuildConfigurationWidget
 * \internal
 */

class MerBuildConfigurationWidget : public ProjectExplorer::NamedWidget
{
    Q_OBJECT

public:
    explicit MerBuildConfigurationWidget(MerBuildConfigurationAspect *aspect)
        : NamedWidget(aspect->displayName())
        , m_ui(new Ui::MerBuildConfigurationWidget)
        , m_aspect(aspect)
    {
        auto vbox = new QVBoxLayout(this);
        vbox->setMargin(0);

        m_detailsContainer = new DetailsWidget(this);

        auto details = new QWidget;
        m_ui->setupUi(details);

        VariableChooser::addSupportForChildWidgets(details, m_aspect->buildConfiguration()->macroExpander());

        FilePath specDir = m_aspect->buildConfiguration()->project()->projectDirectory()
            .pathAppended("rpm");
        if (!specDir.exists())
            specDir = specDir.parentDir();

        m_ui->specFilePathChooser->setInitialBrowsePathBackup(specDir.toString());
        m_ui->specFilePathChooser->setExpectedKind(PathChooser::File);
        m_ui->specFilePathChooser->setPromptDialogFilter(tr("RPM spec File (*.spec)"));
        m_ui->specFilePathChooser->lineEdit()->setPlaceholderText(tr("Auto"));
        m_ui->specFilePathChooser->setValidationFunction([=](FancyLineEdit *edit, QString *errorMessage) {
            return edit->text().isEmpty()
                || m_ui->specFilePathChooser->defaultValidationFunction()(edit, errorMessage);
        });
        m_ui->specFilePathChooser->setPath(m_aspect->specFilePath());
        connect(m_ui->specFilePathChooser, &PathChooser::pathChanged, this, [=](const QString &path) {
            m_aspect->setSpecFilePath(path);
            updateSummary();
        });

        // Tooltip gets updated with validation results
        m_defaultOptionsToolTip = m_ui->sfdkOptionsLineEdit->toolTip()
            .arg(MerBuildConfigurationAspect::allowedSfdkOptions().join(", "));
        connect(m_ui->sfdkOptionsLineEdit, &FancyLineEdit::validChanged,
                this, &MerBuildConfigurationWidget::updateOptionsToolTip);

        m_ui->sfdkOptionsLineEdit->setHistoryCompleter("Mer.SfdkOptions.History");
        m_ui->sfdkOptionsLineEdit->setValidationFunction(MerBuildConfigurationWidget::validateOptions);
        m_ui->sfdkOptionsLineEdit->setText(m_aspect->sfdkOptionsString());
        connect(m_ui->sfdkOptionsLineEdit, &FancyLineEdit::editingFinished, this, [=]() {
            m_aspect->setSfdkOptionsString(m_ui->sfdkOptionsLineEdit->text());
            updateSummary();
        });

        updateOptionsToolTip(m_ui->sfdkOptionsLineEdit->isValid());

        m_detailsContainer->setWidget(details);

        vbox->addWidget(m_detailsContainer);

        updateSummary();

        auto setSignElementsEnabled = [](Ui::MerBuildConfigurationWidget *ui, bool enabled) {
            ui->signingUserChangeButton->setEnabled(enabled);
            ui->signingUserLineEdit->setEnabled(enabled);
            ui->signingPassphraseFileChooser->setEnabled(enabled);
            ui->signingUserClearButton->setEnabled(enabled);
        };

        m_ui->signPackagesCheckBox->setText(tr(Constants::MER_SIGN_PACKAGES_OPTION_NAME));
        m_ui->signingUserClearButton->setIcon(Utils::Icons::EDIT_CLEAR.icon());

        m_ui->signPackagesCheckBox->setChecked(m_aspect->signPackages());
        QString gpgErrorString;
        const bool isGpgAvailable = Sfdk::isGpgAvailable(&gpgErrorString);
        if (!isGpgAvailable) {
            m_ui->signErrorLabel->setText(gpgErrorString);
            m_ui->signErrorIconLabel->setPixmap(Icons::WARNING.pixmap());
            m_ui->signErrorIconLabel->setVisible(true);
            m_ui->signErrorLabel->setVisible(true);

            m_ui->signPackagesCheckBox->setEnabled(false);
            m_ui->signPackagesCheckBox->setChecked(false);
            setSignElementsEnabled(m_ui, false);
            return;
        }
        m_ui->signErrorIconLabel->setVisible(false);
        m_ui->signErrorLabel->setVisible(false);

        m_ui->signingUserLineEdit->setPlaceholderText(tr("Select the signing user"));
        m_ui->signingUserLineEdit->setText(m_aspect->signingUser().toString());
        m_ui->signingUserLineEdit->setToolTip(m_aspect->signingUser().toString());
        m_ui->signingUserLineEdit->setCursorPosition(0);
        connect(m_ui->signingUserLineEdit, &QLineEdit::selectionChanged,
                m_ui->signingUserLineEdit, [this]() {
            if (m_ui->signingUserLineEdit->selectedText().isEmpty())
                m_ui->signingUserLineEdit->setCursorPosition(0);
        });
        connect(m_ui->signingUserClearButton, &QPushButton::clicked, this, [this](){
            m_ui->signingUserLineEdit->clear();
            m_ui->signingUserLineEdit->setToolTip({});
            m_aspect->setSigningUser(GpgKeyInfo());
        });

        m_ui->signingPassphraseFileChooser->setExpectedKind(PathChooser::Kind::File);
        m_ui->signingPassphraseFileChooser->setPromptDialogTitle(
            tr("Select the passphrase file for the signing user"));
        const QString toolTip = tr("Passphrase file for the signing user");
        m_ui->signingPassphraseFileChooser->lineEdit()->setPlaceholderText(toolTip);
        m_ui->signingPassphraseFileChooser->setToolTip(toolTip);
        m_ui->signingPassphraseFileChooser->setPath(m_aspect->signingPassphraseFile());

        m_ui->signingPassphraseFileChooser->setValidationFunction([aspect = m_aspect](
                Utils::FancyLineEdit *edit, QString *errorMessage) {
            Kit *kit = aspect->buildConfiguration()->buildSystem()->kit();
            BuildEngine *engine = MerSdkKitAspect::buildEngine(kit);

            const FilePath passphraseFilePath = FilePath::fromString(edit->text());
            if (passphraseFilePath.isEmpty())
                return true;
            bool isSharedFile = passphraseFilePath.isChildOf(engine->sharedHomePath())
                    || passphraseFilePath.isChildOf(engine->sharedSrcPath());
            if (!isSharedFile) {
                *errorMessage = tr("Passphrase file must be stored under %1 workspace directory")
                        .arg(Sdk::sdkVariant());
                return false;
            }
            return true;
        });
        setSignElementsEnabled(m_ui, m_aspect->signPackages());

        connect(m_ui->signPackagesCheckBox, &QCheckBox::toggled,
                this, [this, setSignElementsEnabled](bool checked) {
            m_aspect->setSignPackages(checked);
            setSignElementsEnabled(m_ui, checked);
        });
        connect(m_ui->signingUserChangeButton, &QPushButton::clicked, this, [this]() {
            GpgKeyInfo selectedSigningUser = MerSigningUserSelectionDialog::selectSigningUser();
            if (!selectedSigningUser.isValid())
                return;

            m_aspect->setSigningUser(selectedSigningUser);
            m_ui->signingUserLineEdit->setText(selectedSigningUser.toString());
            m_ui->signingUserLineEdit->setToolTip(selectedSigningUser.toString());
            m_ui->signingUserLineEdit->setCursorPosition(0);
        });
        connect(m_ui->signingPassphraseFileChooser, &Utils::PathChooser::pathChanged,
                this, [aspect = m_aspect](const QString &passphraseFilePath) {
            aspect->setSigningPassphraseFile(passphraseFilePath);
        });
    }

    ~MerBuildConfigurationWidget() override
    {
        delete m_ui, m_ui = nullptr;
    }

private:
    void updateSummary()
    {
        bool customized = false;
        customized |= !m_ui->specFilePathChooser->path().isEmpty();
        customized |= !m_ui->sfdkOptionsLineEdit->text().isEmpty();

        const QString state = customized
            ? tr("Custom sfdk Options")
            : tr("Default sfdk Options");
        m_detailsContainer->setSummaryText(tr("Use <b>%1</b>").arg(state));
    }

    void updateOptionsToolTip(bool valid)
    {
        if (valid)
            m_ui->sfdkOptionsLineEdit->setToolTip(m_defaultOptionsToolTip);
    }

    static bool validateOptions(FancyLineEdit *edit, QString *errorMessage)
    {
        const bool abortOnMeta = true;
        QtcProcess::SplitError error;
        const QStringList options = QtcProcess::splitArgs(edit->text(), Utils::OsTypeLinux,
                abortOnMeta, &error);
        if (error != QtcProcess::SplitOk) {
            *errorMessage = tr("Could not split options");
            return false;
        }

        for (const QString &option : options) {
            if (option.startsWith('-')) {
                *errorMessage = tr("Configuration options may not start with \"-\"");
                return false;
            }
            if (option.startsWith('=')) {
                *errorMessage = tr("Configuration options may not start with \"=\"");
                return false;
            }

            const int splitPoint = option.indexOf('=');
            const QString name = option.left(splitPoint);

            if (!MerBuildConfigurationAspect::allowedSfdkOptions().contains(name)) {
                *errorMessage = tr("Option not valid or not allowed here: \"%1\"").arg(name);
                return false;
            }
        }

        return true;
    }

private:
    Ui::MerBuildConfigurationWidget *m_ui{};
    QPointer<MerBuildConfigurationAspect> m_aspect;
    QString m_defaultOptionsToolTip;
    QPointer<DetailsWidget> m_detailsContainer;
};

/*!
 * \class MerBuildConfigurationAspect
 * \internal
 */

const QStringList MerBuildConfigurationAspect::s_allowedSfdkOptions{
    "output-dir",
    "output-prefix",
    "search-output-dir",
    "no-search-output-dir",
    "snapshot",
    "force-no-snapshot",
    "task",
    "no-task",
    "fix-version",
    "no-fix-version",
    "no-pull-build-requires",
    "package.signing-user",
    "package.signing-passphrase",
    "package.signing-passphrase-file"
};

MerBuildConfigurationAspect::MerBuildConfigurationAspect(BuildConfiguration *buildConfiguration)
    : m_buildConfiguration(buildConfiguration)
{
    setId(Constants::MER_BUILD_CONFIGURATION_ASPECT);
    setDisplayName(displayName());
    setConfigWidgetCreator([this]() { return new MerBuildConfigurationWidget(this); });

    m_signPackages = MerSettings::signPackagesByDefault();
    m_signingUser = MerSettings::signingUser();
    m_signingPassphraseFile = MerSettings::signingPassphraseFile();
}

BuildConfiguration *MerBuildConfigurationAspect::buildConfiguration() const
{
    return m_buildConfiguration;
}

QString MerBuildConfigurationAspect::displayName()
{
    return tr("%1 Settings").arg(Sdk::sdkVariant());
}

void MerBuildConfigurationAspect::setSpecFilePath(const QString &specFilePath)
{
    if (specFilePath == m_specFilePath)
        return;

    m_specFilePath = specFilePath;

    emit changed();
}

void MerBuildConfigurationAspect::setSfdkOptionsString(const QString &sfdkOptionsString)
{
    if (sfdkOptionsString == m_sfdkOptionsString)
        return;

    m_sfdkOptionsString = sfdkOptionsString;

    emit changed();
}

void MerBuildConfigurationAspect::setSignPackages(bool enable)
{
    if (enable == m_signPackages)
        return;

    m_signPackages = enable;

    emit changed();
}

void MerBuildConfigurationAspect::setSigningUser(const Sfdk::GpgKeyInfo &signingUser)
{
    if (signingUser == m_signingUser)
        return;

    m_signingUser = signingUser;

    emit changed();
}

void MerBuildConfigurationAspect::setSigningPassphraseFile(const QString &passphraseFile)
{
    if (passphraseFile == m_signingPassphraseFile)
        return;

    m_signingPassphraseFile = passphraseFile;

    emit changed();
}

void MerBuildConfigurationAspect::addToEnvironment(Environment &env) const
{
    QStringList parts;

    if (!m_specFilePath.isEmpty())
        parts << "-c" << QtcProcess::quoteArgUnix("specfile=" + m_specFilePath);

    if (!m_sfdkOptionsString.isEmpty()) {
        const bool abortOnMeta = true;
        QtcProcess::SplitError error;
        const QStringList options = QtcProcess::splitArgs(m_sfdkOptionsString, Utils::OsTypeLinux,
                abortOnMeta, &error);
        // Check it silently, the UI already provides error feedback.
        // (According to docs QLineEdit::finished is only emitted when
        // validation passes. The reality seems to be less ideal.)
        if (error != QtcProcess::SplitOk)
            return;
        for (const QString &option : options)
            parts << "-c" << option;
    }

    if (m_signPackages) {
        if (m_signingUser.isValid())
            parts << "-c"
                  << "package.signing-user=" + QtcProcess::quoteArgUnix(m_signingUser.fingerprint);
        if (!m_signingPassphraseFile.isEmpty())
            parts << "-c"
                  << "package.signing-passphrase-file=" + QtcProcess::quoteArgUnix(m_signingPassphraseFile);
    }

    if (!parts.isEmpty())
        env.appendOrSet(Sfdk::Constants::MER_SSH_SFDK_OPTIONS, parts.join(' '));
}

void MerBuildConfigurationAspect::fromMap(const QVariantMap &map)
{
    m_specFilePath = map.value(SPEC_FILE_PATH).toString();
    m_sfdkOptionsString = map.value(SFDK_CONFIGURATION_OPTIONS).toString();

    m_signPackages = map.value(MER_BUILD_CONFIGURATION_SIGN_PACKAGES, m_signPackages).toBool();
    m_signingUser = Sfdk::GpgKeyInfo::fromString(
            map.value(QLatin1String(MER_BUILD_CONFIGURATION_SIGNING_USER)).toString());
    m_signingPassphraseFile =
            map.value(MER_BUILD_CONFIGURATION_SIGNING_PASSPHRASE_FILE, m_signingPassphraseFile)
            .toString();
}

void MerBuildConfigurationAspect::toMap(QVariantMap &map) const
{
    map.insert(SPEC_FILE_PATH, m_specFilePath);
    map.insert(SFDK_CONFIGURATION_OPTIONS, m_sfdkOptionsString);
    map.insert(MER_BUILD_CONFIGURATION_SIGN_PACKAGES, m_signPackages);
    map.insert(MER_BUILD_CONFIGURATION_SIGNING_USER, m_signingUser.toString());
    map.insert(MER_BUILD_CONFIGURATION_SIGNING_PASSPHRASE_FILE, m_signingPassphraseFile);
}

} // Internal
} // Mer

#include "merbuildconfigurationaspect.moc"
