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
    if (!m_saveTracks || isTrackCached(trackId) || url.isEmpty() || url.startsWith("file://")) return;

    net->get(QUrl(url), "", [this, trackId](QNetworkReply* reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QFile file(getTrackPath(trackId));
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                enforceLimit();
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

int CacheManager::getTrackCacheCount() {
    QDir dir(PathProvider::getTrackCachePath());
    dir.setNameFilters(QStringList() << "*.mp3");
    return dir.entryList(QDir::Files).count();
}

int CacheManager::getCoverCacheCount() {
    QDir dir(PathProvider::getCoverCachePath());
    return dir.entryList(QDir::Files).count();
}

QVariantList CacheManager::getTrackCacheItems() {
    QVariantList items;
    QDir dir(PathProvider::getTrackCachePath());
    dir.setNameFilters(QStringList() << "*.mp3");
    for (const QFileInfo& fi : dir.entryInfoList(QDir::Files)) {
        QVariantMap item;
        item["name"] = fi.fileName();
        item["size"] = fi.size();
        item["id"] = fi.baseName();
        items.append(item);
    }
    return items;
}

QVariantList CacheManager::getCoverCacheItems() {
    QVariantList items;
    QDir dir(PathProvider::getCoverCachePath());
    for (const QFileInfo& fi : dir.entryInfoList(QDir::Files)) {
        QVariantMap item;
        item["name"] = fi.fileName();
        item["size"] = fi.size();
        items.append(item);
    }
    return items;
}

void CacheManager::removeCacheFile(const QString& fileName, bool isTrack) {
    QString path = (isTrack ? PathProvider::getTrackCachePath() : PathProvider::getCoverCachePath()) + "/" + fileName;
    QFile::remove(path);
}

QString CacheManager::getCachedCover(const QString& url) {
    if (url.isEmpty()) return "";
    QString path = PathProvider::getCoverCachePath() + "/" + getHash(url);
    if (QFile::exists(path)) return "file://" + path;
    return url;
}

void CacheManager::cacheCover(const QString& url) {
    if (!m_saveCovers || url.isEmpty() || url.startsWith("file://")) return;
    QString path = PathProvider::getCoverCachePath() + "/" + getHash(url);
    if (QFile::exists(path)) return;

    net->get(QUrl(url), "", [this, url, path](QNetworkReply* reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QFile file(path);
            if (file.open(QIODevice::WriteOnly)) {
                file.write(reply->readAll());
                file.close();
                enforceLimit();
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

void CacheManager::setLimit(qint64 bytes) {
    m_limit = bytes;
    enforceLimit();
}

void CacheManager::setSaveTracks(bool save) {
    m_saveTracks = save;
}

void CacheManager::setSaveCovers(bool save) {
    m_saveCovers = save;
}

void CacheManager::enforceLimit() {
    if (m_limit <= 0) return;

    qint64 currentSize = getTrackCacheSize() + getCoverCacheSize();
    if (currentSize <= m_limit) return;

    QFileInfoList allFiles;
    QDir trackDir(PathProvider::getTrackCachePath());
    allFiles.append(trackDir.entryInfoList(QDir::Files));
    QDir coverDir(PathProvider::getCoverCachePath());
    allFiles.append(coverDir.entryInfoList(QDir::Files));

    std::sort(allFiles.begin(), allFiles.end(), [](const QFileInfo& a, const QFileInfo& b) {
        return a.lastRead() < b.lastRead();
    });

    for (const QFileInfo& fi : allFiles) {
        if (currentSize <= m_limit) break;
        qint64 sz = fi.size();
        if (QFile::remove(fi.absoluteFilePath())) {
            currentSize -= sz;
        }
    }
}
