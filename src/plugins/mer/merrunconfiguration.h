/****************************************************************************
**
** Copyright (C) 2012-2016,2018 Jolla Ltd.
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

#ifndef MERRUNCONFIGURATION_H
#define MERRUNCONFIGURATION_H

#include <projectexplorer/runconfiguration.h>

namespace Mer {
namespace Internal {

class MerRunConfiguration : public ProjectExplorer::RunConfiguration
{
    Q_OBJECT
public:
    MerRunConfiguration(ProjectExplorer::Target *target, Utils::Id id);

    ProjectExplorer::Runnable runnable() const override;
    QString disabledReason() const override;
    bool isEnabled() const override;

private:
    mutable QString m_disabledReason;
};

class MerRunConfigurationFactory : public ProjectExplorer::RunConfigurationFactory
{
public:
    MerRunConfigurationFactory();
};

} // namespace Internal
} // namespace Mer

#endif // MERRUNCONFIGURATION_H
