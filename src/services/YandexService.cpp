#include "YandexService.h"
#include "TrackData.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>
#include <QRegularExpression>

YandexService::YandexService(NetworkManager* network, QObject* parent) 
    : BaseService(parent), net(network) {}

void YandexService::setToken(const QString& token) {
    m_token = token;
}

void YandexService::setAudioQuality(const QString& quality) {
    m_quality = quality;
}

QVariantList YandexService::parseYandexTracks(const QJsonArray& tracks) {
    QVariantList tracksList;
    for (const QJsonValue& val : tracks) {
        QJsonObject trackObj = val.isObject() && val.toObject().contains("track") ? val.toObject()["track"].toObject() : val.toObject();
        if (trackObj.isEmpty()) continue;
        
        TrackData track;
        track.id = trackObj["id"].toVariant().toString();
        track.title = trackObj["title"].toString();
        
        QJsonArray artists = trackObj["artists"].toArray();
        QStringList artistNames;
        for (const QJsonValue& a : artists) artistNames.append(a.toObject()["name"].toString());
        track.artist = artistNames.join(", ");
        
        QJsonArray albums = trackObj["albums"].toArray();
        if (!albums.isEmpty()) {
            track.album = QString::number(albums[0].toObject()["id"].toInt());
            QString coverUri = albums[0].toObject()["coverUri"].toString();
            if (!coverUri.isEmpty()) track.coverUrl = "https://" + coverUri.replace("%%", "400x400");
            track.webUrl = "https://music.yandex.ru/album/" + track.album + "/track/" + track.id;
        } else if (trackObj.contains("coverUri")) {
            track.coverUrl = "https://" + trackObj["coverUri"].toString().replace("%%", "400x400");
            track.webUrl = "https://music.yandex.ru/track/" + track.id;
        } else {
            track.webUrl = "https://music.yandex.ru/track/" + track.id;
        }
        track.service = "Yandex";
        tracksList.append(track.toVariantMap());
    }
    return tracksList;
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
            return;
        }

        QByteArray data = reply->readAll();
        QJsonArray tracks = QJsonDocument::fromJson(data).object()["result"].toObject()["tracks"].toObject()["results"].toArray();
        emit searchResultsReady("Yandex", parseYandexTracks(tracks));
    });
}

void YandexService::getCharts() {
    if (m_token.isEmpty()) return;

    QUrl url("https://api.music.yandex.net/users/414787002/playlists/1076");
    net->get(url, m_token, [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;
        QJsonArray tracks = QJsonDocument::fromJson(reply->readAll()).object()["result"].toObject()["tracks"].toArray();
        emit chartsReady("Yandex", parseYandexTracks(tracks));
    });
}

void YandexService::getWave() {
    if (m_token.isEmpty()) return;

    QUrl url("https://api.music.yandex.net/rotor/station/user:onyourwave/tracks");
    net->get(url, m_token, [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;
        QJsonArray sequence = QJsonDocument::fromJson(reply->readAll()).object()["result"].toObject()["sequence"].toArray();
        emit waveReady("Yandex", parseYandexTracks(sequence));
    });
}

void YandexService::getDailyMixes() {
    emit dailyMixesReady("Yandex", QVariantList());
}

void YandexService::importPlaylist(const QString& url) {
    if (m_token.isEmpty()) return;

    QRegularExpression classicRe("users/([^/]+)/playlists/(\\d+)");
    QRegularExpression sharedRe("playlists/([a-zA-Z0-9\\.-]+)");
    
    QRegularExpressionMatch classicMatch = classicRe.match(url);
    QRegularExpressionMatch sharedMatch = sharedRe.match(url);

    if (classicMatch.hasMatch()) {
        QString user = classicMatch.captured(1);
        QString id = classicMatch.captured(2);
        QUrl apiUrl(QString("https://api.music.yandex.net/users/%1/playlists/%2").arg(user, id));
        net->get(apiUrl, m_token, [this](QNetworkReply* reply) {
            if (reply->error() != QNetworkReply::NoError) {
                emit errorOccurred("Yandex API Error: " + reply->errorString());
                return;
            }
            QJsonObject result = QJsonDocument::fromJson(reply->readAll()).object()["result"].toObject();
            QString name = result["title"].toString();
            QString coverUrl = "";
            if (result.contains("cover")) coverUrl = "https://" + result["cover"].toObject()["uri"].toString().replace("%%", "400x400");
            else if (result.contains("ogImage")) coverUrl = "https://" + result["ogImage"].toString().replace("%%", "400x400");
            emit playlistImported(name, coverUrl, parseYandexTracks(result["tracks"].toArray()));
        });
    } else if (sharedMatch.hasMatch()) {
        QString uuid = sharedMatch.captured(1);

        QUrl apiUrl(QString("https://api.music.yandex.net/playlist/%1").arg(uuid));
        QUrlQuery q;
        q.addQueryItem("rich-tracks", "true");
        apiUrl.setQuery(q);

        net->get(apiUrl, m_token, [this, apiUrl](QNetworkReply* reply) {
            if (reply->error() != QNetworkReply::NoError) {
                QByteArray body = reply->readAll();
                emit errorOccurred(QString("Yandex API Error: %1\nURL: %2\nResponse: %3").arg(reply->errorString(), apiUrl.toString(), QString(body)));
                return;
            }
            QJsonObject result = QJsonDocument::fromJson(reply->readAll()).object()["result"].toObject();
            if (result.isEmpty()) {
                emit errorOccurred("Playlist not found (UUID)");
                return;
            }
            QString name = result["title"].toString();
            QString coverUrl = "";
            if (result.contains("cover")) coverUrl = "https://" + result["cover"].toObject()["uri"].toString().replace("%%", "400x400");
            else if (result.contains("ogImage")) coverUrl = "https://" + result["ogImage"].toString().replace("%%", "400x400");
            emit playlistImported(name, coverUrl, parseYandexTracks(result["tracks"].toArray()));
        });
    } else {
        emit errorOccurred("Invalid Yandex Music playlist URL");
    }
}

void YandexService::resolveStreamUrl(const QString& trackId) {
    if (m_token.isEmpty()) return;

    QUrl url(QString("https://api.music.yandex.net/tracks/%1/download-info").arg(trackId));
    net->get(url, m_token, [this, trackId](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray results = doc.object()["result"].toArray();
        if (results.isEmpty()) return;

        int targetBitrate = 320;
        if (m_quality == "low") targetBitrate = 192;
        else if (m_quality == "high") targetBitrate = 2000;
        
        int bestIdx = 0;
        int minDiff = 10000;

        for (int i = 0; i < results.size(); ++i) {
            int br = results[i].toObject()["bitrateInKbps"].toInt();
            int diff = qAbs(br - targetBitrate);
            if (diff < minDiff) {
                minDiff = diff;
                bestIdx = i;
            }
        }

        QString downloadInfoUrl = results[bestIdx].toObject()["downloadInfoUrl"].toString();
        fetchDownloadInfo(trackId, QUrl(downloadInfoUrl));
    });
}

#include <QCryptographicHash>
#include <QXmlStreamReader>
#include <QDateTime>

void YandexService::reportPlay(const QString& trackId, const QString& albumId) {
    if (m_token.isEmpty()) return;

    QUrl url("https://api.music.yandex.net/play-audio");
    QString timestamp = QDateTime::currentDateTimeUtc().toString("yyyy-MM-ddTHH:mm:ss.zzzZ");
    
    QUrlQuery q;
    q.addQueryItem("track-id", trackId);
    if (!albumId.isEmpty() && albumId != "0") q.addQueryItem("album-id", albumId);
    q.addQueryItem("from", "desktop-win");
    q.addQueryItem("play-id", QString::number(QDateTime::currentMSecsSinceEpoch()) + "-morph");
    q.addQueryItem("timestamp", timestamp);
    q.addQueryItem("client-now", timestamp);
    q.addQueryItem("track-length-seconds", "180");
    q.addQueryItem("total-played-seconds", "0");
    q.addQueryItem("end-position-seconds", "0");

    net->post(url, q.toString(QUrl::FullyEncoded).toUtf8(), m_token, [](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) {
        } else {
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
