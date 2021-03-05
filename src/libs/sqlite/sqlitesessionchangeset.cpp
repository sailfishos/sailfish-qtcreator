/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include "sqlitesessionchangeset.h"
#include "sqliteexception.h"
#include "sqlitesessions.h"

#include <utils/smallstringio.h>

#include <sqlite3ext.h>

namespace Sqlite {

namespace {
void checkSessionChangeSetCreation(int resultCode)
{
    switch (resultCode) {
    case SQLITE_NOMEM:
        throw std::bad_alloc();
    }

    if (resultCode != SQLITE_OK)
        throw UnknowError("Unknow exception");
}

void checkIteratorCreation(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throw Sqlite::CannotCreateChangeSetIterator{
            "SessionChangeSet: Cannot create iterator from blob."};
}

void checkIteratorOperation(int resultCode)
{
    if (resultCode != SQLITE_OK)
        throw Sqlite::CannotGetChangeSetOperation{
            "SessionChangeSet: Cannot create iterator from blob."};
}

void checkChangeSetValue(int resultCode)
{
    switch (resultCode) {
    case SQLITE_OK:
        return;
    case SQLITE_RANGE:
        throw Sqlite::ChangeSetTupleIsOutOfRange{
            "SessionChangeSet: You tried to access a non existing column."};
    case SQLITE_MISUSE:
        throw Sqlite::ChangeSetIsMisused{
            "SessionChangeSet: Some misuse happened as you tried to access."};
    }

    throw Sqlite::UnknownError{"SessionChangeSet: Some unknown error happened."};
}

ValueView convertSqliteValue(sqlite3_value *value)
{
    if (value) {
        int type = sqlite3_value_type(value);
        switch (type) {
        case SQLITE_INTEGER:
            return ValueView::create(sqlite3_value_int64(value));
        case SQLITE_FLOAT:
            return ValueView::create(sqlite3_value_double(value));
        case SQLITE_TEXT:
            return ValueView::create(
                Utils::SmallStringView{reinterpret_cast<const char *>(sqlite3_value_text(value)),
                                       static_cast<std::size_t>(sqlite3_value_bytes(value))});
        case SQLITE_NULL:
            return ValueView::create(NullValue{});
        }
    }

    return ValueView::create(NullValue{});
}

} // namespace

SessionChangeSet::SessionChangeSet(BlobView blob)
    : m_data(sqlite3_malloc64(blob.size()))
    , m_size(int(blob.size()))
{
    std::memcpy(m_data, blob.data(), blob.size());
}

SessionChangeSet::SessionChangeSet(Sessions &session)
{
    int resultCode = sqlite3session_changeset(session.session.get(), &m_size, &m_data);
    checkSessionChangeSetCreation(resultCode);
}

SessionChangeSet::~SessionChangeSet()
{
    sqlite3_free(m_data);
}

BlobView SessionChangeSet::asBlobView() const
{
    return {static_cast<const byte *>(m_data), static_cast<std::size_t>(m_size)};
}

SessionChangeSetInternal::ConstIterator SessionChangeSet::begin() const
{
    sqlite3_changeset_iter *sessionIterator;
    int resultCode = sqlite3changeset_start(&sessionIterator, m_size, m_data);

    checkIteratorCreation(resultCode);

    SessionChangeSetInternal::ConstIterator iterator{sessionIterator};

    ++iterator;

    return iterator;
}
namespace SessionChangeSetInternal {
ConstIterator::~ConstIterator()
{
    sqlite3changeset_finalize(m_sessionIterator);
}

namespace {
State convertState(int state)
{
    switch (state) {
    case SQLITE_ROW:
        return State::Row;
    case SQLITE_DONE:
        return State::Done;
    }

    return State::Invalid;
}

Operation convertOperation(int operation)
{
    switch (operation) {
    case SQLITE_INSERT:
        return Operation::Insert;
    case SQLITE_UPDATE:
        return Operation::Update;
    case SQLITE_DELETE:
        return Operation::Delete;
    }

    return Operation::Invalid;
}
} // namespace

ConstIterator &ConstIterator::operator++()
{
    int state = sqlite3changeset_next(m_sessionIterator);

    m_state = convertState(state);

    return *this;
}

Tuple ConstIterator::operator*() const
{
    const char *table;
    int columnCount;
    int operation;
    int isIndirect;

    int resultCode = sqlite3changeset_op(m_sessionIterator, &table, &columnCount, &operation, &isIndirect);

    checkIteratorOperation(resultCode);

    return {table, m_sessionIterator, columnCount, convertOperation(operation)};
}

namespace {
ValueViews fetchValues(sqlite3_changeset_iter *sessionIterator, int column, Operation operation)
{
    sqlite3_value *newValue = nullptr;

    if (operation == Operation::Insert || operation == Operation::Update) {
        int resultCode = sqlite3changeset_new(sessionIterator, column, &newValue);

        checkChangeSetValue(resultCode);
    }

    sqlite3_value *oldValue = nullptr;
    if (operation == Operation::Delete || operation == Operation::Update) {
        int resultCode = sqlite3changeset_old(sessionIterator, column, &oldValue);

        checkChangeSetValue(resultCode);
    }

    return {convertSqliteValue(newValue), convertSqliteValue(oldValue)};
}
} // namespace

ValueViews ConstTupleIterator::operator*() const
{
    return fetchValues(m_sessionIterator, m_column, m_operation);
}

ValueViews Tuple::operator[](int column) const
{
    return fetchValues(sessionIterator, column, operation);
}

} // namespace SessionChangeSetInternal
} // namespace Sqlite
