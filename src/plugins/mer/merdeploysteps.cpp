/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#include "merconstants.h"
#include "merdeploysteps.h"
#include "mersdkmanager.h"
#include <utils/qtcassert.h>
#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>
#include <qtsupport/baseqtversion.h>
#include <qtsupport/qtkitinformation.h>
#include <coreplugin/idocument.h>
#include <extensionsystem/pluginmanager.h>
#include <coreplugin/variablemanager.h>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

class MerSimpleBuildStepConfigWidget : public BuildStepConfigWidget {
public:

    MerSimpleBuildStepConfigWidget(const QString& displayText, const QString& summaryText)
        :m_displayText(displayText),m_summaryText(summaryText){}

    QString summaryText() const
    {
        return QString::fromLatin1("<b>%1:</b> %2").arg(displayName()).arg(m_summaryText);
    }

    QString displayName() const
    {
        return m_displayText;
    }

     bool showWidget() const { return false; }

private:
    QString m_displayText;
    QString m_summaryText;
};


MerProcessStep::MerProcessStep(ProjectExplorer::BuildStepList *bsl,const Core::Id id)
    :AbstractProcessStep(bsl,id)
{

}

MerProcessStep::MerProcessStep(ProjectExplorer::BuildStepList *bsl, MerProcessStep *bs)
    :AbstractProcessStep(bsl,bs)
{

}

QString MerProcessStep::arguments() const
{
    return m_arguments;
}

void MerProcessStep::setArguments(const QString &arguments)
{
    m_arguments = arguments;
}

///////////////////////////////////////////////////////////////////////////////////////////

const Core::Id MerRsyncDeployStep::stepId()
{
    return Core::Id("Qt4ProjectManager.MerRsyncDeployStep");
}

QString MerRsyncDeployStep::displayName()
{
    return tr("Mb2(rsync)");
}

MerRsyncDeployStep::MerRsyncDeployStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}

MerRsyncDeployStep::MerRsyncDeployStep(BuildStepList *bsl, MerRsyncDeployStep *bs)
    : MerProcessStep(bsl, bs)
{
    setDefaultDisplayName(displayName());
}

bool MerRsyncDeployStep::init()
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();

    if (!bc) {
        addOutput(tr("Cannot deploy: No active build configuration."),
            ErrorMessageOutput);
        return false;
    }

    const QtSupport::BaseQtVersion *const qtVersion = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (!qtVersion) {
        addOutput(tr("Cannot deploy: Unusable build configuration."),
            ErrorMessageOutput);
        return false;

    }

    //TODO sanity checks;
    const QString projectDirectory = project()->projectDirectory();
    const QString wrapperScriptsDir = qtVersion->qmakeCommand().parentDir().toString();
    const QString mb2Command = wrapperScriptsDir + QLatin1Char('/') + QLatin1String(Constants::MER_WRAPPER_MB);
    //const QString projectName = QFileInfo(project()->document()->fileName()).baseName();
    setArguments(QLatin1String("deploy -rsync"));

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc ? bc->macroExpander() : Core::VariableManager::instance()->macroExpander());
    pp->setEnvironment(bc ? bc->environment() : Utils::Environment::systemEnvironment());
    pp->setWorkingDirectory(projectDirectory);
    pp->setCommand(mb2Command);
    pp->setArguments(arguments());
    return AbstractProcessStep::init();
}

bool MerRsyncDeployStep::immutable() const
{
    return false;
}

void MerRsyncDeployStep::run(QFutureInterface<bool> &fi)
{
   emit addOutput(tr("Deploying binaries..."), MessageOutput);
   AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerRsyncDeployStep::createConfigWidget()
{
    return new MerDeployStepWidget(displayName(),tr("Deploys with rsync."),this);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////


const Core::Id MerRpmDeployStep::stepId()
{
    return Core::Id("Qt4ProjectManager.MerRpmDeployStep");
}

QString MerRpmDeployStep::displayName()
{
    return tr("Mb2(rpm)");
}

MerRpmDeployStep::MerRpmDeployStep(BuildStepList *bsl)
    : MerProcessStep(bsl, stepId())
{
    setDefaultDisplayName(displayName());
}

MerRpmDeployStep::MerRpmDeployStep(BuildStepList *bsl, MerRpmDeployStep *bs)
    : MerProcessStep(bsl,bs)
{
    setDefaultDisplayName(displayName());
}

bool MerRpmDeployStep::init()
{
    BuildConfiguration *bc = buildConfiguration();
    if (!bc)
        bc = target()->activeBuildConfiguration();

    if (!bc) {
        addOutput(tr("Cannot deploy: No active build configuration."),
            ErrorMessageOutput);
        return false;
    }

    const QtSupport::BaseQtVersion *const qtVersion = QtSupport::QtKitInformation::qtVersion(target()->kit());
    if (!qtVersion) {
        addOutput(tr("Cannot deploy: Unusable build configuration."),
            ErrorMessageOutput);
        return false;

    }

    //TODO sanity checks;
    const QString projectDirectory = project()->projectDirectory();
    const QString wrapperScriptsDir = qtVersion->qmakeCommand().parentDir().toString();
    const QString mb2Command = wrapperScriptsDir + QLatin1Char('/') + QLatin1String(Constants::MER_WRAPPER_MB);
    //const QString projectName = QFileInfo(project()->document()->fileName()).baseName();
    setArguments(QLatin1String("deploy"));

    ProcessParameters *pp = processParameters();
    pp->setMacroExpander(bc ? bc->macroExpander() : Core::VariableManager::instance()->macroExpander());
    pp->setEnvironment(bc ? bc->environment() : Utils::Environment::systemEnvironment());
    pp->setWorkingDirectory(projectDirectory);
    pp->setCommand(mb2Command);
    pp->setArguments(arguments());
    return AbstractProcessStep::init();
}

bool MerRpmDeployStep::immutable() const
{
    return false;
}

void MerRpmDeployStep::run(QFutureInterface<bool> &fi)
{
    emit addOutput(tr("Deploying rpm package..."), MessageOutput);
    AbstractProcessStep::run(fi);
}

BuildStepConfigWidget *MerRpmDeployStep::createConfigWidget()
{
     return new MerDeployStepWidget(displayName(),tr("Deploys rpm package."),this);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////


MerDeployStepWidget::MerDeployStepWidget(const QString& displayText, const QString& summaryText, MerProcessStep *step)
        : m_step(step), m_displayText(displayText),m_summaryText(summaryText)
{
    m_ui.setupUi(this);
    m_ui.commandArgumentsLineEdit->setText(m_step->arguments());
    connect(m_ui.commandArgumentsLineEdit, SIGNAL(textEdited(QString)),
            this, SLOT(commandArgumentsLineEditTextEdited()));
}

QString MerDeployStepWidget::summaryText() const
{
    return QString::fromLatin1("<b>%1:</b> %2").arg(displayName()).arg(m_summaryText);
}

QString MerDeployStepWidget::displayName() const
{
    return m_displayText;
}

void MerDeployStepWidget::commandArgumentsLineEditTextEdited()
{
    m_step->setArguments(m_ui.commandArgumentsLineEdit->text());
}

} // Internal
} // Mer
