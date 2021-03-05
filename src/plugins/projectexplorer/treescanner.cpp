/****************************************************************************
**
** Copyright (C) 2016 Alexander Drozdov.
** Contact: Alexander Drozdov (adrozdoff@gmail.com)
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

#include "treescanner.h"
#include "projectexplorerconstants.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <cpptools/cpptoolsconstants.h>

#include <utils/qtcassert.h>
#include <utils/algorithm.h>
#include <utils/runextensions.h>

#include <memory>

namespace ProjectExplorer {

TreeScanner::TreeScanner(QObject *parent) : QObject(parent)
{
    m_factory = TreeScanner::genericFileType;
    m_filter = [](const Utils::MimeType &mimeType, const Utils::FilePath &fn) {
        return isWellKnownBinary(mimeType, fn) && isMimeBinary(mimeType, fn);
    };

    connect(&m_futureWatcher, &FutureWatcher::finished, this, &TreeScanner::finished);
}

TreeScanner::~TreeScanner()
{
    disconnect(&m_futureWatcher, nullptr, nullptr, nullptr); // Do not trigger signals anymore!

    if (!m_futureWatcher.isFinished()) {
        m_futureWatcher.cancel();
        m_futureWatcher.waitForFinished();
    }
}

bool TreeScanner::asyncScanForFiles(const Utils::FilePath &directory)
{
    if (!m_futureWatcher.isFinished())
        return false;

    m_scanFuture = Utils::runAsync([this, directory](FutureInterface &fi) {
        TreeScanner::scanForFiles(fi, directory, m_filter, m_factory);
    });
    m_futureWatcher.setFuture(m_scanFuture);

    return true;
}

void TreeScanner::setFilter(TreeScanner::FileFilter filter)
{
    if (isFinished())
        m_filter = filter;
}

void TreeScanner::setTypeFactory(TreeScanner::FileTypeFactory factory)
{
    if (isFinished())
        m_factory = factory;
}

TreeScanner::Future TreeScanner::future() const
{
    return m_scanFuture;
}

bool TreeScanner::isFinished() const
{
    return m_futureWatcher.isFinished();
}

TreeScanner::Result TreeScanner::result() const
{
    if (isFinished())
        return m_scanFuture.result();
    return Result();
}

TreeScanner::Result TreeScanner::release()
{
    if (isFinished() && m_scanFuture.resultCount() > 0) {
        auto result = m_scanFuture.result();
        m_scanFuture = Future();
        return result;
    }
    m_scanFuture = Future();
    return Result();
}

void TreeScanner::reset()
{
    if (isFinished())
        m_scanFuture = Future();
}

bool TreeScanner::isWellKnownBinary(const Utils::MimeType & /*mdb*/, const Utils::FilePath &fn)
{
    return fn.endsWith(QLatin1String(".a")) ||
            fn.endsWith(QLatin1String(".o")) ||
            fn.endsWith(QLatin1String(".d")) ||
            fn.endsWith(QLatin1String(".exe")) ||
            fn.endsWith(QLatin1String(".dll")) ||
            fn.endsWith(QLatin1String(".obj")) ||
            fn.endsWith(QLatin1String(".elf"));
}

bool TreeScanner::isMimeBinary(const Utils::MimeType &mimeType, const Utils::FilePath &/*fn*/)
{
    bool isBinary = false;
    if (mimeType.isValid()) {
        QStringList mimes;
        mimes << mimeType.name() << mimeType.allAncestors();
        isBinary = !mimes.contains(QLatin1String("text/plain"));
    }
    return isBinary;
}

FileType TreeScanner::genericFileType(const Utils::MimeType &mimeType, const Utils::FilePath &/*fn*/)
{
    return Node::fileTypeForMimeType(mimeType);
}

void TreeScanner::scanForFiles(FutureInterface &fi, const Utils::FilePath& directory,
                               const FileFilter &filter, const FileTypeFactory &factory)
{
    Result nodes = FileNode::scanForFiles(fi, directory,
                [&filter, &factory](const Utils::FilePath &fn) -> FileNode * {
        const Utils::MimeType mimeType = Utils::mimeTypeForFile(fn.toString());

        // Skip some files during scan.
        if (filter && filter(mimeType, fn))
            return nullptr;

        // Type detection
        FileType type = FileType::Unknown;
        if (factory)
            type = factory(mimeType, fn);

        return new FileNode(fn, type);
    });

    Utils::sort(nodes, ProjectExplorer::Node::sortByPath);

    fi.setProgressValue(fi.progressMaximum());
    fi.reportResult(nodes);
}

} // namespace ProjectExplorer
