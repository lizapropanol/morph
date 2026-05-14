#include "SettingsManager.h"
#include "../utils/PathProvider.h"
#include "../utils/CryptoUtils.h"
#include <QDir>
#include <QSysInfo>
#include <QGuiApplication>
#include <QClipboard>

SettingsManager::SettingsManager(CacheManager* cache, QObject* parent) : QObject(parent), cache(cache) {
    m_path = PathProvider::getDataPath() + "/settings.json";
    load();
}

void SettingsManager::copyToClipboard(const QString& text) {
    QGuiApplication::clipboard()->setText(text);
}

void SettingsManager::load() {
    QFile file(m_path);
    if (file.open(QIODevice::ReadOnly)) {
        m_data = QJsonDocument::fromJson(file.readAll()).object();
        file.close();

        if (m_data.contains("cache_limit")) {
            cache->setLimit(m_data["cache_limit"].toVariant().toLongLong());
        }

        if (m_data.contains("save_track_cache")) {
            cache->setSaveTracks(m_data["save_track_cache"].toBool());
        }
        if (m_data.contains("save_cover_cache")) {
            cache->setSaveCovers(m_data["save_cover_cache"].toBool());
        }

        if (m_data.contains("playlists") && m_data["playlists"].isObject()) {
            QJsonObject playlists = m_data["playlists"].toObject();
            bool changed = false;
            for (auto it = playlists.begin(); it != playlists.end(); ++it) {
                if (it.value().isArray()) {
                    QJsonObject newPl;
                    newPl["tracks"] = it.value().toArray();
                    newPl["coverUrl"] = "";
                    playlists[it.key()] = newPl;
                    changed = true;
                }
            }
            if (changed) {
                m_data["playlists"] = playlists;
                save();
            }
        }
    }
}

void SettingsManager::save() {
    QFile file(m_path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_data).toJson());
        file.close();
    }
}

void SettingsManager::saveSession(const QVariantMap& session) {
    m_data["session"] = QJsonObject::fromVariantMap(session);
    save();
}

QVariantMap SettingsManager::loadSession() {
    return m_data["session"].toObject().toVariantMap();
}

void SettingsManager::toggleLike(const QVariantMap& track) {
    QJsonArray likes = m_data["likes"].toArray();
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
        likes.append(QJsonObject::fromVariantMap(track));
    }
    m_data["likes"] = likes;
    save();
    emit likesChanged();
}

bool SettingsManager::isLiked(const QString& trackId) {
    QJsonArray likes = m_data["likes"].toArray();
    for (int i = 0; i < likes.size(); ++i) {
        if (likes[i].toObject()["id"].toString() == trackId) return true;
    }
    return false;
}

QVariantList SettingsManager::getLikedTracks() {
    return m_data["likes"].toArray().toVariantList();
}

void SettingsManager::createPlaylist(const QString& name, const QString& coverUrl) {
    QJsonObject playlists = m_data["playlists"].toObject();
    if (!playlists.contains(name)) {
        QJsonObject playlistData;
        playlistData["coverUrl"] = coverUrl;
        playlistData["tracks"] = QJsonArray();
        playlists[name] = playlistData;
        m_data["playlists"] = playlists;
        save();
        emit playlistsChanged();
    }
}

void SettingsManager::createPlaylistWithTracks(const QString& name, const QString& coverUrl, const QVariantList& tracks) {
    QJsonObject playlists = m_data["playlists"].toObject();
    QJsonObject playlistData;
    playlistData["coverUrl"] = coverUrl;
    
    QJsonArray tracksArray;
    for (const QVariant& t : tracks) tracksArray.append(QJsonObject::fromVariantMap(t.toMap()));
    playlistData["tracks"] = tracksArray;
    
    playlists[name] = playlistData;
    m_data["playlists"] = playlists;
    save();
    emit playlistsChanged();
}

void SettingsManager::renamePlaylist(const QString& oldName, const QString& newName, const QString& coverUrl) {
    QJsonObject playlists = m_data["playlists"].toObject();
    if (playlists.contains(oldName)) {
        QJsonObject playlistData = playlists[oldName].toObject();
        playlistData["coverUrl"] = coverUrl;
        playlists.remove(oldName);
        playlists[newName] = playlistData;
        m_data["playlists"] = playlists;
        save();
        emit playlistsChanged();
    }
}

void SettingsManager::deletePlaylist(const QString& name) {
    QJsonObject playlists = m_data["playlists"].toObject();
    if (playlists.contains(name)) {
        playlists.remove(name);
        m_data["playlists"] = playlists;
        save();
        emit playlistsChanged();
    }
}

void SettingsManager::addToPlaylist(const QString& playlistName, const QVariantMap& track) {
    QJsonObject playlists = m_data["playlists"].toObject();
    QJsonObject playlistData = playlists[playlistName].toObject();
    QJsonArray tracks = playlistData["tracks"].toArray();
    tracks.append(QJsonObject::fromVariantMap(track));
    playlistData["tracks"] = tracks;
    playlists[playlistName] = playlistData;
    m_data["playlists"] = playlists;
    save();
    emit playlistsChanged();
}

QVariantMap SettingsManager::getPlaylists() {
    if (!m_data.contains("playlists") || !m_data["playlists"].isObject()) return QVariantMap();
    return m_data["playlists"].toObject().toVariantMap();
}

void SettingsManager::setYandexToken(const QString& token) {
    m_data["yandex_token"] = CryptoUtils::encrypt(token);
    save();
    emit settingsChanged();
}

QString SettingsManager::getYandexToken() {
    return CryptoUtils::decrypt(m_data["yandex_token"].toString());
}

void SettingsManager::setSoundCloudToken(const QString& token) {
    m_data["soundcloud_token"] = CryptoUtils::encrypt(token);
    save();
    emit settingsChanged();
}

QString SettingsManager::getSoundCloudToken() {
    return CryptoUtils::decrypt(m_data["soundcloud_token"].toString());
}

void SettingsManager::addSearchHistory(const QVariantMap& track) {
    QJsonArray history = m_data["history"].toArray();
    QJsonObject trackObj = QJsonObject::fromVariantMap(track);

    for (int i = 0; i < history.size(); ++i) {
        if (history[i].toObject()["id"].toString() == trackObj["id"].toString()) {
            return;
        }
    }

    history.insert(0, trackObj);
    if (history.size() > 20) history.removeLast();

    m_data["history"] = history;
    save();
}

QVariantList SettingsManager::getSearchHistory() {
    return m_data["history"].toArray().toVariantList();
}

void SettingsManager::clearSearchHistory() {
    m_data["history"] = QJsonArray();
    save();
}

void SettingsManager::setAudioQuality(const QString& quality) {
    m_data["audio_quality"] = quality;
    save();
    emit settingsChanged();
}

QString SettingsManager::getAudioQuality() {
    if (!m_data.contains("audio_quality")) return "high";
    return m_data["audio_quality"].toString();
}

void SettingsManager::setCacheLimit(qint64 bytes) {
    m_data["cache_limit"] = QString::number(bytes);
    cache->setLimit(bytes);
    save();
    emit settingsChanged();
}

qint64 SettingsManager::getCacheLimit() {
    if (!m_data.contains("cache_limit")) return 0;
    return m_data["cache_limit"].toVariant().toLongLong();
}

void SettingsManager::setSaveTrackCache(bool save) {
    m_data["save_track_cache"] = save;
    cache->setSaveTracks(save);
    this->save();
    emit settingsChanged();
}

bool SettingsManager::getSaveTrackCache() {
    if (!m_data.contains("save_track_cache")) return true;
    return m_data["save_track_cache"].toBool();
}

void SettingsManager::setSaveCoverCache(bool save) {
    m_data["save_cover_cache"] = save;
    cache->setSaveCovers(save);
    this->save();
    emit settingsChanged();
}

bool SettingsManager::getSaveCoverCache() {
    if (!m_data.contains("save_cover_cache")) return true;
    return m_data["save_cover_cache"].toBool();
}

void SettingsManager::setDiscordRpcEnabled(bool enabled) {
    m_data["discord_rpc"] = enabled;
    save();
    emit settingsChanged();
}

bool SettingsManager::getDiscordRpcEnabled() {
    if (!m_data.contains("discord_rpc")) return true;
    return m_data["discord_rpc"].toBool();
}

QVariantMap SettingsManager::getAboutInfo() {
    QVariantMap info;
    info["version"] = "0.1.0";
    info["build_date"] = QString(__DATE__);
    info["build_time"] = QString(__TIME__);
    info["qt_version"] = QString(QT_VERSION_STR);
    info["os_name"] = QSysInfo::prettyProductName();
    info["os_version"] = QSysInfo::productVersion();
    info["kernel"] = QSysInfo::kernelVersion();
    info["arch"] = QSysInfo::currentCpuArchitecture();
    
    QString buildNum = QString(__DATE__).replace(" ", "");
    buildNum += QString(__TIME__).replace(":", "");
    info["build_number"] = buildNum;
    
    return info;
}
