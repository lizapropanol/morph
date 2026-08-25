#include "CacheManager.h"
#include <memory>

CacheManager::CacheManager(NetworkManager* net, QObject* parent) : QObject(parent), net(net) {}

void CacheManager::scanCacheSets() {
    m_cachedTracks.clear();
    QDir trackDir(PathProvider::getTrackCachePath());
    trackDir.setNameFilters(QStringList() << "*.mp3");
    for (const QFileInfo& fi : trackDir.entryInfoList(QDir::Files)) {
        if (fi.size() > 4096) {
            m_cachedTracks.insert(fi.baseName());
        } else {
            QFile::remove(fi.absoluteFilePath());
        }
    }

    QDir trackDirPart(PathProvider::getTrackCachePath());
    trackDirPart.setNameFilters(QStringList() << "*.part" << "*.tmp");
    for (const QString& file : trackDirPart.entryList(QDir::Files)) {
        QFile::remove(trackDirPart.filePath(file));
    }

    m_cachedCovers.clear();
    QDir coverDir(PathProvider::getCoverCachePath());
    for (const QFileInfo& fi : coverDir.entryInfoList(QDir::Files)) {
        if (fi.fileName().endsWith(".part") || fi.fileName().endsWith(".tmp")) {
            QFile::remove(fi.absoluteFilePath());
        } else if (fi.size() > 100) {
            m_cachedCovers.insert(fi.fileName());
        } else {
            QFile::remove(fi.absoluteFilePath());
        }
    }
    m_cacheScanned = true;
}

QString CacheManager::getHash(const QString& input) {
    return QString(QCryptographicHash::hash(input.toUtf8(), QCryptographicHash::Md5).toHex());
}

bool CacheManager::isTrackCached(const QString& trackId) {
    if (!m_cacheScanned) scanCacheSets();
    QString safeId = trackId;
    safeId.replace("/", "_").replace(":", "_").replace("?", "_").replace("*", "_");
    return m_cachedTracks.contains(safeId);
}

QString CacheManager::getTrackPath(const QString& trackId) {
    QString safeId = trackId;
    safeId.replace("/", "_").replace(":", "_").replace("?", "_").replace("*", "_");
    return PathProvider::getTrackCachePath() + "/" + safeId + ".mp3";
}

QString CacheManager::getTrackUrl(const QString& trackId) {
    return QUrl::fromLocalFile(getTrackPath(trackId)).toString();
}

void CacheManager::cancelOtherTrackDownloads(const QString& currentSafeId) {
    auto keys = m_activeTrackReplies.keys();
    for (const QString& key : keys) {
        if (key != currentSafeId) {
            QNetworkReply* r = m_activeTrackReplies.value(key);
            if (r) {
                r->abort();
            }
            m_activeTrackReplies.remove(key);
            QFile::remove(PathProvider::getTrackCachePath() + "/" + key + ".mp3.part");
        }
    }
}

void CacheManager::cacheTrack(const QString& trackId, const QString& url) {
    if (!m_saveTracks || isTrackCached(trackId) || url.isEmpty() || url.startsWith("file://") || url.contains(".m3u8") || url.contains("/hls")) return;
    QString safeId = trackId;
    safeId.replace("/", "_").replace(":", "_").replace("?", "_").replace("*", "_");
    cancelOtherTrackDownloads(safeId);
    performTrackDownload(trackId, QUrl(url));
}

void CacheManager::performTrackDownload(const QString& trackId, const QUrl& url, int redirectionDepth) {
    if (redirectionDepth > 5) return;

    QString safeId = trackId;
    safeId.replace("/", "_").replace(":", "_").replace("?", "_").replace("*", "_");

    if (m_activeTrackReplies.contains(safeId)) {
        return;
    }

    QString finalTrackPath = getTrackPath(trackId);
    QString tempTrackPath = finalTrackPath + ".part";

    auto file = std::make_shared<QFile>(tempTrackPath);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QNetworkRequest request(url);
    request.setTransferTimeout(30000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = net->getNetworkAccessManager()->get(request);
    m_activeTrackReplies.insert(safeId, reply);

    connect(reply, &QNetworkReply::readyRead, [file, reply]() {
        if (file->isOpen()) {
            file->write(reply->readAll());
        }
    });

    connect(reply, &QNetworkReply::finished, [this, safeId, trackId, finalTrackPath, tempTrackPath, file, reply]() {
        m_activeTrackReplies.remove(safeId);

        if (file->isOpen()) {
            file->close();
        }

        if (reply->error() == QNetworkReply::NoError) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode >= 200 && statusCode < 300) {
                QFile checkFile(tempTrackPath);
                if (checkFile.open(QIODevice::ReadOnly)) {
                    QByteArray header = checkFile.read(16);
                    checkFile.close();
                    if (!header.startsWith("#EXTM3U") && checkFile.size() > 4096) {
                        QFile::remove(finalTrackPath);
                        if (QFile::rename(tempTrackPath, finalTrackPath)) {
                            m_cachedTracks.insert(safeId);
                            enforceLimit();
                            emit trackCached(trackId, QUrl::fromLocalFile(finalTrackPath).toString());
                        }
                    } else {
                        QFile::remove(tempTrackPath);
                    }
                }
            } else {
                QFile::remove(tempTrackPath);
            }
        } else {
            QFile::remove(tempTrackPath);
        }
        reply->deleteLater();
    });
}

void CacheManager::clearTrackCache() {
    auto replies = m_activeTrackReplies.values();
    for (QNetworkReply* r : replies) {
        if (r) r->abort();
    }
    m_activeTrackReplies.clear();

    QDir dir(PathProvider::getTrackCachePath());
    dir.setNameFilters(QStringList() << "*.mp3" << "*.part" << "*.tmp");
    dir.setFilter(QDir::Files);
    for (const QString& file : dir.entryList()) {
        dir.remove(file);
    }
    m_cachedTracks.clear();
    m_cacheScanned = false;
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
    m_cacheScanned = false;
}

QString CacheManager::getCachedCover(const QString& url) {
    if (url.isEmpty()) return "";
    if (url.startsWith("file://")) return url;
    if (!m_cacheScanned) scanCacheSets();
    QString hash = getHash(url);
    if (m_cachedCovers.contains(hash)) {
        return QUrl::fromLocalFile(PathProvider::getCoverCachePath() + "/" + hash).toString();
    }
    return url;
}

void CacheManager::cacheCover(const QString& url) {
    if (!m_saveCovers || url.isEmpty() || url.startsWith("file://")) return;
    if (!m_cacheScanned) scanCacheSets();
    QString hash = getHash(url);
    if (m_cachedCovers.contains(hash) || m_activeCoverReplies.contains(hash)) return;
    QString path = PathProvider::getCoverCachePath() + "/" + hash;
    performCoverDownload(url, path, QUrl(url));
}

void CacheManager::performCoverDownload(const QString& url, const QString& path, const QUrl& targetUrl, int redirectionDepth) {
    if (redirectionDepth > 5) return;

    QString hash = getHash(url);
    if (m_activeCoverReplies.contains(hash)) return;

    QString tempPath = path + ".part";
    auto file = std::make_shared<QFile>(tempPath);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) return;

    QNetworkRequest request(targetUrl);
    request.setTransferTimeout(15000);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);

    QNetworkReply* reply = net->getNetworkAccessManager()->get(request);
    m_activeCoverReplies.insert(hash, reply);

    connect(reply, &QNetworkReply::readyRead, [file, reply]() {
        if (file->isOpen()) {
            file->write(reply->readAll());
        }
    });

    connect(reply, &QNetworkReply::finished, [this, url, hash, path, tempPath, file, reply]() {
        m_activeCoverReplies.remove(hash);

        if (file->isOpen()) {
            file->close();
        }

        if (reply->error() == QNetworkReply::NoError) {
            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode >= 200 && statusCode < 300 && QFile::exists(tempPath) && QFileInfo(tempPath).size() > 100) {
                QFile::remove(path);
                if (QFile::rename(tempPath, path)) {
                    m_cachedCovers.insert(hash);
                    enforceLimit();
                    emit coverCached(url, QUrl::fromLocalFile(path).toString());
                }
            } else {
                QFile::remove(tempPath);
            }
        } else {
            QFile::remove(tempPath);
        }
        reply->deleteLater();
    });
}

void CacheManager::clearCoverCache() {
    auto replies = m_activeCoverReplies.values();
    for (QNetworkReply* r : replies) {
        if (r) r->abort();
    }
    m_activeCoverReplies.clear();

    QDir dir(PathProvider::getCoverCachePath());
    dir.setFilter(QDir::Files);
    for (const QString& file : dir.entryList()) {
        dir.remove(file);
    }
    m_cachedCovers.clear();
    m_cacheScanned = false;
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
        return a.lastModified() < b.lastModified();
    });

    for (const QFileInfo& fi : allFiles) {
        if (currentSize <= m_limit) break;
        qint64 sz = fi.size();
        if (QFile::remove(fi.absoluteFilePath())) {
            currentSize -= sz;
        }
    }
}
