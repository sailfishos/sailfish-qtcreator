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

#include <utf8string.h>

#include <chrono>
#include <memory>
#include <vector>

class Utf8StringVector;

namespace ClangBackEnd {

class ProjectPartContainer;
class ProjectPartData;

using time_point = std::chrono::steady_clock::time_point;

class ProjectPart
{
public:
    ProjectPart(const Utf8String &projectPartId = Utf8String());
    ProjectPart(const Utf8String &projectPartId,
                std::initializer_list<Utf8String> arguments);
    ProjectPart(const ProjectPartContainer &projectContainer);
    ~ProjectPart();

    ProjectPart(const ProjectPart &project);
    ProjectPart &operator=(const ProjectPart &project);

    ProjectPart(ProjectPart &&project);
    ProjectPart &operator=(ProjectPart &&project);

    void clear();

    const Utf8String &projectPartId() const;

    void setArguments(const Utf8StringVector &arguments_);

    const std::vector<const char*> &arguments() const;

    int argumentCount() const;
    const char *const *cxArguments() const;

    const time_point &lastChangeTimePoint() const;

private:
    void updateLastChangeTimePoint();

private:
    std::shared_ptr<ProjectPartData> d;
};

bool operator==(const ProjectPart &first, const ProjectPart &second);

} // namespace ClangBackEnd
