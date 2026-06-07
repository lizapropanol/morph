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

QVariantList SoundCloudService::parseSoundCloudTracks(const QJsonArray& tracks) {
    QVariantList results;
    for (const QJsonValue& value : tracks) {
        QJsonObject obj = value.toObject();
        TrackData track;
        track.id = obj["id"].toVariant().toString();
        track.title = obj["title"].toString();
        track.durationMs = obj["duration"].toVariant().toLongLong();
        
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
    return results;
}

void SoundCloudService::search(const QString& query) {
    if (m_token.isEmpty()) return;

    QString authHeader = "";
    if (m_token.startsWith("2-")) {
        authHeader = "OAuth " + m_token;
    }

    if (query.startsWith("http") && query.contains("soundcloud.com")) {
        QUrl url("https://api-v2.soundcloud.com/resolve");
        QUrlQuery q;
        q.addQueryItem("url", query);
        if (authHeader.isEmpty()) q.addQueryItem("client_id", m_token);
        url.setQuery(q);

        net->get(url, authHeader, [this](QNetworkReply* reply) {
            if (reply->error() != QNetworkReply::NoError) return;
            QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
            QJsonObject result = doc.object();
            if (result["kind"].toString() == "track") {
                QJsonArray tracks;
                tracks.append(result);
                emit searchResultsReady("SoundCloud", parseSoundCloudTracks(tracks));
            }
        });
        return;
    }

    QUrl url("https://api-v2.soundcloud.com/search/tracks");
    QUrlQuery q;
    q.addQueryItem("q", query);
    if (authHeader.isEmpty()) {
        q.addQueryItem("client_id", m_token);
    }
    q.addQueryItem("limit", "100");
    url.setQuery(q);

    net->get(url, authHeader, [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray tracks = doc.object()["collection"].toArray();
        m_trackLinks.clear();
        emit searchResultsReady("SoundCloud", parseSoundCloudTracks(tracks));
    });
}

void SoundCloudService::getCharts() {
    emit chartsReady("SoundCloud", QVariantList());
}

void SoundCloudService::getWave() {
    emit waveReady("SoundCloud", QVariantList());
}

void SoundCloudService::getDailyMixes() {
    if (m_token.isEmpty()) return;

    QUrl url("https://api-v2.soundcloud.com/mixed-selections");
    QString authHeader = "";
    if (m_token.startsWith("2-")) {
        authHeader = "OAuth " + m_token;
    } else {
        QUrlQuery q;
        q.addQueryItem("client_id", m_token);
        url.setQuery(q);
    }

    net->get(url, authHeader, [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QJsonArray collection = doc.object()["collection"].toArray();
        
        QVariantList mixedPlaylists;
        
        auto processSection = [&](const QJsonObject& section) {
            QJsonArray items = section["items"].toObject()["collection"].toArray();
            for (const QJsonValue& itemValue : items) {
                QJsonObject item = itemValue.toObject();
                QString kind = item["kind"].toString();
                if (kind == "playlist" || kind == "system-playlist") {
                    QVariantMap pl;
                    pl["id"] = item["id"].toVariant().toString();
                    pl["name"] = item["title"].toString();
                    pl["coverUrl"] = item["artwork_url"].toString().replace("-large", "-t500x500");
                    if (pl["coverUrl"].toString().isEmpty()) {
                        pl["coverUrl"] = item["calculated_artwork_url"].toString().replace("-large", "-t500x500");
                    }
                    pl["service"] = "SoundCloud";
                    pl["permalink_url"] = item["permalink_url"].toString();
                    mixedPlaylists.append(pl);
                }
            }
        };

        for (const QJsonValue& sectionValue : collection) {
            QJsonObject section = sectionValue.toObject();
            QString title = section["title"].toString().toLower();
            if (title.startsWith("mixed for ")) {
                processSection(section);
            }
        }

        if (mixedPlaylists.size() < 10) {
            for (const QJsonValue& sectionValue : collection) {
                QJsonObject section = sectionValue.toObject();
                if (section["title"].toString().toLower() == "made for you") {
                    processSection(section);
                }
            }
        }

        if (mixedPlaylists.isEmpty()) {
            for (const QJsonValue& sectionValue : collection) {
                QJsonObject section = sectionValue.toObject();
                QString title = section["title"].toString().toLower();
                if (title.contains("mixed") || title.contains("made for you")) {
                    processSection(section);
                }
                if (mixedPlaylists.size() >= 10) break;
            }
        }

        emit dailyMixesReady("SoundCloud", mixedPlaylists.mid(0, 12));
    });
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
    QString authHeader = "";
    if (m_token.startsWith("2-")) {
        authHeader = "OAuth " + m_token;
    } else {
        q.addQueryItem("client_id", m_token);
    }
    apiUrl.setQuery(q);

    net->get(apiUrl, authHeader, [this](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred("SoundCloud API Error: " + reply->errorString());
            return;
        }
        
        QByteArray responseData = reply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);
        QJsonObject result = doc.object();
        QString kind = result["kind"].toString();
        
        if (kind != "playlist" && kind != "system-playlist") {
            emit errorOccurred("Invalid SoundCloud URL or not a playlist");
            return;
        }

        QString name = result["title"].toString();
        QString coverUrl = result["artwork_url"].toString().replace("-large", "-t500x500");
        if (coverUrl.isEmpty()) {
            coverUrl = result["calculated_artwork_url"].toString().replace("-large", "-t500x500");
        }
        QJsonArray tracks = result["tracks"].toArray();
        if (coverUrl.isEmpty() && !tracks.isEmpty()) {
            QJsonObject firstTrack = tracks.first().toObject();
            coverUrl = firstTrack["artwork_url"].toString().replace("-large", "-t500x500");
        }
        
        QStringList allTrackIds;
        for (const QJsonValue& val : tracks) {
            QJsonObject tObj = val.toObject();
            QString tid;
            if (tObj.contains("id")) tid = tObj["id"].toVariant().toString();
            else tid = val.toVariant().toString();

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
    fetchNextPlaylistChunk(playlistName, coverUrl, remainingIds, allTracks, trackIds);
}

void SoundCloudService::fetchNextPlaylistChunk(const QString& playlistName, const QString& coverUrl, QStringList* remainingIds, QVariantList* allTracks, const QStringList& originalIds) {
    if (remainingIds->isEmpty()) {
        QVariantList sortedTracks;
        QMap<QString, QVariantMap> trackMap;
        for (const QVariant& tVal : *allTracks) {
            QVariantMap tMap = tVal.toMap();
            trackMap[tMap["id"].toString()] = tMap;
        }
        for (const QString& id : originalIds) {
            if (trackMap.contains(id)) {
                sortedTracks.append(trackMap[id]);
            }
        }
        QString finalCoverUrl = coverUrl;
        if (finalCoverUrl.isEmpty() && !sortedTracks.isEmpty()) {
            finalCoverUrl = sortedTracks.first().toMap()["coverUrl"].toString();
        }
        emit playlistImported(playlistName, finalCoverUrl, sortedTracks);
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
    QString authHeader = "";
    if (m_token.startsWith("2-")) {
        authHeader = "OAuth " + m_token;
    } else {
        q.addQueryItem("client_id", m_token);
    }
    url.setQuery(q);

    net->get(url, authHeader, [this, playlistName, coverUrl, remainingIds, allTracks, originalIds](QNetworkReply* reply) {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray data = reply->readAll();
            QJsonArray results = QJsonDocument::fromJson(data).array();
            for (const QJsonValue& val : results) {
                QJsonObject obj = val.toObject();
                TrackData track;
                track.id = obj["id"].toVariant().toString();
                track.title = obj["title"].toString();
                track.durationMs = obj["duration"].toVariant().toLongLong();
                
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
        fetchNextPlaylistChunk(playlistName, coverUrl, remainingIds, allTracks, originalIds);
    });
}

void SoundCloudService::resolveStreamUrl(const QString& trackId) {
    if (m_token.isEmpty()) return;

    if (m_trackLinks.contains(trackId)) {
        fetchStreamUrl(trackId, m_trackLinks[trackId]);
    } else {
        QUrl url("https://api-v2.soundcloud.com/tracks/" + trackId);
        QUrlQuery q;
        QString authHeader = "";
        if (m_token.startsWith("2-")) {
            authHeader = "OAuth " + m_token;
        } else {
            q.addQueryItem("client_id", m_token);
        }
        url.setQuery(q);

        net->get(url, authHeader, [this, trackId](QNetworkReply* reply) {
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
    QString authHeader = "";
    if (m_token.startsWith("2-")) {
        authHeader = "OAuth " + m_token;
    } else {
        q.addQueryItem("client_id", m_token);
    }
    url.setQuery(q);

    net->get(url, authHeader, [this, trackId](QNetworkReply* reply) {
        if (reply->error() != QNetworkReply::NoError) return;

        QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        QString streamUrl = doc.object()["url"].toString();
        emit streamUrlReady(trackId, streamUrl);
    });
}
