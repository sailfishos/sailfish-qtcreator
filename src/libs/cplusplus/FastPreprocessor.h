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

#include "PreprocessorClient.h"
#include "CppDocument.h"
#include "pp.h"

#include <cplusplus/Control.h>

#include <QSet>
#include <QString>

namespace CPlusPlus {

class CPLUSPLUS_EXPORT FastPreprocessor: public Client
{
    Environment _env;
    Snapshot _snapshot;
    Preprocessor _preproc;
    QSet<QString> _merged;
    Document::Ptr _currentDoc;
    bool _addIncludesToCurrentDoc;

    void mergeEnvironment(const QString &fileName);

public:
    FastPreprocessor(const Snapshot &snapshot);

    QByteArray run(Document::Ptr newDoc,
                   const QByteArray &source,
                   bool mergeDefinedMacrosOfDocument = false);

    // CPlusPlus::Client
    virtual void sourceNeeded(int line, const QString &fileName, IncludeType mode,
                              const QStringList &initialIncludes = QStringList());

    virtual void macroAdded(const Macro &);

    virtual void passedMacroDefinitionCheck(int, int, int, const Macro &);
    virtual void failedMacroDefinitionCheck(int, int, const ByteArrayRef &);

    virtual void notifyMacroReference(int, int, int, const Macro &);

    virtual void startExpandingMacro(int,
                                     int,
                                     int,
                                     const Macro &,
                                     const QVector<MacroArgumentReference> &);
    virtual void stopExpandingMacro(int, const Macro &) {}
    virtual void markAsIncludeGuard(const QByteArray &macroName);

    virtual void startSkippingBlocks(int) {}
    virtual void stopSkippingBlocks(int) {}
};

} // namespace CPlusPlus
