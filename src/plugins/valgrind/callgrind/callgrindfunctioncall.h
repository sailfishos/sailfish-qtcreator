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

#include <QMetaType>

QT_BEGIN_NAMESPACE
template <typename T> class QVector;
QT_END_NAMESPACE

namespace Valgrind {
namespace Callgrind {

class Function;

/**
 * This represents a function call.
 */
class FunctionCall
{
public:
    explicit FunctionCall();
    ~FunctionCall();

    /// the called function
    const Function *callee() const;
    void setCallee(const Function *function);

    /// the calling function
    const Function *caller() const;
    void setCaller(const Function *function);

    /// how often the function was called
    quint64 calls() const;
    void setCalls(quint64 calls);

    /**
     * Destination position data for the given position-index @p posIdx
     * @see ParseData::positions()
     */
    quint64 destination(int posIdx) const;
    QVector<quint64> destinations() const;
    void setDestinations(const QVector<quint64> &destinations);

    /**
     * Inclusive cost of the function call.
     * @see ParseData::events()
     */
    quint64 cost(int event) const;
    QVector<quint64> costs() const;
    void setCosts(const QVector<quint64> &costs);

private:
    Q_DISABLE_COPY(FunctionCall)

    class Private;
    Private *d;
};

} // namespace Callgrind
} // namespace Valgrind

Q_DECLARE_METATYPE(const Valgrind::Callgrind::FunctionCall *)
