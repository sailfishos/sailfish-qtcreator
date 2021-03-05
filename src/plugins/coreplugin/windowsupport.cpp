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

#include "windowsupport.h"

#include "actionmanager/actioncontainer.h"
#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "coreconstants.h"
#include "icore.h"

#include <app/app_version.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QAction>
#include <QEvent>
#include <QMenu>
#include <QWidget>
#include <QWindowStateChangeEvent>

using namespace Utils;

namespace Core {
namespace Internal {

Q_GLOBAL_STATIC(WindowList, m_windowList)

WindowSupport::WindowSupport(QWidget *window, const Context &context)
    : QObject(window),
      m_window(window)
{
    m_window->installEventFilter(this);

    m_contextObject = new IContext(this);
    m_contextObject->setWidget(window);
    m_contextObject->setContext(context);
    ICore::addContextObject(m_contextObject);

    if (useMacShortcuts) {
        m_minimizeAction = new QAction(this);
        ActionManager::registerAction(m_minimizeAction, Constants::MINIMIZE_WINDOW, context);
        connect(m_minimizeAction, &QAction::triggered, m_window, &QWidget::showMinimized);

        m_zoomAction = new QAction(this);
        ActionManager::registerAction(m_zoomAction, Constants::ZOOM_WINDOW, context);
        connect(m_zoomAction, &QAction::triggered, m_window, [this] {
            if (m_window->isMaximized()) {
                // similar to QWidget::showMaximized
                m_window->ensurePolished();
                m_window->setWindowState(m_window->windowState() & ~Qt::WindowMaximized);
                m_window->setVisible(true);
            } else {
                m_window->showMaximized();
            }
        });

        m_closeAction = new QAction(this);
        ActionManager::registerAction(m_closeAction, Constants::CLOSE_WINDOW, context);
        connect(m_closeAction, &QAction::triggered, m_window, &QWidget::close, Qt::QueuedConnection);
    }

    m_toggleFullScreenAction = new QAction(this);
    updateFullScreenAction();
    ActionManager::registerAction(m_toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN, context);
    connect(m_toggleFullScreenAction, &QAction::triggered, this, &WindowSupport::toggleFullScreen);

    m_windowList->addWindow(window);

    connect(ICore::instance(), &ICore::coreAboutToClose, this, [this]() { m_shutdown = true; });
}

WindowSupport::~WindowSupport()
{
    if (!m_shutdown) { // don't update all that stuff if we are shutting down anyhow
        if (useMacShortcuts) {
            ActionManager::unregisterAction(m_minimizeAction, Constants::MINIMIZE_WINDOW);
            ActionManager::unregisterAction(m_zoomAction, Constants::ZOOM_WINDOW);
            ActionManager::unregisterAction(m_closeAction, Constants::CLOSE_WINDOW);
        }
        ActionManager::unregisterAction(m_toggleFullScreenAction, Constants::TOGGLE_FULLSCREEN);
        m_windowList->removeWindow(m_window);
    }
}

void WindowSupport::setCloseActionEnabled(bool enabled)
{
    if (useMacShortcuts)
        m_closeAction->setEnabled(enabled);
}

bool WindowSupport::eventFilter(QObject *obj, QEvent *event)
{
    if (obj != m_window)
        return false;
    if (event->type() == QEvent::WindowStateChange) {
        if (Utils::HostOsInfo::isMacHost()) {
            bool minimized = m_window->isMinimized();
            m_minimizeAction->setEnabled(!minimized);
            m_zoomAction->setEnabled(!minimized);
        }
        m_previousWindowState = static_cast<QWindowStateChangeEvent *>(event)->oldState();
        updateFullScreenAction();
    } else if (event->type() == QEvent::WindowActivate) {
        m_windowList->setActiveWindow(m_window);
    } else if (event->type() == QEvent::Hide) {
        // minimized windows are hidden, but we still want to show them
        m_windowList->setWindowVisible(m_window, m_window->isMinimized());
    } else if (event->type() == QEvent::Show) {
        m_windowList->setWindowVisible(m_window, true);
    }
    return false;
}

void WindowSupport::toggleFullScreen()
{
    if (m_window->isFullScreen()) {
        m_window->setWindowState(m_previousWindowState & ~Qt::WindowFullScreen);
    } else {
        m_window->setWindowState(m_window->windowState() | Qt::WindowFullScreen);
    }
}

void WindowSupport::updateFullScreenAction()
{
    if (m_window->isFullScreen()) {
        if (Utils::HostOsInfo::isMacHost())
            m_toggleFullScreenAction->setText(tr("Exit Full Screen"));
        else
            m_toggleFullScreenAction->setChecked(true);
    } else {
        if (Utils::HostOsInfo::isMacHost())
            m_toggleFullScreenAction->setText(tr("Enter Full Screen"));
        else
            m_toggleFullScreenAction->setChecked(false);
    }
}

WindowList::~WindowList()
{
    qDeleteAll(m_windowActions);
}

void WindowList::addWindow(QWidget *window)
{
#ifdef Q_OS_OSX
    if (!m_dockMenu) {
        m_dockMenu = new QMenu;
        m_dockMenu->setAsDockMenu();
    }
#endif

    m_windows.append(window);
    Id id = Id("QtCreator.Window.").withSuffix(m_windows.size());
    m_windowActionIds.append(id);
    auto action = new QAction(window->windowTitle());
    m_windowActions.append(action);
    QObject::connect(action, &QAction::triggered, [action, this]() { activateWindow(action); });
    action->setCheckable(true);
    action->setChecked(false);
    Command *cmd = ActionManager::registerAction(action, id);
    cmd->setAttribute(Command::CA_UpdateText);
    ActionManager::actionContainer(Constants::M_WINDOW)->addAction(cmd, Constants::G_WINDOW_LIST);
    action->setVisible(window->isVisible() || window->isMinimized()); // minimized windows are hidden but should be shown
    QObject::connect(window, &QWidget::windowTitleChanged, [window, this]() { updateTitle(window); });
    if (m_dockMenu)
        m_dockMenu->addAction(action);
    if (window->isActiveWindow())
        setActiveWindow(window);
}

void WindowList::activateWindow(QAction *action)
{
    int index = m_windowActions.indexOf(action);
    QTC_ASSERT(index >= 0, return);
    QTC_ASSERT(index < m_windows.size(), return);
    ICore::raiseWindow(m_windows.at(index));
}

void WindowList::updateTitle(QWidget *window)
{
    int index = m_windows.indexOf(window);
    QTC_ASSERT(index >= 0, return);
    QTC_ASSERT(index < m_windowActions.size(), return);
    QString title = window->windowTitle();
    if (title.endsWith(QStringLiteral("- ") + Constants::IDE_DISPLAY_NAME))
        title.chop(12);
    m_windowActions.at(index)->setText(Utils::quoteAmpersands(title.trimmed()));
}

void WindowList::removeWindow(QWidget *window)
{
    // remove window from list,
    // remove last action from menu(s)
    // and update all action titles, starting with the index where the window was
    int index = m_windows.indexOf(window);
    QTC_ASSERT(index >= 0, return);

    ActionManager::unregisterAction(m_windowActions.last(), m_windowActionIds.last());
    delete m_windowActions.takeLast();
    m_windowActionIds.removeLast();

    m_windows.removeOne(window);

    for (int i = index; i < m_windows.size(); ++i)
        updateTitle(m_windows.at(i));
}

void WindowList::setActiveWindow(QWidget *window)
{
    for (int i = 0; i < m_windows.size(); ++i)
        m_windowActions.at(i)->setChecked(m_windows.at(i) == window);
}

void WindowList::setWindowVisible(QWidget *window, bool visible)
{
    int index = m_windows.indexOf(window);
    QTC_ASSERT(index >= 0, return);
    QTC_ASSERT(index < m_windowActions.size(), return);
    m_windowActions.at(index)->setVisible(visible);
}

} // Internal
} // Core
