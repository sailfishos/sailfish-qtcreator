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

#include <QCoreApplication>
#include <QDebug>
#include <QProcess>
#include <QtTest>

class SdktoolTest : public QObject
{
    Q_OBJECT

private slots:
    void testSdktool();
};

void SdktoolTest::testSdktool()
{
    QDir rootDir(SDKTOOL_DIR);
    QProcess process;
    process.start(rootDir.absoluteFilePath(QLatin1String("sdktool")),
                  QStringList() << QLatin1String("-test"));
    process.waitForFinished();
    QByteArray output = process.readAllStandardError();
    for (auto line : output.split('\n'))
        qDebug() << line;
    QCOMPARE(process.exitCode(), 0);
}

QTEST_MAIN(SdktoolTest)

#include "tst_sdktool.moc"
