#ifndef APPLICATION_H
#define APPLICATION_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QSystemTrayIcon>
#include <QMenu>
#include "utils/FileWatcher.h"
#include "utils/CacheManager.h"
#include "audio/AudioEngine.h"
#include "ServiceManager.h"
#include "settings/SettingsManager.h"
#include "MprisManager.h"
#include "utils/DiscordManager.h"

class Application : public QObject {
    Q_OBJECT
public:
    explicit Application(QObject* parent = nullptr);
    void start();

public slots:
    void reload();
    void toggleWindow();
    void updateTrayMenu();

private:
    void setupTray();

    QQmlApplicationEngine* engine;
    FileWatcher* watcher;
    CacheManager* cache;
    AudioEngine* audio;
    ServiceManager* services;
    SettingsManager* settings;
    MprisManager* mpris;
    DiscordManager* discord;
    QSystemTrayIcon* trayIcon;
    QMenu* trayMenu;
    QAction* trackInfoAction;
    QAction* playPauseAction;
};

#endif
