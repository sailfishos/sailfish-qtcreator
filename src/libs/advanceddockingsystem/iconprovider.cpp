/****************************************************************************
**
** Copyright (C) 2020 Uwe Kindler
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
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or (at your option) any later version.
** The licenses are as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPLv21 included in the packaging
** of this file. Please review the following information to ensure
** the GNU Lesser General Public License version 2.1 requirements
** will be met: https://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "iconprovider.h"

#include <QVector>

namespace ADS {
    /**
     * Private data class (pimpl)
     */
    struct IconProviderPrivate
    {
        IconProvider *q;
        QVector<QIcon> m_userIcons{IconCount, QIcon()};

        /**
         * Private data constructor
         */
        IconProviderPrivate(IconProvider *parent);
    };
    // struct LedArrayPanelPrivate

    IconProviderPrivate::IconProviderPrivate(IconProvider *parent)
        : q(parent)
    {}

    IconProvider::IconProvider()
        : d(new IconProviderPrivate(this))
    {}

    IconProvider::~IconProvider()
    {
        delete d;
    }

    QIcon IconProvider::customIcon(eIcon iconId) const
    {
        Q_ASSERT(iconId < d->m_userIcons.size());
        return d->m_userIcons[iconId];
    }

    void IconProvider::registerCustomIcon(eIcon iconId, const QIcon &icon)
    {
        Q_ASSERT(iconId < d->m_userIcons.size());
        d->m_userIcons[iconId] = icon;
    }

} // namespace ADS
