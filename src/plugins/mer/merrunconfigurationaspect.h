/****************************************************************************
**
** Copyright (C) 2016 Jolla Ltd.
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

#ifndef MERRUNCONFIGURATIONASPECT_H
#define MERRUNCONFIGURATIONASPECT_H

#include <projectexplorer/runconfiguration.h>

namespace Mer {
namespace Internal {

class MerRunConfigurationAspect : public ProjectExplorer::IRunConfigurationAspect
{
    Q_OBJECT

public:
    enum QmlLiveOption {
        NoQmlLiveOption = 0x0,
        UpdateOnConnect = 0x1,
        UpdatesAsOverlay = 0x2,
        LoadDummyData = 0x4,
        AllowCreateMissing = 0x8
    };
    Q_DECLARE_FLAGS(QmlLiveOptions, QmlLiveOption)
    Q_FLAG(QmlLiveOptions)

public:
    MerRunConfigurationAspect(ProjectExplorer::Target *target);

    bool isQmlLiveEnabled() const { return m_qmlLiveEnabled; }
    int qmlLiveIpcPort() const { return m_qmlLiveIpcPort; }
    QString qmlLiveBenchWorkspace() const { return m_qmlLiveBenchWorkspace; }
    QString qmlLiveTargetWorkspace() const { return m_qmlLiveTargetWorkspace; }
    QmlLiveOptions qmlLiveOptions() const { return m_qmlLiveOptions; }

    QString defaultQmlLiveBenchWorkspace() const;

    void applyTo(ProjectExplorer::Runnable *r) const;

    void fromMap(const QVariantMap &map) override;
    void toMap(QVariantMap &map) const override;

public slots:
    void restoreQmlLiveDefaults();
    void setQmlLiveEnabled(bool qmlLiveEnabled);
    void setQmlLiveIpcPort(int port);
    void setQmlLiveBenchWorkspace(const QString &benchWorkspace);
    void setQmlLiveTargetWorkspace(const QString &targetWorkspace);
    void setQmlLiveOptions(QmlLiveOptions options);

signals:
    void qmlLiveEnabledChanged(bool qmlLiveEnabled);
    void qmlLiveIpcPortChanged(int port);
    void qmlLiveBenchWorkspaceChanged(const QString &benchWorkspace);
    void qmlLiveTargetWorkspaceChanged(const QString &targetWorkspace);
    void qmlLiveOptionsChanged();

private:
    ProjectExplorer::Target *m_target;
    bool m_qmlLiveEnabled;
    int m_qmlLiveIpcPort;
    QString m_qmlLiveBenchWorkspace;
    QString m_qmlLiveTargetWorkspace;
    QmlLiveOptions m_qmlLiveOptions;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MerRunConfigurationAspect::QmlLiveOptions)

} // namespace Internal
} // namespace Mer

#endif // MERRUNCONFIGURATIONASPECT_H
