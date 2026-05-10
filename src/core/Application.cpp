#include "Application.h"
#include "utils/PathProvider.h"
#include <QUrl>
#include <QQmlContext>

Application::Application(QObject* parent) : QObject(parent) {
    engine = new QQmlApplicationEngine(this);
    watcher = new FileWatcher(PathProvider::getStyleFilePath(), this);
    audio = new AudioEngine(this);
    services = new ServiceManager(this);
    settings = new SettingsManager(this);
    mpris = new MprisManager(audio, this);

    services->setAudioQuality(settings->getAudioQuality());

    engine->rootContext()->setContextProperty("MorphAudio", audio);
    engine->rootContext()->setContextProperty("MorphServices", services);
    engine->rootContext()->setContextProperty("MorphSettings", settings);
    engine->rootContext()->setContextProperty("MorphMpris", mpris);

    connect(watcher, &FileWatcher::fileChanged, this, &Application::reload);
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

    engine->load(QUrl::fromLocalFile(PathProvider::getStyleFilePath()));
}
