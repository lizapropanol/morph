#include "SoundCloudService.h"
#include "TrackData.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrlQuery>

SoundCloudService::SoundCloudService(NetworkManager* network, QObject* parent) 
    : BaseService(parent), net(network) {}

void SoundCloudService::setToken(const QString& token) {
    m_token = token;
}

void SoundCloudService::search(const QString& query) {
    if (m_token.isEmpty()) return;

    QUrl url("https://api-v2.soundcloud.com/search/tracks");
    QUrlQuery q;
    q.addQueryItem("q", query);
    q.addQueryItem("client_id", m_token);
    q.addQueryItem("limit", "20");
    url.setQuery(q);

    net->get(url, "", [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray tracks = doc.object()["collection"].toArray();
        
        QVariantList results;
        m_trackLinks.clear();

        for (const QJsonValue& value : tracks) {
            QJsonObject obj = value.toObject();
            TrackData track;
            track.id = QString::number(obj["id"].toInt());
            track.title = obj["title"].toString();
            track.artist = obj["user"].toObject()["username"].toString();
            track.coverUrl = obj["artwork_url"].toString().replace("-large", "-t500x500");
            
            QJsonArray transcodings = obj["media"].toObject()["transcodings"].toArray();
            for (const QJsonValue& t : transcodings) {
                QJsonObject to = t.toObject();
                if (to["format"].toObject()["protocol"].toString() == "progressive") {
                    m_trackLinks[track.id] = to["url"].toString();
                    break;
                }
            }
            
            results.append(track.toVariantMap());
        }
        emit searchResultsReady("SoundCloud", results);
    });
}

void SoundCloudService::getCharts() {
    emit chartsReady("SoundCloud", QVariantList());
}

void SoundCloudService::getWave() {
    emit waveReady("SoundCloud", QVariantList());
}

void SoundCloudService::reportPlay(const QString& trackId) {
}

void SoundCloudService::resolveStreamUrl(const QString& trackId) {
    if (m_token.isEmpty()) return;

    if (m_trackLinks.contains(trackId)) {
        fetchStreamUrl(trackId, m_trackLinks[trackId]);
    } else {
        QUrl url("https://api-v2.soundcloud.com/tracks/" + trackId);
        QUrlQuery q;
        q.addQueryItem("client_id", m_token);
        url.setQuery(q);

        net->get(url, "", [this, trackId](QNetworkReply* reply) {
            if (reply->error() != QNetworkReply::NoError) return;
            
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonArray transcodings = doc.object()["media"].toObject()["transcodings"].toArray();
            QString transcodingUrl;
            for (const QJsonValue& t : transcodings) {
                QJsonObject to = t.toObject();
                if (to["format"].toObject()["protocol"].toString() == "progressive") {
                    transcodingUrl = to["url"].toString();
                    break;
                }
            }
            
            if (!transcodingUrl.isEmpty()) {
                m_trackLinks[trackId] = transcodingUrl;
                fetchStreamUrl(trackId, transcodingUrl);
            }
        });
    }
}

void SoundCloudService::fetchStreamUrl(const QString& trackId, const QString& transcodingUrl) {
    QUrl url(transcodingUrl);
    QUrlQuery q;
    q.addQueryItem("client_id", m_token);
    url.setQuery(q);

    net->get(url, "", [this, trackId](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QString streamUrl = doc.object()["url"].toString();
        emit streamUrlReady(trackId, streamUrl);
    });
}
