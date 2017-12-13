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

#include "ipcclientinterface.h"

namespace ClangBackEnd {

class ClangCodeModelServerInterface;

class CodeCompletedMessage;
class CompleteCodeMessage;
class DocumentAnnotationsChangedMessage;
class EchoMessage;
class ProjectPartsDoNotExistMessage;
class ReferencesMessage;
class RegisterProjectPartsForEditorMessage;
class RegisterTranslationUnitForEditorMessage;
class RegisterUnsavedFilesForEditorMessage;
class RequestDocumentAnnotationsMessage;
class RequestReferencesMessage;
class TranslationUnitDoesNotExistMessage;
class UnregisterProjectPartsForEditorMessage;
class UnregisterTranslationUnitsForEditorMessage;
class UnregisterUnsavedFilesForEditorMessage;
class UpdateTranslationUnitsForEditorMessage;
class UpdateVisibleTranslationUnitsMessage;

class CMBIPC_EXPORT ClangCodeModelClientInterface : public IpcClientInterface
{
public:
    void dispatch(const MessageEnvelop &messageEnvelop) override;

    virtual void alive() = 0;
    virtual void echo(const EchoMessage &message) = 0;
    virtual void codeCompleted(const CodeCompletedMessage &message) = 0;
    virtual void translationUnitDoesNotExist(const TranslationUnitDoesNotExistMessage &message) = 0;
    virtual void projectPartsDoNotExist(const ProjectPartsDoNotExistMessage &message) = 0;
    virtual void documentAnnotationsChanged(const DocumentAnnotationsChangedMessage &message) = 0;
    virtual void references(const ReferencesMessage &message) = 0;
};

} // namespace ClangBackEnd
