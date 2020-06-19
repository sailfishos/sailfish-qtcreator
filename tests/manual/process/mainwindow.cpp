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

#include "mainwindow.h"

#include <utils/fileutils.h>
#include <utils/synchronousprocess.h>

#include <QPlainTextEdit>
#include <QApplication>
#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    m_logWindow(new QPlainTextEdit)
{
    setCentralWidget(m_logWindow);
    QTimer::singleShot(200, this, &MainWindow::test);
}

void MainWindow::append(const QString &s)
{
    m_logWindow->appendPlainText(s);
}

void MainWindow::test()
{
    QStringList args = QApplication::arguments();
    args.pop_front();
    const QString cmd = args.front();
    args.pop_front();
    Utils::SynchronousProcess process;
    process.setTimeoutS(2);
    qDebug() << "Async: " << cmd << args;
    connect(&process, &Utils::SynchronousProcess::stdOutBuffered, this, &MainWindow::append);
    connect(&process, &Utils::SynchronousProcess::stdErrBuffered, this, &MainWindow::append);
    const Utils::SynchronousProcessResponse resp = process.run({Utils::FilePath::fromString(cmd), args});
    qDebug() << resp;
}
