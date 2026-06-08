#include "PlaylistManager.h"

PlaylistManager::PlaylistManager(QJsonObject* data, QObject* parent)
    : QObject(parent), m_data(data) {}

void PlaylistManager::toggleLike(const QVariantMap& track) {
    QJsonArray likes = (*m_data)["likes"].toArray();
    QString id = track["id"].toString();
    bool found = false;
    for (int i = 0; i < likes.size(); ++i) {
        if (likes[i].toObject()["id"].toString() == id) {
            likes.removeAt(i);
            found = true;
            break;
        }
    }
    if (!found) {
        likes.insert(0, QJsonObject::fromVariantMap(track));
    }
    (*m_data)["likes"] = likes;
    emit likesChanged();
}

bool PlaylistManager::isLiked(const QString& trackId) {
    QJsonArray likes = (*m_data)["likes"].toArray();
    for (int i = 0; i < likes.size(); ++i) {
        if (likes[i].toObject()["id"].toString() == trackId) return true;
    }
    return false;
}

QVariantList PlaylistManager::getLikedTracks() {
    return (*m_data)["likes"].toArray().toVariantList();
}

void PlaylistManager::createPlaylist(const QString& name, const QString& coverUrl) {
    QJsonObject playlists = (*m_data)["playlists"].toObject();
    if (!playlists.contains(name)) {
        QJsonObject playlistData;
        playlistData["coverUrl"] = coverUrl;
        playlistData["tracks"] = QJsonArray();
        playlists[name] = playlistData;
        (*m_data)["playlists"] = playlists;
        emit playlistsChanged();
    }
}

void PlaylistManager::createPlaylistWithTracks(const QString& name, const QString& coverUrl, const QVariantList& tracks) {
    QJsonObject playlists = (*m_data)["playlists"].toObject();
    QJsonObject playlistData;
    playlistData["coverUrl"] = coverUrl;

    QJsonArray tracksArray;
    for (const QVariant& t : tracks) tracksArray.append(QJsonObject::fromVariantMap(t.toMap()));
    playlistData["tracks"] = tracksArray;

    playlists[name] = playlistData;
    (*m_data)["playlists"] = playlists;
    emit playlistsChanged();
}

void PlaylistManager::renamePlaylist(const QString& oldName, const QString& newName, const QString& coverUrl) {
    QJsonObject playlists = (*m_data)["playlists"].toObject();
    if (playlists.contains(oldName)) {
        QJsonObject playlistData = playlists[oldName].toObject();
        playlistData["coverUrl"] = coverUrl;
        playlists.remove(oldName);
        playlists[newName] = playlistData;
        (*m_data)["playlists"] = playlists;
        emit playlistsChanged();
    }
}

void PlaylistManager::deletePlaylist(const QString& name) {
    QJsonObject playlists = (*m_data)["playlists"].toObject();
    if (playlists.contains(name)) {
        playlists.remove(name);
        (*m_data)["playlists"] = playlists;
        emit playlistsChanged();
    }
}

void PlaylistManager::addToPlaylist(const QString& playlistName, const QVariantMap& track) {
    QJsonObject playlists = (*m_data)["playlists"].toObject();
    QJsonObject playlistData = playlists[playlistName].toObject();
    QJsonArray tracks = playlistData["tracks"].toArray();
    tracks.insert(0, QJsonObject::fromVariantMap(track));
    playlistData["tracks"] = tracks;
    playlists[playlistName] = playlistData;
    (*m_data)["playlists"] = playlists;
    emit playlistsChanged();
}

void PlaylistManager::removeFromPlaylist(const QString& playlistName, const QString& trackId) {
    QJsonObject playlists = (*m_data)["playlists"].toObject();
    if (!playlists.contains(playlistName)) return;

    QJsonObject playlistData = playlists[playlistName].toObject();
    QJsonArray tracks = playlistData["tracks"].toArray();

    for (int i = 0; i < tracks.size(); ++i) {
        if (tracks[i].toObject()["id"].toString() == trackId) {
            tracks.removeAt(i);
            break;
        }
    }

    playlistData["tracks"] = tracks;
    playlists[playlistName] = playlistData;
    (*m_data)["playlists"] = playlists;
    emit playlistsChanged();
}

QVariantMap PlaylistManager::getPlaylists() {
    if (!m_data->contains("playlists") || !(*m_data)["playlists"].isObject()) return QVariantMap();
    return (*m_data)["playlists"].toObject().toVariantMap();
}
