/****************************************************************************
**
** Copyright (C) 2016-2018 Jolla Ltd.
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

#ifndef MERQMLRUNCONFIGURATIONFACTORY_H
#define MERQMLRUNCONFIGURATIONFACTORY_H

#include <projectexplorer/runconfiguration.h>

namespace Mer {
namespace Internal {

class MerQmlRunConfigurationFactory : public ProjectExplorer::FixedRunConfigurationFactory
{
public:
    MerQmlRunConfigurationFactory();

    bool canHandle(ProjectExplorer::Target *parent) const override;
};

} // namespace Internal
} // namespace Mer

#endif // MERQMLRUNCONFIGURATIONFACTORY_H
