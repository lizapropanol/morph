#include "Application.h"
#include "utils/PathProvider.h"
#include <QUrl>
#include <QQmlContext>
#include <QQuickWindow>
#include <QApplication>

Application::Application(QObject* parent) : QObject(parent) {
    engine = new QQmlApplicationEngine(this);
    watcher = new FileWatcher(PathProvider::getStyleFilePath(), this);
    
    NetworkManager* tempNet = new NetworkManager(this);
    cache = new CacheManager(tempNet, this);
    
    audio = new AudioEngine(this);
    services = new ServiceManager(cache, this);
    settings = new SettingsManager(cache, this);
    mpris = new MprisManager(audio, this);

    services->setAudioQuality(settings->getAudioQuality());

    engine->rootContext()->setContextProperty("MorphAudio", audio);
    engine->rootContext()->setContextProperty("MorphServices", services);
    engine->rootContext()->setContextProperty("MorphSettings", settings);
    engine->rootContext()->setContextProperty("MorphMpris", mpris);
    engine->rootContext()->setContextProperty("MorphCache", cache);

    setupTray();

    connect(watcher, &FileWatcher::fileChanged, this, &Application::reload);
    connect(audio, &AudioEngine::stateChanged, this, &Application::updateTrayMenu);
    connect(mpris, &MprisManager::metadataChanged, this, &Application::updateTrayMenu);
}

void Application::start() {
    connect(engine, &QQmlApplicationEngine::objectCreated, this, [this](QObject *obj, const QUrl &objUrl) {
        if (!obj && objUrl == QUrl::fromLocalFile(PathProvider::getStyleFilePath())) {
            qCritical() << "MORPH_ERROR: Could not load QML file!" << objUrl;
        }
    }, Qt::QueuedConnection);

    engine->load(QUrl::fromLocalFile(PathProvider::getStyleFilePath()));
}

void Application::reload() {
    engine->clearComponentCache();
    const QList<QObject*> rootObjects = engine->rootObjects();
    for (QObject* obj : rootObjects) {
        obj->deleteLater();
    }
    
    engine->rootContext()->setContextProperty("MorphAudio", audio);
    engine->rootContext()->setContextProperty("MorphServices", services);
    engine->rootContext()->setContextProperty("MorphSettings", settings);
    engine->rootContext()->setContextProperty("MorphMpris", mpris);
    engine->rootContext()->setContextProperty("MorphCache", cache);

    engine->load(QUrl::fromLocalFile(PathProvider::getStyleFilePath()));
}

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
    trayMenu->addSeparator();
    trayMenu->addAction(quitAction);

    trayIcon->setContextMenu(trayMenu);
    
    connect(trayIcon, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger) {
            toggleWindow();
        }
    });

    trayIcon->show();
}

void Application::updateTrayMenu() {
    QString title = mpris->currentTitle();
    QString artist = mpris->currentArtist();

    if (title.isEmpty()) {
        trackInfoAction->setText("Not Playing");
    } else {
        trackInfoAction->setText(artist + " - " + title);
    }

    if (audio->isPlaying()) {
        playPauseAction->setText("Pause");
    } else {
        playPauseAction->setText("Play");
    }
}

void Application::toggleWindow() {
    if (engine->rootObjects().isEmpty()) return;
    
    QQuickWindow* window = qobject_cast<QQuickWindow*>(engine->rootObjects().first());
    if (!window) return;

    if (window->isVisible()) {
        window->hide();
    } else {
        window->show();
        window->raise();
        window->requestActivate();
    }
}
