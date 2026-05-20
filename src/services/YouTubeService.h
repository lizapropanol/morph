#ifndef YOUTUBE_SERVICE_H
#define YOUTUBE_SERVICE_H

#include "BaseService.h"
#include <QProcess>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

class YouTubeService : public BaseService {
    Q_OBJECT
public:
    explicit YouTubeService(QObject* parent = nullptr);
    void search(const QString& query) override;
    void resolveStreamUrl(const QString& trackId) override;
    void getCharts() override;
    void getWave() override;
    void getDailyMixes() override;
    void reportPlay(const QString& trackId, const QString& albumId) override;
    void importPlaylist(const QString& url) override;
    bool isTrackCached(const QString& trackId) override;

signals:
    void bitrateReady(const QString& trackId, int bitrate);

private:
    void runYtDlp(const QStringList& args, const QString& type, const QString& id = QString());
    QVariantMap parseYoutubeTrack(const QJsonObject& obj);
};

#endif
