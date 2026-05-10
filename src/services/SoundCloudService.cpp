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
            
            QJsonObject pub = obj["publisher_metadata"].toObject();
            if (!pub.isEmpty() && !pub["artist"].toString().isEmpty()) {
                track.artist = pub["artist"].toString();
            } else {
                track.artist = obj["user"].toObject()["username"].toString();
            }
            
            track.coverUrl = obj["artwork_url"].toString().replace("-large", "-t500x500");
            track.webUrl = obj["permalink_url"].toString();
            
            QJsonArray transcodings = obj["media"].toObject()["transcodings"].toArray();
            for (const QJsonValue& t : transcodings) {
                QJsonObject to = t.toObject();
                if (to["format"].toObject()["protocol"].toString() == "progressive") {
                    m_trackLinks[track.id] = to["url"].toString();
                    break;
                }
            }
            
            track.service = "SoundCloud";
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

void SoundCloudService::reportPlay(const QString& trackId, const QString& albumId) {
}

void SoundCloudService::importPlaylist(const QString& url) {
    if (m_token.isEmpty()) return;

    if (url.contains("on.soundcloud.com")) {
        net->get(QUrl(url), "", [this](QNetworkReply* reply) {
            QUrl redirectedUrl = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
            if (redirectedUrl.isValid()) {
                QString finalUrl = reply->url().resolved(redirectedUrl).toString();
                importPlaylist(finalUrl);
            } else {
                importPlaylist(reply->url().toString());
            }
        });
        return;
    }

    QUrl apiUrl("https://api-v2.soundcloud.com/resolve");
    QUrlQuery q;
    q.addQueryItem("url", url);
    q.addQueryItem("client_id", m_token);
    apiUrl.setQuery(q);

    net->get(apiUrl, "", [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred("SoundCloud API Error: " + reply->errorString());
            return;
        }
        
        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonObject result = doc.object();
        
        if (result["kind"].toString() != "playlist") {
            emit errorOccurred("Invalid SoundCloud URL or not a playlist");
            return;
        }

        QString name = result["title"].toString();
        QString coverUrl = result["artwork_url"].toString().replace("-large", "-t500x500");
        QJsonArray tracks = result["tracks"].toArray();
        
        QStringList allTrackIds;
        for (const QJsonValue& val : tracks) {
            QString tid = val.toObject()["id"].toVariant().toString();
            if (!tid.isEmpty()) allTrackIds.append(tid);
        }

        if (allTrackIds.isEmpty()) {
            emit errorOccurred("Playlist is empty");
            return;
        }

        fetchPlaylistTracksMetadata(name, coverUrl, allTrackIds);
    });
}

void SoundCloudService::fetchPlaylistTracksMetadata(const QString& playlistName, const QString& coverUrl, const QStringList& trackIds) {
    auto remainingIds = new QStringList(trackIds);
    auto allTracks = new QVariantList();
    fetchNextPlaylistChunk(playlistName, coverUrl, remainingIds, allTracks);
}

void SoundCloudService::fetchNextPlaylistChunk(const QString& playlistName, const QString& coverUrl, QStringList* remainingIds, QVariantList* allTracks) {
    if (remainingIds->isEmpty()) {
        emit playlistImported(playlistName, coverUrl, *allTracks);
        delete remainingIds;
        delete allTracks;
        return;
    }

    int chunkSize = 50;
    QStringList currentChunk;
    for (int i = 0; i < chunkSize && !remainingIds->isEmpty(); ++i) {
        currentChunk.append(remainingIds->takeFirst());
    }

    QUrl url("https://api-v2.soundcloud.com/tracks");
    QUrlQuery q;
    q.addQueryItem("ids", currentChunk.join(","));
    q.addQueryItem("client_id", m_token);
    url.setQuery(q);

    net->get(url, "", [this, playlistName, coverUrl, remainingIds, allTracks](QNetworkReply* reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QJsonArray results = QJsonDocument::fromJson(reply->readAll()).array();
            for (const QJsonValue& val : results) {
                QJsonObject obj = val.toObject();
                TrackData track;
                track.id = obj["id"].toVariant().toString();
                track.title = obj["title"].toString();
                
                QJsonObject pub = obj["publisher_metadata"].toObject();
                if (!pub.isEmpty() && !pub["artist"].toString().isEmpty()) {
                    track.artist = pub["artist"].toString();
                } else {
                    track.artist = obj["user"].toObject()["username"].toString();
                }
                
                track.coverUrl = obj["artwork_url"].toString().replace("-large", "-t500x500");
                track.webUrl = obj["permalink_url"].toString();
                track.service = "SoundCloud";
                allTracks->append(track.toVariantMap());
            }
        }
        fetchNextPlaylistChunk(playlistName, coverUrl, remainingIds, allTracks);
    });
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
