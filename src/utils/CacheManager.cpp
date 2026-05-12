#include "CacheManager.h"

CacheManager::CacheManager(NetworkManager* net, QObject* parent) : QObject(parent), net(net) {}

QString CacheManager::getHash(const QString& input) {
    return QString(QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Md5).toHex());
}

bool CacheManager::isTrackCached(const QString& trackId) {
    return QFile::exists(getTrackPath(trackId));
}

QString CacheManager::getTrackPath(const QString& trackId) {
    return PathProvider::getTrackCachePath() + "/" + trackId + ".mp3";
}

QString CacheManager::getTrackUrl(const QString& trackId) {
    return "file://" + getTrackPath(trackId);
}

void CacheManager::cacheTrack(const QString& trackId, const QString& url) {
    if (isTrackCached(trackId) || url.isEmpty() || url.startsWith("file://")) return;

    net->get(QUrl(url), "", [this, trackId](QNetworkReply* reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QFile file(getTrackPath(trackId));
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                emit trackCached(trackId, "file://" + file.fileName());
            }
        }
        reply->deleteLater();
    });
}

void CacheManager::clearTrackCache() {
    QDir dir(PathProvider::getTrackCachePath());
    dir.setNameFilters(QStringList() << "*.mp3");
    dir.setFilter(QDir::Files);
    for (const QString& file : dir.entryList()) {
        dir.remove(file);
    }
}

qint64 CacheManager::getTrackCacheSize() {
    qint64 size = 0;
    QDir dir(PathProvider::getTrackCachePath());
    for (const QFileInfo& fi : dir.entryInfoList(QDir::Files)) {
        size += fi.size();
    }
    return size;
}

qint64 CacheManager::getCoverCacheSize() {
    qint64 size = 0;
    QDir dir(PathProvider::getCoverCachePath());
    for (const QFileInfo& fi : dir.entryInfoList(QDir::Files)) {
        size += fi.size();
    }
    return size;
}

QString CacheManager::getCachedCover(const QString& url) {
    if (url.isEmpty()) return "";
    QString path = PathProvider::getCoverCachePath() + "/" + getHash(url);
    if (QFile::exists(path)) return "file://" + path;
    return url;
}

void CacheManager::cacheCover(const QString& url) {
    if (url.isEmpty() || url.startsWith("file://")) return;
    QString path = PathProvider::getCoverCachePath() + "/" + getHash(url);
    if (QFile::exists(path)) return;

    net->get(QUrl(url), "", [this, url, path](QNetworkReply* reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                emit coverCached(url, "file://" + file.fileName());
            }
        }
        reply->deleteLater();
    });
}

void CacheManager::clearCoverCache() {
    QDir dir(PathProvider::getCoverCachePath());
    dir.setFilter(QDir::Files);
    for (const QString& file : dir.entryList()) {
        dir.remove(file);
    }
}
