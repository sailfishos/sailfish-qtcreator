/****************************************************************************
**
** Copyright (C) 2012 - 2014 Jolla Ltd.
** Contact: http://jolla.com/
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

#ifndef MERDEPLOYSTEPS_H
#define MERDEPLOYSTEPS_H

#include "ui_merdeploystep.h"
#include "merabstractvmstartstep.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <remotelinux/abstractremotelinuxdeploystep.h>
#include <ssh/sshconnection.h>

#include <QDialog>
#include <QFutureWatcher>

namespace Mer {
namespace Internal {

namespace Ui {
class MerRpmInfo;
}

class MerDeployConfiguration;
class MerNamedCommandDeployService;

class MerProcessStep: public ProjectExplorer::AbstractProcessStep
{
public:
    enum InitOption {
        NoInitOption = 0x00,
        DoNotNeedDevice = 0x01,
    };
    Q_DECLARE_FLAGS(InitOptions, InitOption)

    explicit MerProcessStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    bool init(QList<const BuildStep *> &earlierSteps) override;
    bool init(QList<const BuildStep *> &earlierSteps, InitOptions options);
    QString arguments() const;
    void setArguments(const QString &arguments);

protected:
    MerDeployConfiguration *deployConfiguration() const;

private:
    QString m_arguments;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MerProcessStep::InitOptions)

class MerEmulatorStartStep : public MerAbstractVmStartStep
{
    Q_OBJECT
public:
    explicit MerEmulatorStartStep(ProjectExplorer::BuildStepList *bsl);
    bool init(QList<const BuildStep *> &earlierSteps) override;
    static Core::Id stepId();
    static QString displayName();
};

class MerConnectionTestStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    explicit MerConnectionTestStep(ProjectExplorer::BuildStepList *bsl);

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;
    bool runInGuiThread() const override;

    static Core::Id stepId();
    static QString displayName();

private slots:
    void onConnected();
    void onConnectionFailure();
    void checkForCancel();

private:
    void finish(bool result);

private:
    QFutureInterface<bool> *m_futureInterface;
    QSsh::SshConnection *m_connection;
    QTimer *m_checkForCancelTimer;
};

class MerPrepareTargetStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    explicit MerPrepareTargetStep(ProjectExplorer::BuildStepList *bsl);

    bool init(QList<const BuildStep *> &earlierSteps) override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    bool immutable() const override;
    bool runInGuiThread() const override;

    static Core::Id stepId();
    static QString displayName();

private slots:
    void onImplFinished();

private:
    ProjectExplorer::BuildStep *m_impl;
    QFutureWatcher<bool> m_watcher;
};

class MerMb2RsyncDeployStep : public MerProcessStep
{
    Q_OBJECT
public:
    explicit MerMb2RsyncDeployStep(ProjectExplorer::BuildStepList *bsl);
    bool init(QList<const BuildStep *> &earlierSteps) override;
    bool immutable() const override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    static Core::Id stepId();
    static QString displayName();
};

class MerLocalRsyncDeployStep : public MerProcessStep
{
    Q_OBJECT
public:
    explicit MerLocalRsyncDeployStep(ProjectExplorer::BuildStepList *bsl);
    bool init(QList<const BuildStep *> &earlierSteps) override;
    bool immutable() const override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    static Core::Id stepId();
    static QString displayName();
};

class MerMb2RpmDeployStep : public MerProcessStep
{
    Q_OBJECT
public:
    explicit MerMb2RpmDeployStep(ProjectExplorer::BuildStepList *bsl);
    bool init(QList<const BuildStep *> &earlierSteps) override;
    bool immutable() const override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    static Core::Id stepId();
    static QString displayName();
};

//TODO: HACK
class MerMb2RpmBuildStep : public MerProcessStep
{
    Q_OBJECT
public:
    explicit MerMb2RpmBuildStep(ProjectExplorer::BuildStepList *bsl);
    bool init(QList<const BuildStep *> &earlierSteps) override;
    bool immutable() const override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    static Core::Id stepId();
    static QString displayName();
    void processFinished(int exitCode, QProcess::ExitStatus status) override;
    void stdOutput(const QString &line) override;
    QStringList packagesFilePath() const;
private:
    QString m_sharedHome;
    QString m_sharedSrc;
    QStringList m_packages;
};

class RpmInfo: public QDialog
{
    Q_OBJECT
public:
    RpmInfo(const QStringList &list, QWidget *parent);
    ~RpmInfo();
public slots:
    void copyToClipboard();
    void openContainingFolder();
private:
    Ui::MerRpmInfo *m_ui;
    QStringList m_list;
};

class MerRpmValidationStep : public MerProcessStep
{
    Q_OBJECT
public:
    explicit MerRpmValidationStep(ProjectExplorer::BuildStepList *bsl);
    bool init(QList<const BuildStep *> &earlierSteps) override;
    bool immutable() const override;
    void run(QFutureInterface<bool> &fi) override;
    ProjectExplorer::BuildStepConfigWidget *createConfigWidget() override;
    static Core::Id stepId();
    static QString displayName();
private:
    MerMb2RpmBuildStep *m_packagingStep;
};

class MerDeployStepWidget : public ProjectExplorer::BuildStepConfigWidget
{
    Q_OBJECT
public:
    MerDeployStepWidget(MerProcessStep *step);
    QString displayName() const;
    QString summaryText() const;
    QString commnadText() const;
    void setCommandText(const QString& commandText);
    void setDisplayName(const QString& summaryText);
    void setSummaryText(const QString& displayText);
private slots:
    void commandArgumentsLineEditTextEdited();
private:
    MerProcessStep *m_step;
    Ui::MerDeployStepWidget m_ui;
    QString m_displayText;
    QString m_summaryText;
};

class MerNamedCommandDeployStep : public RemoteLinux::AbstractRemoteLinuxDeployStep
{
    Q_OBJECT
public:
    MerNamedCommandDeployStep(ProjectExplorer::BuildStepList *bsl, Core::Id id);
    RemoteLinux::AbstractRemoteLinuxDeployService *deployService() const override;
protected:
    bool initInternal(QString *error = 0) override;
    void setCommand(const QString &name, const QString &command);
private:
    MerNamedCommandDeployService *m_deployService;
};

class MerResetAmbienceDeployStep : public MerNamedCommandDeployStep
{
    Q_OBJECT
public:
    explicit MerResetAmbienceDeployStep(ProjectExplorer::BuildStepList *bsl);
    static Core::Id stepId();
    static QString displayName();
};

template <class Step>
class MerDeployStepFactory : public ProjectExplorer::BuildStepFactory
{
public:
    MerDeployStepFactory()
    {
        registerStep<Step>(Step::stepId());
        setDisplayName(Step::displayName());
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_DEPLOY);
    }

    bool canHandle(ProjectExplorer::BuildStepList *bsl) const override
    {
        if (!BuildStepFactory::canHandle(bsl))
            return false;

        if (!qobject_cast<MerDeployConfiguration *>(bsl->parent()))
            return false;

        return true;
    }
};

} // namespace Internal
} // namespace Mer

#endif // MERDEPLOYSTEPS_H
