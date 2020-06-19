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

#include "clangpchmanager_global.h"

#include <pchmanagerclientinterface.h>
#include <projectpartid.h>

#include <vector>

namespace ClangPchManager {
class PchManagerConnectionClient;
class ProgressManagerInterface;
class PchManagerNotifierInterface;

class CLANGPCHMANAGER_EXPORT PchManagerClient final : public ClangBackEnd::PchManagerClientInterface
{
    friend class PchManagerNotifierInterface;
public:
    PchManagerClient(ProgressManagerInterface &pchCreationProgressManager,
                     ProgressManagerInterface &dependencyCreationProgressManager)
        : m_pchCreationProgressManager(pchCreationProgressManager)
        , m_dependencyCreationProgressManager(dependencyCreationProgressManager)
    {}

    void alive() override;
    void precompiledHeadersUpdated(ClangBackEnd::PrecompiledHeadersUpdatedMessage &&message) override;
    void progress(ClangBackEnd::ProgressMessage &&message) override;

    void precompiledHeaderRemoved(ClangBackEnd::ProjectPartId projectPartId);

    void setConnectionClient(PchManagerConnectionClient *connectionClient);

    unittest_public : const std::vector<PchManagerNotifierInterface *> &notifiers() const;
    void precompiledHeaderUpdated(ClangBackEnd::ProjectPartId projectPartId);

    void attach(PchManagerNotifierInterface *notifier);
    void detach(PchManagerNotifierInterface *notifier);

private:
    std::vector<PchManagerNotifierInterface*> m_notifiers;
    PchManagerConnectionClient *m_connectionClient=nullptr;
    ProgressManagerInterface &m_pchCreationProgressManager;
    ProgressManagerInterface &m_dependencyCreationProgressManager;
};

} // namespace ClangPchManager
