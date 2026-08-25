#ifndef CACHE_MANAGER_H
#define CACHE_MANAGER_H

#include <QObject>
#include <QString>
#include <QDir>
#include <QFile>
#include <QUrl>
#include <QCryptographicHash>
#include "../network/NetworkManager.h"
#include "PathProvider.h"

#include <QSet>

class CacheManager : public QObject {
    Q_OBJECT
public:
    explicit CacheManager(NetworkManager* net, QObject* parent = nullptr);

    Q_INVOKABLE bool isTrackCached(const QString& trackId);
    Q_INVOKABLE QString getTrackPath(const QString& trackId);
    Q_INVOKABLE QString getTrackUrl(const QString& trackId);
    Q_INVOKABLE void cacheTrack(const QString& trackId, const QString& url);
    Q_INVOKABLE void clearTrackCache();

    Q_INVOKABLE qint64 getTrackCacheSize();
    Q_INVOKABLE qint64 getCoverCacheSize();
    
    Q_INVOKABLE int getTrackCacheCount();
    Q_INVOKABLE int getCoverCacheCount();
    
    Q_INVOKABLE QVariantList getTrackCacheItems();
    Q_INVOKABLE QVariantList getCoverCacheItems();
    Q_INVOKABLE void removeCacheFile(const QString& fileName, bool isTrack);

    Q_INVOKABLE QString getCachedCover(const QString& url);
    Q_INVOKABLE void cacheCover(const QString& url);
    Q_INVOKABLE void clearCoverCache();

    Q_INVOKABLE void setLimit(qint64 bytes);

    void setSaveTracks(bool save);
    void setSaveCovers(bool save);

signals:
    void trackCached(const QString& trackId, const QString& localPath);
    void coverCached(const QString& url, const QString& localPath);

private:
    NetworkManager* net;
    qint64 m_limit = 0;
    bool m_saveTracks = true;
    bool m_saveCovers = true;
    bool m_cacheScanned = false;
    QSet<QString> m_cachedTracks;
    QSet<QString> m_cachedCovers;
    QMap<QString, QNetworkReply*> m_activeTrackReplies;
    QMap<QString, QNetworkReply*> m_activeCoverReplies;
    void scanCacheSets();
    QString getHash(const QString& input);
    void enforceLimit();
    void cancelOtherTrackDownloads(const QString& currentSafeId);
    void performTrackDownload(const QString& trackId, const QUrl& url, int redirectionDepth = 0);
    void performCoverDownload(const QString& url, const QString& path, const QUrl& targetUrl, int redirectionDepth = 0);
};

#endif
