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

class CacheManager : public QObject {
    Q_OBJECT
public:
    explicit CacheManager(NetworkManager* net, QObject* parent = nullptr);

    Q_INVOKABLE bool isTrackCached(const QString& trackId);
    Q_INVOKABLE QString getTrackPath(const QString& trackId);
    Q_INVOKABLE QString getTrackUrl(const QString& trackId);
    void cacheTrack(const QString& trackId, const QString& url);
    Q_INVOKABLE void clearTrackCache();

    Q_INVOKABLE qint64 getTrackCacheSize();
    Q_INVOKABLE qint64 getCoverCacheSize();

    Q_INVOKABLE QString getCachedCover(const QString& url);
    Q_INVOKABLE void cacheCover(const QString& url);
    Q_INVOKABLE void clearCoverCache();

signals:
    void trackCached(const QString& trackId, const QString& localPath);
    void coverCached(const QString& url, const QString& localPath);

private:
    NetworkManager* net;
    QString getHash(const QString& input);
};

#endif
