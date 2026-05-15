#include "Application.h"
#include <QQmlContext>
#include <QIcon>
#include <QApplication>
#include <QQuickWindow>
#include <QDebug>
#include <QUrl>
#include "utils/PathProvider.h"
#include "services/TrackData.h"

Application::Application(QObject *parent) : QObject(parent) {
    PathProvider::ensureConfigExists();
    
    engine = new QQmlApplicationEngine(this);
    net = new NetworkManager(this);
    cache = new CacheManager(net, this);
    settings = new SettingsManager(cache, this);
    audio = new AudioEngine(this);
    services = new ServiceManager(cache, this);
    mpris = new MprisManager(audio, this);
    discord = new DiscordManager(audio, settings, this);
    watcher = new FileWatcher(settings->getActiveStylePath(), this);
    watcher->setDirectory(PathProvider::getConfigPath());

    engine->rootContext()->setContextProperty("MorphAudio", audio);
    engine->rootContext()->setContextProperty("MorphServices", services);
    engine->rootContext()->setContextProperty("MorphSettings", settings);
    engine->rootContext()->setContextProperty("MorphMpris", mpris);
    engine->rootContext()->setContextProperty("MorphDiscord", discord);
    engine->rootContext()->setContextProperty("MorphCache", cache);
    engine->rootContext()->setContextProperty("MorphApp", this);

    setupTray();

    connect(watcher, &FileWatcher::fileChanged, this, &Application::reload);
    connect(watcher, &FileWatcher::directoryChanged, this, &Application::styleFilesChanged);
    connect(audio, &AudioEngine::stateChanged, this, &Application::updateTrayMenu);
    connect(mpris, &MprisManager::metadataChanged, this, &Application::updateTrayMenu);
}

void Application::start() {
    connect(engine, &QQmlApplicationEngine::objectCreated, this, [this](QObject *obj, const QUrl &objUrl) {
        if (!obj && objUrl == QUrl::fromLocalFile(settings->getActiveStylePath())) {
            qCritical() << "MORPH_ERROR: Could not load QML file!" << objUrl;
        }
    });

    engine->load(QUrl::fromLocalFile(settings->getActiveStylePath()));
}

void Application::reload() {
    engine->clearComponentCache();
    const QList<QObject*> rootObjects = engine->rootObjects();
    for (QObject* obj : rootObjects) {
        obj->deleteLater();
    }
    
    watcher->setPath(settings->getActiveStylePath());

    engine->rootContext()->setContextProperty("MorphAudio", audio);
    engine->rootContext()->setContextProperty("MorphServices", services);
    engine->rootContext()->setContextProperty("MorphSettings", settings);
    engine->rootContext()->setContextProperty("MorphMpris", mpris);
    engine->rootContext()->setContextProperty("MorphDiscord", discord);
    engine->rootContext()->setContextProperty("MorphCache", cache);
    engine->rootContext()->setContextProperty("MorphApp", this);

    engine->load(QUrl::fromLocalFile(settings->getActiveStylePath()));
}

#include <QBuffer>
#include <QImage>
#include <QCoreApplication>
#include <QQuickItem>
#include <QQuickItemGrabResult>
#include <QEventLoop>

void Application::setupTray() {
    trayIcon = new QSystemTrayIcon(QIcon(":/assets/logo.svg"), this);
    trayMenu = new QMenu();

    trackInfoAction = new QAction("Not Playing", this);
    trackInfoAction->setEnabled(false);
    
    playPauseAction = new QAction("Play", this);
    connect(playPauseAction, &QAction::triggered, this, [this]() {
        if (audio->isPlaying()) audio->pause();
        else audio->resume();
    });

    QAction* prevAction = new QAction("Previous", this);
    connect(prevAction, &QAction::triggered, mpris, &MprisManager::previousRequested);

    QAction* nextAction = new QAction("Next", this);
    connect(nextAction, &QAction::triggered, mpris, &MprisManager::nextRequested);

    QAction* toggleAction = new QAction("Show/Hide", this);
    connect(toggleAction, &QAction::triggered, this, &Application::toggleWindow);
    
    QAction* quitAction = new QAction("Quit", this);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    trayMenu->addAction(trackInfoAction);
    trayMenu->addSeparator();
    trayMenu->addAction(prevAction);
    trayMenu->addAction(playPauseAction);
    trayMenu->addAction(nextAction);
    trayMenu->addSeparator();
    trayMenu->addAction(toggleAction);
    trayMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayMenu);
    trayIcon->show();
}

void Application::updateTrayMenu() {
    QString title = mpris->currentTitle();
    QString artist = mpris->currentArtist();

    if (title.isEmpty()) {
        trackInfoAction->setText("Not Playing");
    } else {
        trackInfoAction->setText(title + " - " + artist);
    }

    if (audio->isPlaying()) {
        playPauseAction->setText("Pause");
    } else {
        playPauseAction->setText("Resume");
    }
}

void Application::toggleWindow() {
    if (engine->rootObjects().isEmpty()) return;
    QQuickWindow* window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
    if (window) {
        if (window->isVisible()) window->hide();
        else {
            window->show();
            window->raise();
        }
    }
}
