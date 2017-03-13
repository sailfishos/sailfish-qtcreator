/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "timelinetheme.h"

#include <utils/icon.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>
#include <utils/theme/theme.h>

#include <QIcon>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QQuickImageProvider>

namespace Timeline {

class TimelineImageIconProvider : public QQuickImageProvider
{
public:
    TimelineImageIconProvider()
        : QQuickImageProvider(Pixmap)
    {
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        Q_UNUSED(requestedSize)

        const QStringList idElements = id.split(QLatin1Char('/'));

        QTC_ASSERT(!idElements.isEmpty(), return QPixmap());
        const QString &iconName = idElements.first();
        const QIcon::Mode iconMode = (idElements.count() > 1
                                      && idElements.at(1) == QLatin1String("disabled"))
                ? QIcon::Disabled : QIcon::Normal;

        Utils::Icon icon;
        if (iconName == QLatin1String("prev"))
            icon = Utils::Icons::PREV_TOOLBAR;
        else if (iconName == QLatin1String("next"))
            icon = Utils::Icons::NEXT_TOOLBAR;
        else if (iconName == QLatin1String("zoom"))
            icon = Utils::Icons::ZOOM_TOOLBAR;
        else if (iconName == QLatin1String("rangeselection"))
            icon = Utils::Icon({{QLatin1String(":/timeline/ico_rangeselection.png"),
                                 Utils::Theme::IconsBaseColor}});
        else if (iconName == QLatin1String("rangeselected"))
            icon = Utils::Icon({{QLatin1String(":/timeline/ico_rangeselected.png"),
                                 Utils::Theme::IconsBaseColor}});
        else if (iconName == QLatin1String("selectionmode"))
            icon = Utils::Icon({{QLatin1String(":/timeline/ico_selectionmode.png"),
                                 Utils::Theme::IconsBaseColor}});

        const QSize iconSize(16, 16);
        const QPixmap result = icon.icon().pixmap(iconSize, iconMode);

        if (size)
            *size = result.size();
        return result;
    }
};

void TimelineTheme::setupTheme(QQmlEngine *engine)
{
    QQmlPropertyMap *themePropertyMap = new QQmlPropertyMap(engine);
    const QVariantHash creatorTheme = Utils::creatorTheme()->values();
    for (auto it = creatorTheme.constBegin(); it != creatorTheme.constEnd(); ++it)
        themePropertyMap->insert(it.key(), it.value());

    engine->rootContext()->setContextProperty(QLatin1String("creatorTheme"), themePropertyMap);

    engine->addImageProvider(QLatin1String("icons"), new TimelineImageIconProvider);
}

} // namespace Timeline
