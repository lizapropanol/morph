#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QQmlApplicationEngine>
#include "utils/FileWatcher.h"
#include "audio/AudioEngine.h"
#include "ServiceManager.h"
#include "settings/SettingsManager.h"
#include "MprisManager.h"

class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(QObject* parent = nullptr);
    void start();

public slots:
    void reload();

private:
    QQmlApplicationEngine* engine;
    FileWatcher* watcher;
    AudioEngine* audio;
    ServiceManager* services;
    SettingsManager* settings;
    MprisManager* mpris;
};

#endif
