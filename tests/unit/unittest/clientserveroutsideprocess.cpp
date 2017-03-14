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

#include "googletest.h"

#include "mockclangcodemodelclient.h"

#include <cmbalivemessage.h>
#include <cmbcodecompletedmessage.h>
#include <cmbcompletecodemessage.h>
#include <cmbechomessage.h>
#include <cmbendmessage.h>
#include <cmbregisterprojectsforeditormessage.h>
#include <cmbregistertranslationunitsforeditormessage.h>
#include <cmbunregisterprojectsforeditormessage.h>
#include <cmbunregistertranslationunitsforeditormessage.h>
#include <documentannotationschangedmessage.h>
#include <clangcodemodelconnectionclient.h>
#include <projectpartsdonotexistmessage.h>
#include <readmessageblock.h>
#include <translationunitdoesnotexistmessage.h>
#include <writemessageblock.h>

#include <utils/hostosinfo.h>

#include <QBuffer>
#include <QProcess>
#include <QSignalSpy>
#include <QString>
#include <QVariant>

#include <vector>

using namespace ClangBackEnd;

using ::testing::Eq;
using ::testing::SizeIs;

class ClientServerOutsideProcess : public ::testing::Test
{
protected:
    void SetUp();
    void TearDown();

    static void SetUpTestCase();
    static void TearDownTestCase();

    static MockClangCodeModelClient mockClangCodeModelClient;
    static ClangBackEnd::ClangCodeModelConnectionClient client;
};

MockClangCodeModelClient ClientServerOutsideProcess::mockClangCodeModelClient;
ClangBackEnd::ClangCodeModelConnectionClient ClientServerOutsideProcess::client(&ClientServerOutsideProcess::mockClangCodeModelClient);

TEST_F(ClientServerOutsideProcess, RestartProcessAsynchronously)
{
    QSignalSpy clientSpy(&client, &ConnectionClient::connectedToLocalSocket);

    client.restartProcessAsynchronously();

    ASSERT_TRUE(clientSpy.wait(100000));
    ASSERT_TRUE(client.isProcessIsRunning());
    ASSERT_TRUE(client.isConnected());
}

TEST_F(ClientServerOutsideProcess, RestartProcessAfterAliveTimeout)
{
    QSignalSpy clientSpy(&client, &ConnectionClient::connectedToLocalSocket);

    client.setProcessAliveTimerInterval(1);

    ASSERT_TRUE(clientSpy.wait(100000));
    ASSERT_THAT(clientSpy, SizeIs(1));
}

TEST_F(ClientServerOutsideProcess, RestartProcessAfterTermination)
{
    QSignalSpy clientSpy(&client, &ConnectionClient::connectedToLocalSocket);

    client.processForTestOnly()->kill();

    ASSERT_TRUE(clientSpy.wait(100000));
    ASSERT_THAT(clientSpy, SizeIs(1));
}

TEST_F(ClientServerOutsideProcess, SendRegisterTranslationUnitForEditorMessage)
{
    auto filePath = Utf8StringLiteral("foo.cpp");
    ClangBackEnd::FileContainer fileContainer(filePath, Utf8StringLiteral("projectId"));
    ClangBackEnd::RegisterTranslationUnitForEditorMessage registerTranslationUnitForEditorMessage({fileContainer},
                                                                                                  filePath,
                                                                                                  {filePath});
    EchoMessage echoMessage(registerTranslationUnitForEditorMessage);

    EXPECT_CALL(mockClangCodeModelClient, echo(echoMessage))
            .Times(1);

    client.serverProxy().registerTranslationUnitsForEditor(registerTranslationUnitForEditorMessage);
    ASSERT_TRUE(client.waitForEcho());
}

TEST_F(ClientServerOutsideProcess, SendUnregisterTranslationUnitsForEditorMessage)
{
    FileContainer fileContainer(Utf8StringLiteral("foo.cpp"), Utf8StringLiteral("projectId"));
    ClangBackEnd::UnregisterTranslationUnitsForEditorMessage unregisterTranslationUnitsForEditorMessage ({fileContainer});
    EchoMessage echoMessage(unregisterTranslationUnitsForEditorMessage);

    EXPECT_CALL(mockClangCodeModelClient, echo(echoMessage))
            .Times(1);

    client.serverProxy().unregisterTranslationUnitsForEditor(unregisterTranslationUnitsForEditorMessage);
    ASSERT_TRUE(client.waitForEcho());
}

TEST_F(ClientServerOutsideProcess, SendCompleteCodeMessage)
{
    CompleteCodeMessage codeCompleteMessage(Utf8StringLiteral("foo.cpp"), 24, 33, Utf8StringLiteral("do what I want"));
    EchoMessage echoMessage(codeCompleteMessage);

    EXPECT_CALL(mockClangCodeModelClient, echo(echoMessage))
            .Times(1);

    client.serverProxy().completeCode(codeCompleteMessage);
    ASSERT_TRUE(client.waitForEcho());
}

TEST_F(ClientServerOutsideProcess, SendRegisterProjectPartsForEditorMessage)
{
    ClangBackEnd::ProjectPartContainer projectContainer(Utf8StringLiteral(TESTDATA_DIR"/complete.pro"));
    ClangBackEnd::RegisterProjectPartsForEditorMessage registerProjectPartsForEditorMessage({projectContainer});
    EchoMessage echoMessage(registerProjectPartsForEditorMessage);

    EXPECT_CALL(mockClangCodeModelClient, echo(echoMessage))
            .Times(1);

    client.serverProxy().registerProjectPartsForEditor(registerProjectPartsForEditorMessage);
    ASSERT_TRUE(client.waitForEcho());
}

TEST_F(ClientServerOutsideProcess, SendUnregisterProjectPartsForEditorMessage)
{
    ClangBackEnd::UnregisterProjectPartsForEditorMessage unregisterProjectPartsForEditorMessage({Utf8StringLiteral(TESTDATA_DIR"/complete.pro")});
    EchoMessage echoMessage(unregisterProjectPartsForEditorMessage);

    EXPECT_CALL(mockClangCodeModelClient, echo(echoMessage))
            .Times(1);

    client.serverProxy().unregisterProjectPartsForEditor(unregisterProjectPartsForEditorMessage);
    ASSERT_TRUE(client.waitForEcho());
}

void ClientServerOutsideProcess::SetUpTestCase()
{
    QSignalSpy clientSpy(&client, &ConnectionClient::connectedToLocalSocket);
    client.setProcessPath(Utils::HostOsInfo::withExecutableSuffix(QStringLiteral(ECHOSERVER)));

    client.startProcessAndConnectToServerAsynchronously();

    ASSERT_TRUE(clientSpy.wait(100000));
    ASSERT_THAT(clientSpy, SizeIs(1));
}

void ClientServerOutsideProcess::TearDownTestCase()
{
    client.finishProcess();
}
void ClientServerOutsideProcess::SetUp()
{
    client.setProcessAliveTimerInterval(1000000);

    ASSERT_TRUE(client.isConnected());
}

void ClientServerOutsideProcess::TearDown()
{
    client.setProcessAliveTimerInterval(1000000);
    client.waitForConnected();

    ASSERT_TRUE(client.isProcessIsRunning());
    ASSERT_TRUE(client.isConnected());
}
