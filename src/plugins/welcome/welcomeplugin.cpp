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

#include "welcomeplugin.h"

#include <extensionsystem/pluginmanager.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/imode.h>
#include <coreplugin/iwelcomepage.h>
#include <coreplugin/iwizardfactory.h>
#include <coreplugin/modemanager.h>

#include <utils/algorithm.h>
#include <utils/icon.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/styledbar.h>

#include <utils/theme/theme.h>

#include <QVBoxLayout>
#include <QMessageBox>

#include <QDir>
#include <QQmlPropertyMap>
#include <QQuickImageProvider>
#include <QTimer>

#include <QtQuickWidgets/QQuickWidget>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>

enum { debug = 0 };

using namespace Core;
using namespace ExtensionSystem;
using namespace Utils;

static const char currentPageSettingsKeyC[] = "WelcomeTab";

namespace Welcome {
namespace Internal {

static QString applicationDirPath()
{
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    return FileUtils::normalizePathName(QCoreApplication::applicationDirPath());
}

static QString resourcePath()
{
    // normalize paths so QML doesn't freak out if it's wrongly capitalized on Windows
    return FileUtils::normalizePathName(ICore::resourcePath());
}

class WelcomeImageIconProvider : public QQuickImageProvider
{
public:
    WelcomeImageIconProvider()
        : QQuickImageProvider(Pixmap)
    {
    }

    QPixmap requestPixmap(const QString &id, QSize *size, const QSize &requestedSize) override
    {
        Q_UNUSED(requestedSize)

        QString maskFile;
        Theme::Color themeColor = Theme::Welcome_ForegroundPrimaryColor;

        const QStringList elements = id.split(QLatin1Char('/'));

        if (!elements.empty())
            maskFile = elements.first();

        if (elements.count() >= 2) {
            const static QMetaObject &m = Theme::staticMetaObject;
            const static QMetaEnum e = m.enumerator(m.indexOfEnumerator("Color"));
            bool success = false;
            int value = e.keyToValue(elements.at(1).toLatin1(), &success);
            if (success)
                themeColor = Theme::Color(value);
        }

        const QString fileName = QString::fromLatin1(":/welcome/images/%1.png").arg(maskFile);
        const Icon icon({{fileName, themeColor}}, Icon::Tint);
        const QPixmap result = icon.pixmap();
        if (size)
            *size = result.size();
        return result;
    }
};

class WelcomeMode : public IMode
{
    Q_OBJECT
    Q_PROPERTY(int activePlugin READ activePlugin WRITE setActivePlugin NOTIFY activePluginChanged)
    Q_PROPERTY(QStringList recentProjectsShortcuts READ recentProjectsShortcuts NOTIFY recentProjectsShortcutsChanged)
    Q_PROPERTY(QStringList sessionsShortcuts READ sessionsShortcuts NOTIFY sessionsShortcutsChanged)
public:
    WelcomeMode();
    ~WelcomeMode();

    void activated();
    void initPlugins();
    int activePlugin() const { return m_activePlugin; }

    QStringList recentProjectsShortcuts() const { return m_recentProjectsShortcuts; }
    QStringList sessionsShortcuts() const { return m_sessionsShortcuts; }

    Q_INVOKABLE bool openDroppedFiles(const QList<QUrl> &urls);

public slots:
    void setActivePlugin(int pos)
    {
        if (m_activePlugin != pos) {
            m_activePlugin = pos;
            emit activePluginChanged(pos);
        }
    }

signals:
    void activePluginChanged(int pos);

    void openSessionTriggered(int index);
    void openRecentProjectTriggered(int index);

    void recentProjectsShortcutsChanged(QStringList recentProjectsShortcuts);
    void sessionsShortcutsChanged(QStringList sessionsShortcuts);

private:
    void welcomePluginAdded(QObject*);
    void sceneGraphError(QQuickWindow::SceneGraphError, const QString &message);
    void facilitateQml(QQmlEngine *engine);
    void addPages(const QList<IWelcomePage *> &pages);
    void addKeyboardShortcuts();

    QWidget *m_modeWidget;
    QQuickWidget *m_welcomePage;
    QMap<Id, IWelcomePage *> m_idPageMap;
    QList<IWelcomePage *> m_pluginList;
    int m_activePlugin;
    QStringList m_recentProjectsShortcuts;
    QStringList m_sessionsShortcuts;
};

WelcomeMode::WelcomeMode()
    : m_activePlugin(0)
{
    setDisplayName(tr("Welcome"));

    const Utils::Icon MODE_WELCOME_CLASSIC(
            QLatin1String(":/welcome/images/mode_welcome.png"));
    const Utils::Icon MODE_WELCOME_FLAT({
            {QLatin1String(":/welcome/images/mode_welcome_mask.png"), Utils::Theme::IconsBaseColor}});
    const Utils::Icon MODE_WELCOME_FLAT_ACTIVE({
            {QLatin1String(":/welcome/images/mode_welcome_mask.png"), Utils::Theme::IconsModeWelcomeActiveColor}});

    setIcon(Utils::Icon::modeIcon(MODE_WELCOME_CLASSIC,
                                  MODE_WELCOME_FLAT, MODE_WELCOME_FLAT_ACTIVE));
    setPriority(Constants::P_MODE_WELCOME);
    setId(Constants::MODE_WELCOME);
    setContextHelpId(QLatin1String("Qt Creator Manual"));
    setContext(Context(Constants::C_WELCOME_MODE));

    m_modeWidget = new QWidget;
    m_modeWidget->setObjectName(QLatin1String("WelcomePageModeWidget"));
    QVBoxLayout *layout = new QVBoxLayout(m_modeWidget);
    layout->setMargin(0);
    layout->setSpacing(0);

    m_welcomePage = new QQuickWidget;
    m_welcomePage->setResizeMode(QQuickWidget::SizeRootObjectToView);

    m_welcomePage->setObjectName(QLatin1String("WelcomePage"));

    connect(m_welcomePage, &QQuickWidget::sceneGraphError,
            this, &WelcomeMode::sceneGraphError);

    StyledBar *styledBar = new StyledBar(m_modeWidget);
    styledBar->setObjectName(QLatin1String("WelcomePageStyledBar"));
    layout->addWidget(styledBar);

    m_welcomePage->setParent(m_modeWidget);
    layout->addWidget(m_welcomePage);

    addKeyboardShortcuts();

    setWidget(m_modeWidget);
}

void WelcomeMode::addKeyboardShortcuts()
{
    const int actionsCount = 9;
    Context welcomeContext(Core::Constants::C_WELCOME_MODE);

    const Id sessionBase = "Welcome.OpenSession";
    for (int i = 1; i <= actionsCount; ++i) {
        auto act = new QAction(tr("Open Session #%1").arg(i), this);
        Command *cmd = ActionManager::registerAction(act, sessionBase.withSuffix(i), welcomeContext);
        cmd->setDefaultKeySequence(QKeySequence((UseMacShortcuts ? tr("Ctrl+Meta+%1") : tr("Ctrl+Alt+%1")).arg(i)));
        m_sessionsShortcuts.append(cmd->keySequence().toString(QKeySequence::NativeText));

        connect(act, &QAction::triggered, this, [this, i] { openSessionTriggered(i-1); });
        connect(cmd, &Command::keySequenceChanged, this, [this, i, cmd] {
            m_sessionsShortcuts[i-1] = cmd->keySequence().toString(QKeySequence::NativeText);
            emit sessionsShortcutsChanged(m_sessionsShortcuts);
        });
    }

    const Id projectBase = "Welcome.OpenRecentProject";
    for (int i = 1; i <= actionsCount; ++i) {
        auto act = new QAction(tr("Open Recent Project #%1").arg(i), this);
        Command *cmd = ActionManager::registerAction(act, projectBase.withSuffix(i), welcomeContext);
        cmd->setDefaultKeySequence(QKeySequence(tr("Ctrl+Shift+%1").arg(i)));
        m_recentProjectsShortcuts.append(cmd->keySequence().toString(QKeySequence::NativeText));

        connect(act, &QAction::triggered, this, [this, i] { openRecentProjectTriggered(i-1); });
        connect(cmd, &Command::keySequenceChanged, this, [this, i, cmd] {
            m_recentProjectsShortcuts[i-1] = cmd->keySequence().toString(QKeySequence::NativeText);
            emit recentProjectsShortcutsChanged(m_recentProjectsShortcuts);
        });
    }
}

WelcomeMode::~WelcomeMode()
{
    QSettings *settings = ICore::settings();
    settings->setValue(QLatin1String(currentPageSettingsKeyC), activePlugin());
    delete m_modeWidget;
}

void WelcomeMode::sceneGraphError(QQuickWindow::SceneGraphError, const QString &message)
{
    QMessageBox *messageBox =
        new QMessageBox(QMessageBox::Warning,
                        tr("Welcome Mode Load Error"), message,
                        QMessageBox::Close, m_modeWidget);
    messageBox->setModal(false);
    messageBox->setAttribute(Qt::WA_DeleteOnClose);
    messageBox->show();
}

void WelcomeMode::facilitateQml(QQmlEngine *engine)
{
    QStringList importPathList = engine->importPathList();
    importPathList << resourcePath() + QLatin1String("/welcomescreen");
    engine->setImportPathList(importPathList);
    engine->addImageProvider(QLatin1String("icons"), new WelcomeImageIconProvider);
    if (!debug)
        engine->setOutputWarningsToStandardError(false);

    QString pluginPath = applicationDirPath();
    if (HostOsInfo::isMacHost())
        pluginPath += QLatin1String("/../PlugIns");
    else
        pluginPath += QLatin1String("/../" IDE_LIBRARY_BASENAME "/qtcreator");
    engine->addImportPath(QDir::cleanPath(pluginPath));

    QQmlContext *ctx = engine->rootContext();
    ctx->setContextProperty(QLatin1String("welcomeMode"), this);
    ctx->setContextProperty(QLatin1String("creatorTheme"), Utils::creatorTheme()->values());
    ctx->setContextProperty(QLatin1String("useNativeText"), true);
}

void WelcomeMode::initPlugins()
{
    QSettings *settings = ICore::settings();
    setActivePlugin(settings->value(QLatin1String(currentPageSettingsKeyC)).toInt());

    facilitateQml(m_welcomePage->engine());

    QList<IWelcomePage *> availablePages = PluginManager::getObjects<IWelcomePage>();
    addPages(availablePages);
    // make sure later added pages are made available too:
    connect(PluginManager::instance(), &PluginManager::objectAdded,
            this, &WelcomeMode::welcomePluginAdded);

    QString path = resourcePath() + QLatin1String("/welcomescreen/welcomescreen.qml");

    // finally, load the root page
    m_welcomePage->setSource(QUrl::fromLocalFile(path));
}

bool WelcomeMode::openDroppedFiles(const QList<QUrl> &urls)
{
    const QList<QUrl> localUrls = Utils::filtered(urls, &QUrl::isLocalFile);
    if (!localUrls.isEmpty()) {
        QTimer::singleShot(0, [localUrls]() {
            ICore::openFiles(Utils::transform(localUrls, &QUrl::toLocalFile), ICore::SwitchMode);
        });
        return true;
    }
    return false;
}

void WelcomeMode::welcomePluginAdded(QObject *obj)
{
    IWelcomePage *page = qobject_cast<IWelcomePage*>(obj);
    if (!page)
        return;
    addPages(QList<IWelcomePage *>() << page);
}

void WelcomeMode::addPages(const QList<IWelcomePage *> &pages)
{
    QList<IWelcomePage *> addedPages = pages;
    Utils::sort(addedPages, &IWelcomePage::priority);
    // insert into m_pluginList, keeping m_pluginList sorted by priority
    QQmlEngine *engine = m_welcomePage->engine();
    auto addIt = addedPages.begin();
    auto currentIt = m_pluginList.begin();
    while (addIt != addedPages.end()) {
        IWelcomePage *page = *addIt;
        QTC_ASSERT(!m_idPageMap.contains(page->id()), ++addIt; continue);
        while (currentIt != m_pluginList.end() && (*currentIt)->priority() <= page->priority())
            ++currentIt;
        // currentIt is now either end() or a page with higher value
        currentIt = m_pluginList.insert(currentIt, page);
        m_idPageMap.insert(page->id(), page);
        page->facilitateQml(engine);
        ++currentIt;
        ++addIt;
    }
    // update model through reset
    QQmlContext *ctx = engine->rootContext();
    ctx->setContextProperty(QLatin1String("pagesModel"), QVariant::fromValue(
                                Utils::transform(m_pluginList, // transform into QList<QObject *>
                                                 [](IWelcomePage *page) -> QObject * {
                                    return page;
                                })));
}

WelcomePlugin::WelcomePlugin()
  : m_welcomeMode(0)
{
}

bool WelcomePlugin::initialize(const QStringList & /* arguments */, QString *errorMessage)
{
    if (!Utils::HostOsInfo::canCreateOpenGLContext(errorMessage))
        return false;

    m_welcomeMode = new WelcomeMode;
    addAutoReleasedObject(m_welcomeMode);

    return true;
}

void WelcomePlugin::extensionsInitialized()
{
    m_welcomeMode->initPlugins();
    ModeManager::activateMode(m_welcomeMode->id());
}

} // namespace Internal
} // namespace Welcome

#include "welcomeplugin.moc"
