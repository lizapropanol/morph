#include "PathProvider.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

QString PathProvider::getConfigPath() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!path.endsWith("/morph")) path = QDir::cleanPath(path + "/morph");
    QString canonical = QDir(path).canonicalPath();
    return canonical.isEmpty() ? path : canonical;
}

QString PathProvider::getDataPath() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!path.endsWith("/morph")) path = QDir::cleanPath(path + "/morph");
    QString canonical = QDir(path).canonicalPath();
    return canonical.isEmpty() ? path : canonical;
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

PathProvider& PathProvider::instance() {
    static PathProvider inst;
    return inst;
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

    auto restoreIfChanged = [this](const QString& resPath, const QString& diskPath) {
        bool needsRestore = false;
        QFile diskFile(diskPath);
        if (!diskFile.exists()) {
            needsRestore = true;
        } else {
            QFile resFile(resPath);
            if (resFile.open(QIODevice::ReadOnly) && diskFile.open(QIODevice::ReadOnly)) {
                if (resFile.readAll() != diskFile.readAll()) {
                    needsRestore = true;
                }
                resFile.close();
                diskFile.close();
            }
        }

        if (needsRestore) {
            bool existed = QFile::exists(diskPath);
            if (existed) QFile::remove(diskPath);
            QFile::copy(resPath, diskPath);
            QFile::setPermissions(diskPath, QFile::WriteUser | QFile::ReadUser);
            if (existed) emit configRestored(QString("Config restored: %1").arg(QFileInfo(diskPath).fileName()));
        }
    };

    restoreIfChanged(":/ui/style.qml", getStyleFilePath());
    restoreIfChanged(":/ui/style_white.qml", getConfigPath() + "/style_white.qml");
    restoreIfChanged(":/ui/auto_theme.qml", getConfigPath() + "/auto_theme.qml");

    QStringList assets = {
        "logo.svg", "home-circle.svg", "music-circle.svg", "cog.svg", "heart-outline.svg", "heart.svg", "magnify.svg", "pause-circle.svg",
        "chevron-left.svg", "chevron-right.svg", "chevron-down.svg", "chevron-up.svg",
        "pause.svg", "play-circle.svg", "play.svg", "plus-box.svg", "plus.svg",
        "repeat-once.svg", "repeat.svg", "skip-next.svg", "skip-previous.svg", 
        "soundcloud_icon.svg", "yandex_music_icon.svg", "youtube_music_icon.svg", "discord.svg", "github.svg",
        "telegram.svg", "information.svg", "harddisk.svg", "check.svg", "check-all.svg",
        "delete.svg", "close.svg", "morph.png", "notebook-outline.svg", "camera-iris.svg", "pencil.svg"
    };

    for (const QString& asset : assets) {
        QString dest = assetsPath + "/" + asset;
        if (!QFile::exists(dest)) {
            QFile::copy(":/assets/" + asset, dest);
            QFile::setPermissions(dest, QFile::WriteUser | QFile::ReadUser);
        }
    }
}
