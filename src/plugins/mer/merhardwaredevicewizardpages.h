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

#ifndef MERHARDWAREDEVICEWIZARDPAGES_H
#define MERHARDWAREDEVICEWIZARDPAGES_H

#include <QWizardPage>

namespace Mer {
namespace Internal {

namespace Ui {
    class MerHardwareDeviceWizardGeneralPage;
    class MerHardwareDeviceWizardKeyPage;
}

class MerHardwareDeviceWizardGeneralPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit MerHardwareDeviceWizardGeneralPage(QWidget *parent = 0);

    QString configName() const;
    QString hostName() const;
    QString userName() const;
    QString password() const;
    QString freePorts() const;
    int timeout() const;
    int sshPort() const;

    bool isComplete() const;

private:
    Ui::MerHardwareDeviceWizardGeneralPage *m_ui;
};

class MerHardwareDeviceWizardKeyPage : public QWizardPage
{
    Q_OBJECT
public:
    explicit MerHardwareDeviceWizardKeyPage(QWidget *parent = 0);
    void initializePage();
    QString publicKeyFilePath() const;
    QString privateKeyFilePath() const;
    bool isNewSshKeysRquired() const;
    QString sharedSshPath() const;
private slots:
    void handleSdkVmChanged(const QString &vmName);
    void handleTestConnectionClicked();

private:
    Ui::MerHardwareDeviceWizardKeyPage *m_ui;
};

}
}

#endif // MERHARDWAREDEVICEPAGES_H
