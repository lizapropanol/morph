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
    void search(const QString& query) override;
    void resolveStreamUrl(const QString& trackId) override;
    void getCharts() override;
    void getWave() override;
    void reportPlay(const QString& trackId, const QString& albumId) override;
    void importPlaylist(const QString& url) override;

private:
    void fetchStreamUrl(const QString& trackId, const QString& transcodingUrl);
    void fetchPlaylistTracksMetadata(const QString& playlistName, const QString& coverUrl, const QStringList& trackIds);
    void fetchNextPlaylistChunk(const QString& playlistName, const QString& coverUrl, QStringList* remainingIds, QVariantList* allTracks);

private:
    NetworkManager* net;
    QString m_token;
    QMap<QString, QString> m_trackLinks;
};

#endif
