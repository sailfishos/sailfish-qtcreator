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

#ifndef MERDEVICECONFIGURATIONWIZARD_H
#define MERDEVICECONFIGURATIONWIZARD_H

#include <projectexplorer/devicesupport/idevice.h>

#include <QWizard>

namespace Mer {
namespace Internal {

class MerDeviceConfigurationWizardPrivate;
class MerDeviceConfigurationWizard : public QWizard
{
public:
    explicit MerDeviceConfigurationWizard(Core::Id id, QWidget *parent = 0);
    ~MerDeviceConfigurationWizard();

    ProjectExplorer::IDevice::Ptr device();

    virtual int nextId() const;

private:
    MerDeviceConfigurationWizardPrivate * const d;
};

} // Internal
} // Mer

#endif // MERDEVICECONFIGURATIONWIZARD_H
