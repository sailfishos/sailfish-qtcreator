/****************************************************************************
**
** Copyright (C) 2012 - 2013 Jolla Ltd.
** Contact: http://jolla.com/
**
** This file is part of Qt Creator.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Digia.
**
****************************************************************************/

#include <QProcessEnvironment>
#include <QProcess>
#include <QtTest>

bool waitForSignal(QObject *receiver, const char *member, int timeout = 3000)
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    QObject::connect(receiver, member, &loop, SLOT(quit()));
    timer.start(timeout);
    loop.exec();
    if (!timer.isActive())
        qCritical("waitForSignal %s timed out after %d ms", member, timeout);
    return timer.isActive();
}

class tst_MerSsh : public QObject
{
    Q_OBJECT

public:
    tst_MerSsh();

public slots:
    void processAppOutput();

signals:
    void outputReceived();

private slots:
    void init();
    void cleanup();
    void runWithNoParams();
    void runWithOnlyOptions();
    void runWithInvalidOptions();
    void runQmake();
    void runGcc();

private:
    QString m_mersshPath;
    QProcess *m_process;
    QByteArray m_expectedOutput;
};

tst_MerSsh::tst_MerSsh()
    : m_process(0)
{
    const QString testAppDir =  qApp->applicationDirPath();
    m_mersshPath =
#if defined(Q_OS_MAC)
            QString::fromLatin1("%1/../../../bin/Qt Creator.app/Contents/Resources/merssh").arg(testAppDir);
#elif defined(Q_OS_WIN)
            QString::fromLatin1("%1/../../../bin/merssh.exe").arg(testAppDir);
#else
            QString::fromLatin1("%1/../../../bin/merssh").arg(testAppDir);
#endif
}

void tst_MerSsh::processAppOutput()
{
    m_expectedOutput = m_process->readAll();
    emit outputReceived();
}

void tst_MerSsh::init()
{
    m_process = new QProcess(this);
    m_process->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_process, SIGNAL(readyReadStandardOutput()), this, SLOT(processAppOutput()));
    m_process->setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    m_expectedOutput.clear();
}

void tst_MerSsh::cleanup()
{
    m_process->kill();
    m_process->waitForFinished();
    delete m_process;
    m_process = 0;
}

void tst_MerSsh::runWithNoParams()
{
    m_process->start(m_mersshPath);
    if (!m_process->waitForStarted())
        qDebug() << "Error launching " + m_mersshPath.toUtf8() + " : " + m_process->errorString().toUtf8();
    QVERIFY(waitForSignal(this, SIGNAL(outputReceived())));
    QVERIFY(m_expectedOutput.contains("merssh usage:"));
}

void tst_MerSsh::runWithOnlyOptions()
{
    QFile file("data/options.txt");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QStringList arguments;
    arguments << QString::fromLatin1(file.readAll()).split(QChar::fromLatin1('\n'));
    m_process->start(m_mersshPath, arguments);
    if (!m_process->waitForStarted())
        qDebug() << "Error launching " + m_mersshPath.toUtf8() + " : " + m_process->errorString().toUtf8();
    QVERIFY(waitForSignal(this, SIGNAL(outputReceived())));
    QVERIFY(m_expectedOutput.contains("Cannot open file"));
}

void tst_MerSsh::runWithInvalidOptions()
{
    QFile file("data/invalidoptions.txt");
    QVERIFY(file.open(QIODevice::ReadOnly));
    QStringList arguments;
    arguments << QString::fromLatin1(file.readAll()).split(QChar::fromLatin1('\n'));
    m_process->start(m_mersshPath, arguments);
    if (!m_process->waitForStarted())
        qDebug() << "Error launching " + m_mersshPath.toUtf8() + " : " + m_process->errorString().toUtf8();
    QVERIFY(waitForSignal(this, SIGNAL(outputReceived())));
    QVERIFY(m_expectedOutput.contains("Cannot open file"));
}

void tst_MerSsh::runQmake()
{

    m_process->start(m_mersshPath);
    if (!m_process->waitForStarted())
        qDebug() << "Error launching " + m_mersshPath.toUtf8() + " : " + m_process->errorString().toUtf8();
    QVERIFY(waitForSignal(this, SIGNAL(outputReceived())));
    QVERIFY(m_expectedOutput.contains("merssh usage:"));
}

void tst_MerSsh::runGcc()
{
    m_process->start(m_mersshPath);
    if (!m_process->waitForStarted())
        qDebug() << "Error launching " + m_mersshPath.toUtf8() + " : " + m_process->errorString().toUtf8();
    QVERIFY(waitForSignal(this, SIGNAL(outputReceived())));
    QVERIFY(m_expectedOutput.contains("merssh usage:"));
}

QTEST_MAIN(tst_MerSsh)

#include "tst_merssh.moc"
