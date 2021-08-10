/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#include <string.h>

#if __has_include(<unistd.h>)
#include <unistd.h>
#endif

#if __has_include(<utime.h>)
#define HAVE_UTIME 1
#endif

#if (_XOPEN_SOURCE >= 500) && !(_POSIX_C_SOURCE >= 200809L) || _DEFAULT_SOURCE || _BSD_SOURCE
#define HAVE_USLEEP 1
#endif

#ifdef _GNU_SOURCE
#define HAVE_STRCHRNUL 1
#endif

#if _POSIX_C_SOURCE >= 199309L || _XOPEN_SOURCE >= 500
#define HAVE_FDATASYNC 1
#endif

#if _POSIX_C_SOURCE >= 1 || _BSD_SOURCE
#define HAVE_LOCALTIME_R 1
#else
#define HAVE_LOCALTIME_S 1
#endif

#define HAVE_MALLOC_USABLE_SIZE 1
#define HAVE_ISNAN 1

#define SQLITE_THREADSAFE 2
#define SQLITE_ENABLE_FTS5 1
#define SQLITE_ENABLE_UNLOCK_NOTIFY 1
#define SQLITE_ENABLE_JSON1 1
#define SQLITE_DEFAULT_FOREIGN_KEYS 1
#define SQLITE_TEMP_STORE 2
#define SQLITE_DEFAULT_WAL_SYNCHRONOUS 1
#define SQLITE_MAX_WORKER_THREADS 1
#define SQLITE_DEFAULT_MEMSTATUS 0
#define SQLITE_OMIT_DEPRECATED 1
#define SQLITE_OMIT_DECLTYPE 1
#define SQLITE_MAX_EXPR_DEPTH 0
#define SQLITE_OMIT_SHARED_CACHE 1
#define SQLITE_USE_ALLOCA 1
#define SQLITE_ENABLE_MEMORY_MANAGEMENT 1
#define SQLITE_ENABLE_NULL_TRIM 1
#define SQLITE_OMIT_EXPLAIN 1
#define SQLITE_OMIT_LOAD_EXTENSION 1
#define SQLITE_OMIT_UTF16 1
#define SQLITE_DQS 0
#define SQLITE_ENABLE_STAT4 1
#define SQLITE_DEFAULT_MMAP_SIZE 268435456
#define SQLITE_ENABLE_SESSION 1
#define SQLITE_ENABLE_PREUPDATE_HOOK 1
#define SQLITE_LIKE_DOESNT_MATCH_BLOBS 1
#define SQLITE_OMIT_AUTOINIT 1
#define SQLITE_DEFAULT_CACHE_SIZE -100000
