#include "YandexService.h"
#include "TrackData.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

YandexService::YandexService(NetworkManager* network, QObject* parent) 
    : BaseService(parent), net(network) {}

void YandexService::setToken(const QString& token) {
    m_token = token;
}

void YandexService::search(const QString& query) {
    if (m_token.isEmpty()) return;

    QUrl url("https://api.music.yandex.net/search");
    QUrlQuery q;
    q.addQueryItem("text", query);
    q.addQueryItem("type", "track");
    q.addQueryItem("page", "0");
    q.addQueryItem("pageSize", "20");
    url.setQuery(q);

    net->get(url, m_token, [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) {
            qCritical() << "YANDEX_ERROR:" << reply->errorString();
            return;
        }

        QByteArray data = reply->readAll();
        qDebug() << "YANDEX_DATA:" << data;

        QJsonDocument doc = QJsonDocument::fromJson(data);
        QJsonArray tracks = doc.object()["result"].toObject()["tracks"].toObject()["results"].toArray();
        
        QVariantList results;
        for (const QJsonValue& value : tracks) {
            QJsonObject obj = value.toObject();
            TrackData track;
            track.id = QString::number(obj["id"].toInt());
            track.title = obj["title"].toString();
            
            QJsonArray artists = obj["artists"].toArray();
            QStringList artistNames;
            for (const QJsonValue& a : artists) artistNames.append(a.toObject()["name"].toString());
            track.artist = artistNames.join(", ");
            
            track.coverUrl = "https://" + obj["coverUri"].toString().replace("%%", "400x400");
            results.append(track.toVariantMap());
        }
        emit searchResultsReady("Yandex", results);
    });
}

void YandexService::getCharts() {
    if (m_token.isEmpty()) return;

    QUrl url("https://api.music.yandex.net/users/414787002/playlists/1076");
    net->get(url, m_token, [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray tracks = doc.object()["result"].toObject()["tracks"].toArray();
        if (tracks.isEmpty()) return;

        QVariantList results;
        for (const QJsonValue& val : tracks) {
            QJsonObject trackObj = val.toObject()["track"].toObject();
            if (trackObj.isEmpty()) continue;
            
            TrackData track;
            track.id = QString::number(trackObj["id"].toInt());
            if (track.id == "0") track.id = trackObj["id"].toString();
            track.title = trackObj["title"].toString();
            
            QJsonArray artists = trackObj["artists"].toArray();
            QStringList artistNames;
            for (const QJsonValue& a : artists) artistNames.append(a.toObject()["name"].toString());
            track.artist = artistNames.join(", ");
            
            QJsonArray albums = trackObj["albums"].toArray();
            if (!albums.isEmpty()) {
                QString coverUri = albums[0].toObject()["coverUri"].toString();
                if (!coverUri.isEmpty()) {
                    track.coverUrl = "https://" + coverUri.replace("%%", "400x400");
                }
            } else if (trackObj.contains("coverUri")) {
                track.coverUrl = "https://" + trackObj["coverUri"].toString().replace("%%", "400x400");
            }
            
            results.append(track.toVariantMap());
        }
        emit chartsReady("Yandex", results);
    });
}

void YandexService::getWave() {
    if (m_token.isEmpty()) return;

    QUrl url("https://api.music.yandex.net/rotor/station/user:onyourwave/tracks");
    net->get(url, m_token, [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray sequence = doc.object()["result"].toObject()["sequence"].toArray();
        
        QVariantList results;
        for (const QJsonValue& val : sequence) {
            QJsonObject trackObj = val.toObject()["track"].toObject();
            if (trackObj.isEmpty()) continue;

            TrackData track;
            track.id = QString::number(trackObj["id"].toInt());
            if (track.id == "0") track.id = trackObj["id"].toString();
            track.title = trackObj["title"].toString();
            
            QJsonArray artists = trackObj["artists"].toArray();
            QStringList artistNames;
            for (const QJsonValue& a : artists) artistNames.append(a.toObject()["name"].toString());
            track.artist = artistNames.join(", ");
            
            QJsonArray albums = trackObj["albums"].toArray();
            if (!albums.isEmpty()) {
                QString coverUri = albums[0].toObject()["coverUri"].toString();
                if (!coverUri.isEmpty()) {
                    track.coverUrl = "https://" + coverUri.replace("%%", "400x400");
                }
            } else if (trackObj.contains("coverUri")) {
                track.coverUrl = "https://" + trackObj["coverUri"].toString().replace("%%", "400x400");
            }
            
            results.append(track.toVariantMap());
        }
        emit waveReady("Yandex", results);
    });
}

void YandexService::resolveStreamUrl(const QString& trackId) {
    if (m_token.isEmpty()) return;

    QUrl url(QString("https://api.music.yandex.net/tracks/%1/download-info").arg(trackId));
    net->get(url, m_token, [this, trackId](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray results = doc.object()["result"].toArray();
        if (results.isEmpty()) return;

        QString downloadInfoUrl = results[0].toObject()["downloadInfoUrl"].toString();
        fetchDownloadInfo(trackId, QUrl(downloadInfoUrl));
    });
}

#include <QCryptographicHash>
#include <QXmlStreamReader>
#include <QDateTime>

void YandexService::reportPlay(const QString& trackId) {
    if (m_token.isEmpty()) return;

    QUrl url("https://api.music.yandex.net/play-audio");
    QString timestamp = QDateTime::currentDateTimeUtc().toString(Qt::ISODate) + "Z";
    
    QUrlQuery q;
    q.addQueryItem("track-id", trackId);
    q.addQueryItem("from", "desktop_win");
    q.addQueryItem("play-id", "morph-play-" + QString::number(QDateTime::currentMSecsSinceEpoch()));
    q.addQueryItem("timestamp", timestamp);
    q.addQueryItem("client-now", timestamp);
    q.addQueryItem("total-played-seconds", "30");

    net->post(url, q.toString().toUtf8(), m_token, [](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Yandex Play Report Failed:" << reply->errorString();
        } else {
            qDebug() << "Yandex Play Report OK:" << reply->readAll();
        }
    });
}

void YandexService::fetchDownloadInfo(const QString& trackId, const QUrl& url) {
    net->get(url, "", [this, trackId](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;

        QByteArray data = reply->readAll();
        QXmlStreamReader xml(data);
        
        QString host, path, ts, s;
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()) {
                if (xml.name() == "host") host = xml.readElementText();
                else if (xml.name() == "path") path = xml.readElementText();
                else if (xml.name() == "ts") ts = xml.readElementText();
                else if (xml.name() == "s") s = xml.readElementText();
            }
        }

        if (host.isEmpty() || path.isEmpty()) return;

        QString sign = QCryptographicHash::hash(
            ("XGRlNCpTE_w0pS7i-709aj9m_ms9ndv" + path.mid(1) + s).toUtf8(),
            QCryptographicHash::Md5
        ).toHex();

        QString streamUrl = QString("https://%1/get-mp3/%2/%3%4")
            .arg(host).arg(sign).arg(ts).arg(path);
            
        emit streamUrlReady(trackId, streamUrl);
    });
}
