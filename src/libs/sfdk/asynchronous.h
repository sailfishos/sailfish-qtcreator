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

#pragma once

#include "sfdkglobal.h"

#include <QEventLoop>
#include <QPointer>

#include <tuple>
#include <utility>

namespace Sfdk {

template<typename... Ts>
using Functor = std::function<void(Ts...)>;

template<typename Fn, typename... Args>
void callIf(const QPointer<const QObject> &context, const Fn &fn, Args&&... args)
{
    if (context)
        fn(std::forward<Args>(args)...);
}

template<typename Fn, typename... InArgs, typename... OutArgs>
void execAsynchronous(std::tuple<OutArgs&...> outArgs, const Fn &fn, InArgs&&... inArgs)
{
	QEventLoop loop;
	auto whenDone = [&loop, &outArgs](OutArgs...as) {
		loop.quit();
		outArgs = std::tie(as...);
	};
	fn(std::forward<InArgs>(inArgs)..., &loop, whenDone);
	loop.exec();
}

} // namespace Sfdk
