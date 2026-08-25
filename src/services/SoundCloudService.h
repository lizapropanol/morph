#ifndef SOUNDCLOUD_SERVICE_H
#define SOUNDCLOUD_SERVICE_H

#include "BaseService.h"
#include "../network/NetworkManager.h"

#include <QMap>

class SoundCloudService : public BaseService {
    Q_OBJECT
public:
    explicit SoundCloudService(NetworkManager* network, QObject* parent = nullptr);
    void setToken(const QString& token);
    void setAudioQuality(const QString& quality);
    void search(const QString& query) override;
    void resolveStreamUrl(const QString& trackId) override;
    void getCharts() override;
    void getWave() override;
    void getDailyMixes() override;
    void reportPlay(const QString& trackId, const QString& albumId) override;
    void importPlaylist(const QString& url) override;
signals:
    void bitrateReady(const QString& trackId, int bitrate);

private:
    void fetchStreamUrl(const QString& trackId, const QString& transcodingUrl, int bitrate);
    void fetchPlaylistTracksMetadata(const QString& playlistName, const QString& coverUrl, const QStringList& trackIds);
    void fetchNextPlaylistChunk(const QString& playlistName, const QString& coverUrl, QStringList* remainingIds, QVariantList* allTracks, const QStringList& originalIds);
    QVariantList parseSoundCloudTracks(const QJsonArray& tracks);

private:
    NetworkManager* net;
    QString m_token;
    QString m_quality = "128";
    QMap<QString, QString> m_trackLinks;
};

#endif
