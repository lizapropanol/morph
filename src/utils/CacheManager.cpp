#include "CacheManager.h"
#include <memory>
#include <QFileInfo>

CacheManager::CacheManager(NetworkManager* net, QObject* parent) : QObject(parent), net(net) {}

void CacheManager::scanCacheSets() {
    m_cachedTracks.clear();
    QString trackPath = PathProvider::getTrackCachePath();
    QDir().mkpath(trackPath);
    QDir trackDir(trackPath);
    trackDir.setNameFilters(QStringList() << "*.mp3");
    for (const QFileInfo& fi : trackDir.entryInfoList(QDir::Files)) {
        if (fi.size() > 4096) {
            m_cachedTracks.insert(fi.baseName());
        } else {
            QFile::remove(fi.absoluteFilePath());
        }
    }

    QDir trackDirPart(trackPath);
    trackDirPart.setNameFilters(QStringList() << "*.part" << "*.tmp");
    for (const QString& file : trackDirPart.entryList(QDir::Files)) {
        QFile::remove(trackDirPart.filePath(file));
    }

    m_cachedCovers.clear();
    QString coverPath = PathProvider::getCoverCachePath();
    QDir().mkpath(coverPath);
    QDir coverDir(coverPath);
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

void CacheManager::cacheTrack(const QString& trackId, const QString& url) {
    if (isTrackCached(trackId) || url.isEmpty() || url.startsWith("file://")) return;
    performTrackDownload(trackId, QUrl(url));
}

void CacheManager::performTrackDownload(const QString& trackId, const QUrl& url, int redirectionDepth) {
    if (redirectionDepth > 5) return;

    QString safeId = trackId;
    safeId.replace("/", "_").replace(":", "_").replace("?", "_").replace("*", "_");

    if (m_activeTrackReplies.contains(safeId) && redirectionDepth == 0) {
        return;
    }

    QString finalTrackPath = getTrackPath(trackId);
    QString tempTrackPath = finalTrackPath + ".part";
    QDir().mkpath(QFileInfo(finalTrackPath).dir().absolutePath());

    if (url.toString().contains(".m3u8") || url.toString().contains("/hls")) {
        QNetworkRequest req(url);
        req.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
        req.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

        QNetworkReply* reply = net->getNetworkAccessManager()->get(req);
        reply->ignoreSslErrors();
        m_activeTrackReplies.insert(safeId, reply);

        connect(reply, &QNetworkReply::finished, [this, safeId, trackId, url, finalTrackPath, tempTrackPath, reply, redirectionDepth]() {
            m_activeTrackReplies.remove(safeId);

            int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307 || statusCode == 308) {
                QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
                if (redirectUrl.isRelative()) redirectUrl = url.resolved(redirectUrl);
                reply->deleteLater();
                performTrackDownload(trackId, redirectUrl, redirectionDepth + 1);
                return;
            }

            QByteArray content = reply->readAll();
            reply->deleteLater();

            QStringList lines = QString::fromUtf8(content).split('\n');
            QStringList segUrls;
            for (const QString& line : lines) {
                QString trimmed = line.trimmed();
                if (trimmed.startsWith("#EXT-X-MAP:URI=\"")) {
                    int end = trimmed.lastIndexOf('"');
                    if (end > 16) {
                        QString initUri = trimmed.mid(16, end - 16);
                        QUrl fullInit = initUri.startsWith("http") ? QUrl(initUri) : url.resolved(QUrl(initUri));
                        segUrls.append(fullInit.toString());
                    }
                } else if (!trimmed.isEmpty() && !trimmed.startsWith("#")) {
                    QUrl fullSeg = trimmed.startsWith("http") ? QUrl(trimmed) : url.resolved(QUrl(trimmed));
                    segUrls.append(fullSeg.toString());
                }
            }

            if (segUrls.isEmpty()) return;

            auto file = std::make_shared<QFile>(tempTrackPath);
            if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) return;

            auto currentIndex = std::make_shared<int>(0);
            auto downloadStep = std::make_shared<std::function<void()>>();

            *downloadStep = [this, safeId, trackId, finalTrackPath, tempTrackPath, segUrls, file, currentIndex, downloadStep]() {
                if (*currentIndex >= segUrls.size()) {
                    file->flush();
                    file->close();
                    if (QFileInfo(tempTrackPath).size() > 4096) {
                        QFile::remove(finalTrackPath);
                        bool moved = QFile::rename(tempTrackPath, finalTrackPath);
                        if (!moved) {
                            if (QFile::copy(tempTrackPath, finalTrackPath)) {
                                QFile::remove(tempTrackPath);
                                moved = true;
                            }
                        }
                        if (moved) {
                            m_cachedTracks.insert(safeId);
                            enforceLimit();
                            emit trackCached(trackId, QUrl::fromLocalFile(finalTrackPath).toString());
                        } else {
                            QFile::remove(tempTrackPath);
                        }
                    } else {
                        QFile::remove(tempTrackPath);
                    }
                    return;
                }

                QUrl segUrl(segUrls[*currentIndex]);
                QNetworkRequest segReq(segUrl);
                segReq.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
                segReq.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

                QNetworkReply* segReply = net->getNetworkAccessManager()->get(segReq);
                segReply->ignoreSslErrors();
                m_activeTrackReplies.insert(safeId, segReply);

                connect(segReply, &QNetworkReply::finished, [this, safeId, segReply, file, currentIndex, downloadStep]() {
                    m_activeTrackReplies.remove(safeId);
                    if (segReply->error() == QNetworkReply::NoError && file->isOpen()) {
                        file->write(segReply->readAll());
                        file->flush();
                    }
                    segReply->deleteLater();
                    (*currentIndex)++;
                    (*downloadStep)();
                });
            };

            (*downloadStep)();
        });
        return;
    }

    auto file = std::make_shared<QFile>(tempTrackPath);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

    QNetworkReply* reply = net->getNetworkAccessManager()->get(request);
    reply->ignoreSslErrors();
    m_activeTrackReplies.insert(safeId, reply);

    connect(reply, &QNetworkReply::readyRead, [file, reply]() {
        if (file->isOpen()) {
            file->write(reply->readAll());
            file->flush();
        }
    });

    connect(reply, &QNetworkReply::finished, [this, safeId, trackId, url, finalTrackPath, tempTrackPath, file, reply, redirectionDepth]() {
        m_activeTrackReplies.remove(safeId);

        if (file->isOpen()) {
            file->write(reply->readAll());
            file->flush();
            file->close();
        }

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307 || statusCode == 308) {
            QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (redirectUrl.isRelative()) redirectUrl = url.resolved(redirectUrl);
            QFile::remove(tempTrackPath);
            reply->deleteLater();
            performTrackDownload(trackId, redirectUrl, redirectionDepth + 1);
            return;
        }

        QFile checkFile(tempTrackPath);
        qint64 fileSize = checkFile.size();
        bool valid = false;
        bool isHlsManifest = false;
        if (fileSize > 100 && checkFile.open(QIODevice::ReadOnly)) {
            QByteArray header = checkFile.read(32);
            checkFile.close();
            if (header.startsWith("#EXTM3U")) {
                isHlsManifest = true;
            } else if (fileSize > 4096 && !header.toLower().startsWith("<!doctype") && !header.toLower().startsWith("<html")) {
                valid = true;
            }
        }

        reply->deleteLater();

        if (isHlsManifest) {
            QFile::remove(tempTrackPath);
            performTrackDownload(trackId, url, redirectionDepth);
            return;
        }

        if (valid) {
            QFile::remove(finalTrackPath);
            bool moved = QFile::rename(tempTrackPath, finalTrackPath);
            if (!moved) {
                if (QFile::copy(tempTrackPath, finalTrackPath)) {
                    QFile::remove(tempTrackPath);
                    moved = true;
                }
            }
            if (moved) {
                m_cachedTracks.insert(safeId);
                enforceLimit();
                emit trackCached(trackId, QUrl::fromLocalFile(finalTrackPath).toString());
            } else {
                QFile::remove(tempTrackPath);
            }
        } else {
            QFile::remove(tempTrackPath);
        }
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
    if (m_activeCoverReplies.contains(hash) && redirectionDepth == 0) return;

    QString tempPath = path + ".part";
    QDir().mkpath(QFileInfo(path).dir().absolutePath());
    auto file = std::make_shared<QFile>(tempPath);
    if (!file->open(QIODevice::WriteOnly | QIODevice::Truncate)) return;

    QNetworkRequest request(targetUrl);
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute, QNetworkRequest::NoLessSafeRedirectPolicy);
    request.setRawHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36");

    QNetworkReply* reply = net->getNetworkAccessManager()->get(request);
    reply->ignoreSslErrors();
    m_activeCoverReplies.insert(hash, reply);

    connect(reply, &QNetworkReply::readyRead, [file, reply]() {
        if (file->isOpen()) {
            file->write(reply->readAll());
            file->flush();
        }
    });

    connect(reply, &QNetworkReply::finished, [this, url, hash, path, tempPath, targetUrl, file, reply, redirectionDepth]() {
        m_activeCoverReplies.remove(hash);

        if (file->isOpen()) {
            file->write(reply->readAll());
            file->flush();
            file->close();
        }

        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        if (statusCode == 301 || statusCode == 302 || statusCode == 303 || statusCode == 307 || statusCode == 308) {
            QUrl redirectUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (redirectUrl.isRelative()) redirectUrl = targetUrl.resolved(redirectUrl);
            QFile::remove(tempPath);
            reply->deleteLater();
            performCoverDownload(url, path, redirectUrl, redirectionDepth + 1);
            return;
        }

        if (QFile::exists(tempPath) && QFileInfo(tempPath).size() > 100) {
            QFile::remove(path);
            bool moved = QFile::rename(tempPath, path);
            if (!moved) {
                if (QFile::copy(tempPath, path)) {
                    QFile::remove(tempPath);
                    moved = true;
                }
            }
            if (moved) {
                m_cachedCovers.insert(hash);
                enforceLimit();
                emit coverCached(url, QUrl::fromLocalFile(path).toString());
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
