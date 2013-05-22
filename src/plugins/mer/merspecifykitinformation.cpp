/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
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

#include "merspecifykitinformation.h"
#include "merconstants.h"
#include "merdevicefactory.h"
#include "mersdkkitinformation.h"

#include <utils/pathchooser.h>
#include <projectexplorer/projectexplorerconstants.h>

using namespace ProjectExplorer;

namespace Mer {
namespace Internal {

MerSpecifyKitInformationWidget::MerSpecifyKitInformationWidget(Kit *k) :
    KitConfigWidget(k)
{
    m_chooser = new Utils::PathChooser;
    m_chooser->setExpectedKind(Utils::PathChooser::File);
    m_chooser->setFileName(MerSpecifyKitInformation::specifyPath(k));
    connect(m_chooser, SIGNAL(changed(QString)), this, SLOT(pathWasChanged()));
}

QString MerSpecifyKitInformationWidget::displayName() const
{
    return tr("Specify:");
}

QString MerSpecifyKitInformationWidget::toolTip() const
{
    return tr("Path for specify tool.");
}

void MerSpecifyKitInformationWidget::refresh()
{
    if (!m_ignoreChange)
        m_chooser->setFileName(MerSpecifyKitInformation::specifyPath(m_kit));
}

void MerSpecifyKitInformationWidget::makeReadOnly()
{
    m_chooser->setEnabled(false);
}

bool MerSpecifyKitInformationWidget::visibleInKit()
{
    return MerDeviceFactory::canCreate(ProjectExplorer::DeviceTypeKitInformation::deviceTypeId(m_kit));
}

QWidget *MerSpecifyKitInformationWidget::mainWidget() const
{
    return m_chooser->lineEdit();
}

QWidget *MerSpecifyKitInformationWidget::buttonWidget() const
{
    return m_chooser->buttonAtIndex(0);
}

void MerSpecifyKitInformationWidget::pathWasChanged()
{
    m_ignoreChange = true;
    MerSpecifyKitInformation::setSpecifyPath(m_kit, m_chooser->fileName());
    m_ignoreChange = false;
}


MerSpecifyKitInformation::MerSpecifyKitInformation()
{
    setObjectName(QLatin1String("MerSpecifyKitInformation"));
}

Core::Id MerSpecifyKitInformation::dataId() const
{
    return Core::Id(Constants::MER_KIT_SPECIFY_INFORMATION);
}

unsigned int MerSpecifyKitInformation::priority() const
{
    return 23;
}

QVariant MerSpecifyKitInformation::defaultValue(Kit *k) const
{
    Q_UNUSED(k)
    return QString();
}

QList<Task> MerSpecifyKitInformation::validate(const Kit *k) const
{
    QList<Task> result;
    const Utils::FileName file = MerSpecifyKitInformation::specifyPath(k);
    if (!file.toFileInfo().isFile()) {
        const QString message = QCoreApplication::translate("MerSdk",
                                                            "No valid specify tool found");
        return QList<Task>() << Task(Task::Error, message, Utils::FileName(), -1,
                                     Core::Id(ProjectExplorer::Constants::TASK_CATEGORY_BUILDSYSTEM));
    }
    return result;
}

KitConfigWidget *MerSpecifyKitInformation::createConfigWidget(Kit *k) const
{
    return new MerSpecifyKitInformationWidget(k);
}

KitInformation::ItemList MerSpecifyKitInformation::toUserOutput(Kit *k) const
{
    return ItemList() << qMakePair(tr("Specify"), specifyPath(k).toUserOutput());
}

Utils::FileName MerSpecifyKitInformation::specifyPath(const Kit *k)
{
    if (!k)
        return Utils::FileName();
    return Utils::FileName::fromString(k->value(Core::Id(Constants::MER_KIT_SPECIFY_INFORMATION)).toString());
}

void MerSpecifyKitInformation::setSpecifyPath(Kit *k, const Utils::FileName &v)
{
    k->setValue(Core::Id(Constants::MER_KIT_SPECIFY_INFORMATION), v.toString());
}

} // Internal
} // Mer
