#include "ServiceManager.h"

ServiceManager::ServiceManager(QObject* parent) : QObject(parent) {
    net = new NetworkManager(this);
    yandex = new YandexService(net, this);
    soundcloud = new SoundCloudService(net, this);

    connect(yandex, &BaseService::searchResultsReady, [this](const QString& serviceName, const QVariantList& results) {
        emit searchResultsReady(serviceName, results);
    });

    connect(soundcloud, &BaseService::searchResultsReady, [this](const QString& serviceName, const QVariantList& results) {
        emit searchResultsReady(serviceName, results);
    });

    connect(yandex, &BaseService::chartsReady, this, &ServiceManager::chartsReady);
    connect(soundcloud, &BaseService::chartsReady, this, &ServiceManager::chartsReady);
    connect(yandex, &BaseService::waveReady, this, &ServiceManager::waveReady);
    connect(soundcloud, &BaseService::waveReady, this, &ServiceManager::waveReady);

    connect(yandex, &BaseService::streamUrlReady, this, &ServiceManager::streamUrlReady);
    connect(soundcloud, &BaseService::streamUrlReady, this, &ServiceManager::streamUrlReady);
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

void ServiceManager::resolve(const QString& serviceName, const QString& trackId) {
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

void ServiceManager::setYandexToken(const QString& token) {
    yandex->setToken(token);
}

void ServiceManager::setSoundCloudClientId(const QString& clientId) {
    soundcloud->setToken(clientId);
}
