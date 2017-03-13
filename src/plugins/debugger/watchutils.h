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

#pragma once

// NOTE: Don't add dependencies to other files.
// This is used in the debugger auto-tests.

#include <QString>

namespace Debugger {
namespace Internal {

bool isSkippableFunction(const QString &funcName, const QString &fileName);
bool isLeavableFunction(const QString &funcName, const QString &fileName);

bool hasLetterOrNumber(const QString &exp);
bool hasSideEffects(const QString &exp);
bool isKeyWord(const QString &exp);
bool isPointerType(const QString &type);
bool isCharPointerType(const QString &type);
bool startsWithDigit(const QString &str);
QString stripPointerType(QString type);
QString gdbQuoteTypes(const QString &type);
bool isFloatType(const QString &type);
bool isIntOrFloatType(const QString &type);
bool isIntType(const QString &type);

QString formatToolTipAddress(quint64 a);
QString removeObviousSideEffects(const QString &exp);

} // namespace Internal
} // namespace Debugger
