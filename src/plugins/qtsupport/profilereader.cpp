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

#include "profilereader.h"

#include <coreplugin/icore.h>
#include <projectexplorer/taskhub.h>

#include <QCoreApplication>
#include <QDebug>

using namespace ProjectExplorer;
using namespace QtSupport;

static QString format(const QString &fileName, int lineNo, const QString &msg)
{
    if (lineNo > 0)
        return QString::fromLatin1("%1(%2): %3").arg(fileName, QString::number(lineNo), msg);
    else if (!fileName.isEmpty())
        return QString::fromLatin1("%1: %2").arg(fileName, msg);
    else
        return msg;
}

ProMessageHandler::ProMessageHandler(bool verbose, bool exact)
    : m_verbose(verbose)
    , m_exact(exact)
    //: Prefix used for output from the cumulative evaluation of project files.
    , m_prefix(QCoreApplication::translate("ProMessageHandler", "[Inexact] "))
{
}

ProMessageHandler::~ProMessageHandler()
{
    if (!m_messages.isEmpty())
        Core::MessageManager::writeFlashing(m_messages);
}



void ProMessageHandler::message(int type, const QString &msg, const QString &fileName, int lineNo)
{
    if ((type & CategoryMask) == ErrorMessage && ((type & SourceMask) == SourceParser || m_verbose)) {
        // parse error in qmake files
        if (m_exact) {
            TaskHub::addTask(
                BuildSystemTask(Task::Error, msg, Utils::FilePath::fromString(fileName), lineNo));
        } else {
            appendMessage(format(fileName, lineNo, msg));
        }
    }
}

void ProMessageHandler::fileMessage(int type, const QString &msg)
{
    // message(), warning() or error() calls in qmake files
    if (!m_verbose)
        return;
    if (m_exact && type == QMakeHandler::ErrorMessage)
        TaskHub::addTask(BuildSystemTask(Task::Error, msg));
    else if (m_exact && type == QMakeHandler::WarningMessage)
        TaskHub::addTask(BuildSystemTask(Task::Warning, msg));
    else
        appendMessage(msg);
}

void ProMessageHandler::appendMessage(const QString &msg)
{
    m_messages << (m_exact ? msg : m_prefix + msg);
    if (msg.contains("Project ERROR: Unknown module(s) in QT:")) {
        m_messages << QCoreApplication::translate("ProMessageHandler",
                "  Use \"Build > Run qmake\" to get missing build time dependencies installed.\n"
                "  If it persists, check for missing \"BuildRequires\" in RPM .spec (or .yaml) file.");
    }
}

ProFileReader::ProFileReader(QMakeGlobals *option, QMakeVfs *vfs)
    : QMakeParser(ProFileCacheManager::instance()->cache(), vfs, this)
    , ProFileEvaluator(option, this, vfs, this)
    , m_ignoreLevel(0)
{
    setExtraConfigs(QStringList("qtc_run"));
}

ProFileReader::~ProFileReader()
{
    foreach (ProFile *pf, m_proFiles)
        pf->deref();
}

void ProFileReader::setCumulative(bool on)
{
    ProMessageHandler::setVerbose(!on);
    ProMessageHandler::setExact(!on);
    ProFileEvaluator::setCumulative(on);
}

void ProFileReader::aboutToEval(ProFile *parent, ProFile *pro, EvalFileType type)
{
    if (m_ignoreLevel || (type != EvalProjectFile && type != EvalIncludeFile)) {
        m_ignoreLevel++;
    } else if (parent) {  // Skip the actual .pro file, as nobody needs that.
        QVector<ProFile *> &children = m_includeFiles[parent];
        if (!children.contains(pro)) {
            children.append(pro);
            m_proFiles.append(pro);
            pro->ref();
        }
    }
}

void ProFileReader::doneWithEval(ProFile *)
{
    if (m_ignoreLevel)
        m_ignoreLevel--;
}

QHash<ProFile *, QVector<ProFile *> > ProFileReader::includeFiles() const
{
    return m_includeFiles;
}

ProFileCacheManager *ProFileCacheManager::s_instance = nullptr;

ProFileCacheManager::ProFileCacheManager(QObject *parent) :
    QObject(parent)
{
    s_instance = this;
    m_timer.setInterval(5000);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout,
            this, &ProFileCacheManager::clear);
}

void ProFileCacheManager::incRefCount()
{
    ++m_refCount;
    m_timer.stop();
}

void ProFileCacheManager::decRefCount()
{
    --m_refCount;
    if (!m_refCount)
        m_timer.start();
}

ProFileCacheManager::~ProFileCacheManager()
{
    s_instance = nullptr;
    clear();
}

ProFileCache *ProFileCacheManager::cache()
{
    if (!m_cache)
        m_cache = new ProFileCache;
    return m_cache;
}

void ProFileCacheManager::clear()
{
    Q_ASSERT(m_refCount == 0);
    // Just deleting the cache will be safe as long as the sequence of
    // obtaining a cache pointer and using it is atomic as far as the main
    // loop is concerned. Use a shared pointer once this is not true anymore.
    delete m_cache;
    m_cache = nullptr;
}

void ProFileCacheManager::discardFiles(const QString &prefix, QMakeVfs *vfs)
{
    if (m_cache)
        m_cache->discardFiles(prefix, vfs);
}

void ProFileCacheManager::discardFile(const QString &fileName, QMakeVfs *vfs)
{
    if (m_cache)
        m_cache->discardFile(fileName, vfs);
}
