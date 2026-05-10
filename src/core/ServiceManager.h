#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include <QObject>
#include <QVariantList>
#include "../network/NetworkManager.h"
#include "../services/YandexService.h"
#include "../services/SoundCloudService.h"

class ServiceManager : public QObject {
    Q_OBJECT
public:
    explicit ServiceManager(QObject* parent = nullptr);

public slots:
    void search(const QString& query, const QString& serviceName = "all");
    void getCharts();
    void getWave();
    void resolve(const QString& serviceName, const QString& trackId);
    void reportPlay(const QString& serviceName, const QString& trackId, const QString& albumId);
    void importPlaylist(const QString& url);
    void setYandexToken(const QString& token);
    void setSoundCloudClientId(const QString& clientId);
    void setAudioQuality(const QString& quality);

signals:
    void searchResultsReady(const QString& serviceName, const QVariantList& results);
    void streamUrlReady(const QString& trackId, const QString& streamUrl);
    void chartsReady(const QString& serviceName, const QVariantList& tracks);
    void waveReady(const QString& serviceName, const QVariantList& tracks);
    void playlistImported(const QString& name, const QString& coverUrl, const QVariantList& tracks);
    void errorOccurred(const QString& message);

private:
    NetworkManager* net;
    YandexService* yandex;
    SoundCloudService* soundcloud;
};

#endif
