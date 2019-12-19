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

#include "mertargetmanagementdialog.h"

#include "merlogging.h"
#include "ui_mertargetmanagementpage.h"
#include "ui_mertargetmanagementpackagespage.h"
#include "ui_mertargetmanagementprogresspage.h"

#include <sfdk/buildengine.h>
#include <sfdk/sdk.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QDir>
#include <QProcess>
#include <QScrollBar>

using namespace Sfdk;
using namespace Utils;

namespace Mer {
namespace Internal {

/*!
 * \class MerTargetManagementProcess
 */

class MerTargetManagementProcess : public QProcess
{
    Q_OBJECT

public:
    MerTargetManagementProcess(const QString &targetName, QObject *parent = nullptr)
        : QProcess(parent)
        , m_targetName(targetName)
    {
    }

    bool listPackages(const QStringList &patterns, QList<QPair<QString, bool>> *out)
    {
        QStringList args{"tools", "target", "package-list", m_targetName};
        args += patterns;

        if (!execSfdk(args))
            return false;

        QTextStream stream(readAllStandardOutput());
        while (!stream.atEnd()) {
            const QString line = stream.readLine();
            if (line.isEmpty())
                continue;
            const QStringList columns = line.split(' ', QString::SkipEmptyParts);
            QTC_ASSERT(columns.count() >= 2, return false);
            const QString name = columns.at(0);
            const bool installed = columns.at(1) == "installed";
            out->append(qMakePair(name, installed));
        }

        return true;
    }

    bool installPackages(const QStringList &packageNames)
    {
        QStringList args{"tools", "target", "package-install", m_targetName};
        args += packageNames;
        return execSfdk(args);
    }

    bool removePackages(const QStringList &packageNames)
    {
        QStringList args{"tools", "target", "package-remove", m_targetName};
        args += packageNames;
        return execSfdk(args);
    }

    bool refresh()
    {
        const QStringList args{"engine", "exec", "sdk-manage", "target", "refresh", m_targetName};
        return execSfdk(args);
    }

    bool synchronize()
    {
        const QStringList args{"engine", "exec", "sdk-manage", "target", "sync", m_targetName};
        return execSfdk(args);
    }

private:
    bool execSfdk(const QStringList &arguments)
    {
        setProgram(Sdk::installationPath() + "/bin/sfdk" QTC_HOST_EXE_SUFFIX);
        setWorkingDirectory(QDir::homePath());
        setArguments(arguments);

        start();

        if (!waitForStarted(-1)) {
            qCWarning(Log::mer) << "Could not start sfdk";
            return false;
        }

        while (state() != QProcess::NotRunning)
            QCoreApplication::processEvents(QEventLoop::WaitForMoreEvents);

        return exitStatus() == QProcess::NormalExit && exitCode() == EXIT_SUCCESS;
    }

private:
    QString m_targetName;
};

/*!
 * \class MerTargetManagementBaseWizardPage
 */

class MerTargetManagementBaseWizardPage : public QWizardPage
{
    Q_OBJECT

public:
    enum PageId {
        InitialPage,
        PackagesPage,
        ProgressPage,
    };

public:
    using QWizardPage::QWizardPage;

protected:
    MerTargetManagementDialog *wizard() const
    {
        return static_cast<MerTargetManagementDialog *>(QWizardPage::wizard());
    }
};

/*!
 * \class MerTargetManagementPage
 */

class MerTargetManagementPage : public MerTargetManagementBaseWizardPage
{
    Q_OBJECT

public:
    explicit MerTargetManagementPage(BuildEngine *engine, QWidget *parent)
        : MerTargetManagementBaseWizardPage(parent)
        , m_ui(std::make_unique<Ui::MerTargetManagementPage>())
        , m_engine(engine)
    {
        m_ui->setupUi(this);

        for (const QString &targetName : engine->buildTargetNames())
            new QTreeWidgetItem(m_ui->targetsTreeWidget, {targetName});

        if (m_ui->targetsTreeWidget->topLevelItemCount() > 0) {
            m_ui->targetsTreeWidget->setCurrentItem(m_ui->targetsTreeWidget->topLevelItem(0),
                    0, QItemSelectionModel::Select);
        }

        connect(m_ui->targetsTreeWidget, &QTreeWidget::currentItemChanged,
                this, &MerTargetManagementPage::completeChanged);
    }

    QString targetName() const
    {
        return m_ui->targetsTreeWidget->currentItem()
            ? m_ui->targetsTreeWidget->currentItem()->text(0)
            : QString();
    }

    void setTargetName(const QString &targetName)
    {
        const QList<QTreeWidgetItem *> items = m_ui->targetsTreeWidget->findItems(targetName,
                Qt::MatchExactly);
        QTC_ASSERT(!items.isEmpty(), return);
        QTC_ASSERT(items.count() == 1, return);
        m_ui->targetsTreeWidget->setCurrentItem(items.first());
    }

    MerTargetManagementDialog::Action action() const
    {
        if (m_ui->manageRadioButton->isChecked())
            return MerTargetManagementDialog::ManagePackages;
        else if (m_ui->refreshRadioButton->isChecked())
            return MerTargetManagementDialog::Refresh;
        else if (m_ui->synchronizeRadioButton->isChecked())
            return MerTargetManagementDialog::Synchronize;

        QTC_ASSERT(false, return MerTargetManagementDialog::ManagePackages);
    }

    void setAction(MerTargetManagementDialog::Action action)
    {
        switch (action) {
        case MerTargetManagementDialog::ManagePackages:
            m_ui->manageRadioButton->click();
            return;
        case MerTargetManagementDialog::Refresh:
            m_ui->refreshRadioButton->click();
            return;
        case MerTargetManagementDialog::Synchronize:
            m_ui->synchronizeRadioButton->click();
            return;
        }

        QTC_CHECK(false);
    }

    bool isComplete() const override
    {
        return !targetName().isEmpty();
    }

    int nextId() const override
    {
        return m_ui->manageRadioButton->isChecked()
            ? MerTargetManagementBaseWizardPage::PackagesPage
            : MerTargetManagementBaseWizardPage::ProgressPage;
    }

private:
    std::unique_ptr<Ui::MerTargetManagementPage> m_ui;
    BuildEngine *m_engine;
};

/*!
 * \class MerTargetManagementPackagesPage
 */

class MerTargetManagementPackagesPage : public MerTargetManagementBaseWizardPage
{
    Q_OBJECT

    enum Columns {
        NameColumn,
        InstallColumn,
        RemoveColumn,
    };

public:
    explicit MerTargetManagementPackagesPage(QWidget *parent)
        : MerTargetManagementBaseWizardPage(parent)
        , m_ui(std::make_unique<Ui::MerTargetManagementPackagesPage>())
    {
        m_ui->setupUi(this);

        m_ui->searchTreeWidget->header()->setSectionResizeMode(0, QHeaderView::Stretch);
        m_ui->searchTreeWidget->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        m_ui->searchTreeWidget->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);

        connect(m_ui->searchLineEdit, &QLineEdit::textChanged,
                this, &MerTargetManagementPackagesPage::onSearchStringChanged);
        connect(m_ui->searchButton, &QPushButton::clicked,
                this, &MerTargetManagementPackagesPage::searchPackages);
        connect(m_ui->searchTreeWidget, &QTreeWidget::itemChanged,
                this, &MerTargetManagementPackagesPage::onSearchTreeItemChanged);
    }

public:
    QStringList packagesToInstall() const
    {
        QStringList packageNames;
        for (int i = 0; i < m_ui->installTreeWidget->topLevelItemCount(); ++i)
            packageNames += m_ui->installTreeWidget->topLevelItem(i)->text(0);
        return packageNames;
    }

    void setPackagesToInstall(const QStringList &packageNames)
    {
        m_ui->installTreeWidget->clear();
        for (const QString &name : packageNames)
            new QTreeWidgetItem(m_ui->installTreeWidget, {name});
        emit completeChanged();
    }

    QStringList packagesToRemove() const
    {
        QStringList packageNames;
        for (int i = 0; i < m_ui->removeTreeWidget->topLevelItemCount(); ++i)
            packageNames += m_ui->removeTreeWidget->topLevelItem(i)->text(0);
        return packageNames;
    }

    void setPackagesToRemove(const QStringList &packageNames)
    {
        m_ui->removeTreeWidget->clear();
        for (const QString &name : packageNames)
            new QTreeWidgetItem(m_ui->removeTreeWidget, {name});
        emit completeChanged();
    }

    bool isComplete() const override
    {
        return m_ui->installTreeWidget->topLevelItemCount() > 0
            || m_ui->removeTreeWidget->topLevelItemCount() > 0;
    }

private:
    void onSearchStringChanged(const QString &searchString)
    {
        const QStringList split = searchString.split(' ', QString::SkipEmptyParts);
        const QList<int> lengths = Utils::transform(split, &QString::length);
        const int minLength = *std::min_element(lengths.begin(), lengths.end());

        if (split.isEmpty() || minLength >= 3) {
            m_filters = split;
            m_ui->searchLineEdit->setPalette(palette());
        } else {
            m_filters.clear();
            QPalette palette = this->palette();
            palette.setColor(QPalette::Text, Qt::red);
            m_ui->searchLineEdit->setPalette(palette);
        }

        m_ui->searchButton->setEnabled(!m_filters.isEmpty());
    }

    void searchPackages()
    {
        setEnabled(false);

        MerTargetManagementProcess process(wizard()->targetName());
        QList<QPair<QString, bool>> packages;

        m_ui->searchTreeWidget->clear();

        if (!process.listPackages(m_filters, &packages)) {
            qCWarning(Log::mer) << "Failed to search for packages" << m_filters
                << "error:" << process.readAllStandardError();
        } else {
            const QSet<QString> packagesToInstall = this->packagesToInstall().toSet();
            const QSet<QString> packagesToRemove = this->packagesToRemove().toSet();

            for (const auto &packageInfo : packages) {
                const QString name = packageInfo.first;
                const bool installed = packageInfo.second;
                const bool selected = installed
                    ? packagesToRemove.contains(name)
                    : packagesToInstall.contains(name);
                const QStringList columns{
                    /* NameColumn    */ name,
                    /* InstallColumn */ installed ? QString() : tr("Install"),
                    /* RemoveColumn  */ installed ? tr("Remove") : QString()
                };
                auto *item = new QTreeWidgetItem(static_cast<QTreeWidget *>(nullptr), columns);
                item->setCheckState(installed ? RemoveColumn : InstallColumn,
                        selected ? Qt::Checked : Qt::Unchecked);
                m_ui->searchTreeWidget->addTopLevelItem(item);
            }
        }

        setEnabled(true);
    }

    void onSearchTreeItemChanged(QTreeWidgetItem *item, int column)
    {
        QTC_ASSERT(column == InstallColumn || column == RemoveColumn, return);

        const QString name = item->text(NameColumn);
        const bool add = item->checkState(column) == Qt::Checked;
        QTreeWidget *const target = column == InstallColumn
            ? m_ui->installTreeWidget
            : m_ui->removeTreeWidget;

        const QList<QTreeWidgetItem *> current = target->findItems(name, Qt::MatchExactly);
        if (add) {
            QTC_ASSERT(current.isEmpty(), return);
            new QTreeWidgetItem(target, {name});
        } else {
            QTC_ASSERT(!current.isEmpty(), return);
            qDeleteAll(current);
        }

        emit completeChanged();
    }

private:
    std::unique_ptr<Ui::MerTargetManagementPackagesPage> m_ui;
    QStringList m_filters;
};

/*!
 * \class MerTargetManagementProgressPage
 */

class MerTargetManagementProgressPage : public MerTargetManagementBaseWizardPage
{
    Q_OBJECT

public:
    explicit MerTargetManagementProgressPage(QWidget *parent)
        : MerTargetManagementBaseWizardPage(parent)
        , m_ui(std::make_unique<Ui::MerTargetManagementProgressPage>())
    {
        m_ui->setupUi(this);
        setFinalPage(true);
        m_ui->textBrowser->hide();

        connect(m_ui->detailsButton, &QPushButton::toggled,
                this, &MerTargetManagementProgressPage::showDetails);
    }

    void setStatus(const QString &message, bool complete)
    {
        m_ui->statusLabel->setText(message);
        if (complete) {
            m_ui->progressBar->setRange(0, 100);
            m_ui->progressBar->setValue(100);
            emit completeChanged();
        }
    }

    void appendDetails(const QString &details)
    {
        // Adapted from https://stackoverflow.com/a/43755944
        static QRegularExpression ansiControlSequences(R"(\x1B\[([0-9]{1,2}(;[0-9]{1,2})*)?[mK])");

        QString clean = details;
        clean.remove(ansiControlSequences);

        QScrollBar *const scrollBar = m_ui->textBrowser->verticalScrollBar();
        const bool follow = !scrollBar->isVisible()
            || scrollBar->value() == scrollBar->maximum();

        m_ui->textBrowser->setPlainText(m_ui->textBrowser->toPlainText() + clean);

        if (follow)
            scrollBar->setValue(scrollBar->maximum());
    }

    void initializePage() override
    {
        m_ui->progressBar->setRange(0, 0);
    }

    bool isComplete() const override
    {
        return m_ui->progressBar->value() == 100;
    }

private:
    void showDetails(bool show)
    {
        m_ui->textBrowser->setVisible(show);

        if (show) {
            QScrollBar *const scrollBar = m_ui->textBrowser->verticalScrollBar();
            scrollBar->setValue(scrollBar->maximum());
        }
    }

private:
    std::unique_ptr<Ui::MerTargetManagementProgressPage> m_ui;
};

/*!
 * \class MerTargetManagementDialog
 */

MerTargetManagementDialog::MerTargetManagementDialog(Sfdk::BuildEngine *engine, QWidget *parent)
    : QWizard(parent)
    , m_engine(engine)
    , m_initialPage(std::make_unique<MerTargetManagementPage>(engine, this))
    , m_packagesPage(std::make_unique<MerTargetManagementPackagesPage>(this))
    , m_progressPage(std::make_unique<MerTargetManagementProgressPage>(this))
{
    setMinimumSize(700, 300);

    setWindowTitle(tr("Manage Build Targets"));

    setPage(MerTargetManagementBaseWizardPage::InitialPage, m_initialPage.get());
    setPage(MerTargetManagementBaseWizardPage::PackagesPage, m_packagesPage.get());
    setPage(MerTargetManagementBaseWizardPage::ProgressPage, m_progressPage.get());

    setWizardStyle(QWizard::ClassicStyle);

    setOption(QWizard::NoBackButtonOnLastPage);
    setOption(QWizard::NoCancelButtonOnLastPage);

    connect(this, &QWizard::currentIdChanged, this, &MerTargetManagementDialog::onCurrentIdChanged);
}

MerTargetManagementDialog::~MerTargetManagementDialog()
{
}

QString MerTargetManagementDialog::targetName() const
{
    return m_initialPage->targetName();
}

void MerTargetManagementDialog::setTargetName(const QString &targetName)
{
    m_initialPage->setTargetName(targetName);
}

MerTargetManagementDialog::Action MerTargetManagementDialog::action() const
{
    return m_initialPage->action();
}

void MerTargetManagementDialog::setAction(Action action)
{
    m_initialPage->setAction(action);
}

QStringList MerTargetManagementDialog::packagesToInstall() const
{
    return m_packagesPage->packagesToInstall();
}

void MerTargetManagementDialog::setPackagesToInstall(const QStringList &packageNames)
{
    return m_packagesPage->setPackagesToInstall(packageNames);
}

QStringList MerTargetManagementDialog::packagesToRemove() const
{
    return m_packagesPage->packagesToRemove();
}

void MerTargetManagementDialog::setPackagesToRemove(const QStringList &packageNames)
{
    return m_packagesPage->setPackagesToRemove(packageNames);
}

void MerTargetManagementDialog::reject()
{
    if (currentId() == MerTargetManagementBaseWizardPage::ProgressPage
            && !m_progressPage->isComplete()) {
        return;
    }

    QWizard::reject();
}

void MerTargetManagementDialog::onCurrentIdChanged(int id)
{
    if (id != MerTargetManagementBaseWizardPage::ProgressPage)
        return;

    switch (action()) {
    case ManagePackages:
        managePackages();
        return;
    case Refresh:
        refresh();
        return;
    case Synchronize:
        synchronize();
        return;
    }
}

void MerTargetManagementDialog::managePackages()
{
    MerTargetManagementProcess process(targetName());

    connect(&process, &QProcess::readyReadStandardOutput, [&]() {
        m_progressPage->appendDetails(QString::fromLocal8Bit(process.readAllStandardOutput()));
    });
    connect(&process, &QProcess::readyReadStandardError, [&]() {
        m_progressPage->appendDetails(QString::fromLocal8Bit(process.readAllStandardOutput()));
    });

    const QStringList packagesToRemove = this->packagesToRemove();
    if (!packagesToRemove.isEmpty()) {
        m_progressPage->setStatus(tr("Removing packages..."), false);
        if (!process.removePackages(packagesToRemove)) {
            m_progressPage->setStatus(tr("Package removal failed."), true);
            return;
        }
    }

    const QStringList packagesToInstall = this->packagesToInstall();
    if (!packagesToInstall.isEmpty()) {
        m_progressPage->setStatus(tr("Installing packages..."), false);
        if (!process.installPackages(packagesToInstall)) {
            m_progressPage->setStatus(tr("Package installation failed."), true);
            return;
        }
    }

    m_progressPage->setStatus(tr("Done"), true);
}

void MerTargetManagementDialog::refresh()
{
    MerTargetManagementProcess process(targetName());

    connect(&process, &QProcess::readyReadStandardOutput, [&]() {
        m_progressPage->appendDetails(QString::fromLocal8Bit(process.readAllStandardOutput()));
    });
    connect(&process, &QProcess::readyReadStandardError, [&]() {
        m_progressPage->appendDetails(QString::fromLocal8Bit(process.readAllStandardOutput()));
    });

    m_progressPage->setStatus(tr("Refreshing..."), false);

    if (!process.refresh()) {
        m_progressPage->setStatus(tr("Failed"), true);
        return;
    }

    m_progressPage->setStatus(tr("Done"), true);
}

void MerTargetManagementDialog::synchronize()
{
    MerTargetManagementProcess process(targetName());

    connect(&process, &QProcess::readyReadStandardOutput, [&]() {
        m_progressPage->appendDetails(QString::fromLocal8Bit(process.readAllStandardOutput()));
    });
    connect(&process, &QProcess::readyReadStandardError, [&]() {
        m_progressPage->appendDetails(QString::fromLocal8Bit(process.readAllStandardOutput()));
    });

    m_progressPage->setStatus(tr("Synchronizing..."), false);

    if (!process.synchronize()) {
        m_progressPage->setStatus(tr("Failed"), true);
        return;
    }

    m_progressPage->setStatus(tr("Done"), true);
}

} // namespace Internal
} // namespace Mer

#include "mertargetmanagementdialog.moc"
