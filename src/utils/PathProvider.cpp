#include "PathProvider.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

QString PathProvider::getConfigPath() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!path.endsWith("/morph")) path = QDir::cleanPath(path + "/morph");
    return path;
}

QString PathProvider::getDataPath() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!path.endsWith("/morph")) path = QDir::cleanPath(path + "/morph");
    return path;
}

QString PathProvider::getStyleFilePath() {
    return getConfigPath() + "/style.qml";
}

QString PathProvider::getTrackCachePath() {
    return getDataPath() + "/cache/tracks";
}

QString PathProvider::getCoverCachePath() {
    return getDataPath() + "/cache/covers";
}

void PathProvider::ensureConfigExists() {
    QString configPath = getConfigPath();
    QString dataPath = getDataPath();
    QString assetsPath = configPath + "/assets";
    
    QDir().mkpath(configPath);
    QDir().mkpath(dataPath);
    QDir().mkpath(assetsPath);
    QDir().mkpath(getTrackCachePath());
    QDir().mkpath(getCoverCachePath());

    QString oldSettings = configPath + "/settings.json";
    QString newSettings = dataPath + "/settings.json";
    if (QFile::exists(oldSettings) && !QFile::exists(newSettings)) {
        QFile::rename(oldSettings, newSettings);
    }

    QString styleFile = getStyleFilePath();
    if (!QFile::exists(styleFile)) {
        QFile::copy(":/ui/style.qml", styleFile);
        QFile::setPermissions(styleFile, QFile::WriteUser | QFile::ReadUser);
    }

    QString lightStyleFile = getConfigPath() + "/style_white.qml";
    if (!QFile::exists(lightStyleFile)) {
        QFile::copy(":/ui/style_white.qml", lightStyleFile);
        QFile::setPermissions(lightStyleFile, QFile::WriteUser | QFile::ReadUser);
    }

    QStringList assets = {
        "logo.svg", "heart-outline.svg", "heart.svg", "magnify.svg", "pause-circle.svg",
        "chevron-left.svg", "chevron-right.svg", "chevron-down.svg", "chevron-up.svg",
        "pause.svg", "play-circle.svg", "play.svg", "plus-box.svg", "plus.svg",
        "repeat-once.svg", "repeat.svg", "skip-next.svg", "skip-previous.svg", 
        "soundcloud_icon.svg", "yandex_music_icon.svg", "discord.svg", "github.svg",
        "telegram.svg", "information.svg", "harddisk.svg", "check.svg", "check-all.svg",
        "close.svg", "morph.png", "notebook-outline.svg", "camera-iris.svg", "pencil.svg"
    };

    for (const QString& asset : assets) {
        QString dest = assetsPath + "/" + asset;
        if (!QFile::exists(dest)) {
            QFile::copy(":/assets/" + asset, dest);
            QFile::setPermissions(dest, QFile::WriteUser | QFile::ReadUser);
        }
    }
}
