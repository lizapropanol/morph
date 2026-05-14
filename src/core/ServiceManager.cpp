#include "ServiceManager.h"

ServiceManager::ServiceManager(CacheManager* cache, QObject* parent) : QObject(parent), cache(cache) {
    net = new NetworkManager(this);
    yandex = new YandexService(net, this);
    soundcloud = new SoundCloudService(net, this);

    auto processTracks = [this](QVariantList& results) {
        for (int i = 0; i < results.size(); ++i) {
            QVariantMap track = results[i].toMap();
            QString coverUrl = track["coverUrl"].toString();
            if (!coverUrl.isEmpty()) {
                this->cache->cacheCover(coverUrl);
            }
        }
    };

    connect(yandex, &BaseService::searchResultsReady, [this, processTracks](const QString& serviceName, QVariantList results) {
        processTracks(results);
        emit searchResultsReady(serviceName, results);
    });
    connect(soundcloud, &BaseService::searchResultsReady, [this, processTracks](const QString& serviceName, QVariantList results) {
        processTracks(results);
        emit searchResultsReady(serviceName, results);
    });

    connect(yandex, &BaseService::chartsReady, [this, processTracks](const QString& serviceName, QVariantList results) {
        processTracks(results);
        emit chartsReady(serviceName, results);
    });
    connect(soundcloud, &BaseService::chartsReady, [this, processTracks](const QString& serviceName, QVariantList results) {
        processTracks(results);
        emit chartsReady(serviceName, results);
    });

    connect(yandex, &BaseService::waveReady, [this, processTracks](const QString& serviceName, QVariantList results) {
        processTracks(results);
        emit waveReady(serviceName, results);
    });
    connect(soundcloud, &BaseService::waveReady, [this, processTracks](const QString& serviceName, QVariantList results) {
        processTracks(results);
        emit waveReady(serviceName, results);
    });
    
    connect(yandex, &BaseService::dailyMixesReady, [this](const QString& serviceName, QVariantList playlists) {
        for (int i = 0; i < playlists.size(); ++i) {
            QVariantMap pl = playlists[i].toMap();
            QString coverUrl = pl["coverUrl"].toString();
            if (!coverUrl.isEmpty()) {
                this->cache->cacheCover(coverUrl);
            }
        }
        emit dailyMixesReady(serviceName, playlists);
    });
    connect(soundcloud, &BaseService::dailyMixesReady, [this](const QString& serviceName, QVariantList playlists) {
        for (int i = 0; i < playlists.size(); ++i) {
            QVariantMap pl = playlists[i].toMap();
            QString coverUrl = pl["coverUrl"].toString();
            if (!coverUrl.isEmpty()) {
                this->cache->cacheCover(coverUrl);
            }
        }
        emit dailyMixesReady(serviceName, playlists);
    });

    connect(yandex, &BaseService::streamUrlReady, [this](const QString& trackId, const QString& streamUrl) {
        this->cache->cacheTrack(trackId, streamUrl);
        emit streamUrlReady(trackId, streamUrl);
    });
    connect(soundcloud, &BaseService::streamUrlReady, [this](const QString& trackId, const QString& streamUrl) {
        this->cache->cacheTrack(trackId, streamUrl);
        emit streamUrlReady(trackId, streamUrl);
    });

    connect(yandex, &BaseService::playlistImported, [this, processTracks](const QString& name, QString coverUrl, QVariantList tracks) {
        if (!coverUrl.isEmpty()) {
            this->cache->cacheCover(coverUrl);
        }
        processTracks(tracks);
        emit playlistImported(name, coverUrl, tracks);
    });
    connect(soundcloud, &BaseService::playlistImported, [this, processTracks](const QString& name, QString coverUrl, QVariantList tracks) {
        if (!coverUrl.isEmpty()) {
            this->cache->cacheCover(coverUrl);
        }
        processTracks(tracks);
        emit playlistImported(name, coverUrl, tracks);
    });

    connect(yandex, &BaseService::errorOccurred, this, &ServiceManager::errorOccurred);
    connect(soundcloud, &BaseService::errorOccurred, this, &ServiceManager::errorOccurred);
}

void ServiceManager::search(const QString& query, const QString& serviceName) {
    if (serviceName == "all" || serviceName == "yandex") {
        yandex->search(query);
    }
    if (serviceName == "all" || serviceName == "soundcloud") {
        soundcloud->search(query);
    }
}

void ServiceManager::getCharts() {
    yandex->getCharts();
    soundcloud->getCharts();
}

void ServiceManager::getWave() {
    yandex->getWave();
    soundcloud->getWave();
}

void ServiceManager::getDailyMixes() {
    yandex->getDailyMixes();
    soundcloud->getDailyMixes();
}

void ServiceManager::resolve(const QString& serviceName, const QString& trackId) {
    if (cache->isTrackCached(trackId)) {
        emit streamUrlReady(trackId, cache->getTrackUrl(trackId));
        return;
    }

    if (serviceName == "Yandex") {
        yandex->resolveStreamUrl(trackId);
    } else if (serviceName == "SoundCloud") {
        soundcloud->resolveStreamUrl(trackId);
    }
}

void ServiceManager::reportPlay(const QString& serviceName, const QString& trackId, const QString& albumId) {
    if (serviceName == "Yandex") {
        yandex->reportPlay(trackId, albumId);
    } else if (serviceName == "SoundCloud") {
        soundcloud->reportPlay(trackId, albumId);
    }
}

void ServiceManager::importPlaylist(const QString& url) {
    if (url.contains("yandex")) {
        yandex->importPlaylist(url);
    } else if (url.contains("soundcloud")) {
        soundcloud->importPlaylist(url);
    } else {
        emit errorOccurred("Unsupported URL. Use Yandex or SoundCloud.");
    }
}

void ServiceManager::setYandexToken(const QString& token) {
    yandex->setToken(token);
}

void ServiceManager::setSoundCloudClientId(const QString& clientId) {
    soundcloud->setToken(clientId);
}

void ServiceManager::setAudioQuality(const QString& quality) {
    yandex->setAudioQuality(quality);
}
