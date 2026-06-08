#include "SettingsManager.h"
#include "../utils/PathProvider.h"
#include <QDir>
#include <QSysInfo>

SettingsManager::SettingsManager(CacheManager* cache, QObject* parent) : QObject(parent), cache(cache) {
    m_path = PathProvider::getDataPath() + "/settings.json";
    load();

    m_playlists = new PlaylistManager(&m_data, this);
    m_tokens = new TokenManager(&m_data, this);
    m_history = new HistoryManager(&m_data, this);
    m_styles = new StyleManager(&m_data, this);

    connect(m_playlists, &PlaylistManager::likesChanged, this, &SettingsManager::likesChanged);
    connect(m_playlists, &PlaylistManager::playlistsChanged, this, &SettingsManager::playlistsChanged);
    connect(m_tokens, &TokenManager::settingsChanged, this, &SettingsManager::settingsChanged);
    connect(m_styles, &StyleManager::settingsChanged, this, &SettingsManager::settingsChanged);
}

void SettingsManager::attachHighlighter(QObject* textDocument) {
    if (!textDocument) return;
    QQuickTextDocument* doc = qobject_cast<QQuickTextDocument*>(textDocument);
    if (doc) {
        new QmlHighlighter(doc->textDocument());
    }
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
    QString tmpPath = m_path + ".tmp";
    QFile file(tmpPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(m_data).toJson());
        file.close();
        QFile::remove(m_path);
        QFile::rename(tmpPath, m_path);
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
    m_playlists->toggleLike(track);
    save();
}

bool SettingsManager::isLiked(const QString& trackId) {
    return m_playlists->isLiked(trackId);
}

QVariantList SettingsManager::getLikedTracks() {
    return m_playlists->getLikedTracks();
}

void SettingsManager::createPlaylist(const QString& name, const QString& coverUrl) {
    m_playlists->createPlaylist(name, coverUrl);
    save();
}

void SettingsManager::createPlaylistWithTracks(const QString& name, const QString& coverUrl, const QVariantList& tracks) {
    m_playlists->createPlaylistWithTracks(name, coverUrl, tracks);
    save();
}

void SettingsManager::renamePlaylist(const QString& oldName, const QString& newName, const QString& coverUrl) {
    m_playlists->renamePlaylist(oldName, newName, coverUrl);
    save();
}

void SettingsManager::deletePlaylist(const QString& name) {
    m_playlists->deletePlaylist(name);
    save();
}

void SettingsManager::addToPlaylist(const QString& playlistName, const QVariantMap& track) {
    m_playlists->addToPlaylist(playlistName, track);
    save();
}

void SettingsManager::removeFromPlaylist(const QString& playlistName, const QString& trackId) {
    m_playlists->removeFromPlaylist(playlistName, trackId);
    save();
}

QVariantMap SettingsManager::getPlaylists() {
    return m_playlists->getPlaylists();
}

void SettingsManager::setYandexToken(const QString& token) {
    m_tokens->setYandexToken(token);
    save();
}

QString SettingsManager::getYandexToken() {
    return m_tokens->getYandexToken();
}

void SettingsManager::setSoundCloudToken(const QString& token) {
    m_tokens->setSoundCloudToken(token);
    save();
}

QString SettingsManager::getSoundCloudToken() {
    return m_tokens->getSoundCloudToken();
}

void SettingsManager::addSearchHistory(const QVariantMap& track) {
    m_history->addSearchHistory(track);
    save();
}

QVariantList SettingsManager::getSearchHistory() {
    return m_history->getSearchHistory();
}

void SettingsManager::clearSearchHistory() {
    m_data.remove("history");
    save();
    emit settingsChanged();
}

void SettingsManager::addVibeHistory(const QString& trackId) {
    m_history->addVibeHistory(trackId);
    save();
}

bool SettingsManager::isInVibeHistory(const QString& trackId) {
    return m_history->isInVibeHistory(trackId);
}

void SettingsManager::clearVibeHistory() {
    m_history->clearVibeHistory();
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

void SettingsManager::setSaveTrackCache(bool enable) {
    m_data["save_track_cache"] = enable;
    cache->setSaveTracks(enable);
    this->save();
    emit settingsChanged();
}

bool SettingsManager::getSaveTrackCache() {
    if (!m_data.contains("save_track_cache")) return true;
    return m_data["save_track_cache"].toBool();
}

void SettingsManager::setSaveCoverCache(bool enable) {
    m_data["save_cover_cache"] = enable;
    cache->setSaveCovers(enable);
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
#ifdef MORPH_VERSION
    info["version"] = QString(MORPH_VERSION);
#else
    info["version"] = "unknown";
#endif
    info["build_date"] = QString(__DATE__);
    info["build_time"] = QString(__TIME__);
    info["qt_version"] = QString(QT_VERSION_STR);
    info["os_name"] = QSysInfo::prettyProductName();
    info["os_version"] = QSysInfo::productVersion();
    info["kernel"] = QSysInfo::kernelVersion();
    info["arch"] = QSysInfo::currentCpuArchitecture();

#ifdef MORPH_BUILD_NUMBER
    info["build_number"] = QString(MORPH_BUILD_NUMBER);
#else
    info["build_number"] = "unknown";
#endif

    return info;
}

QString SettingsManager::getStyleFileContent() {
    return m_styles->getStyleFileContent();
}

bool SettingsManager::writeStyleFileContent(const QString& content) {
    return m_styles->writeStyleFileContent(content);
}

QString SettingsManager::getStyleContentByName(const QString& fileName) {
    return m_styles->getStyleContentByName(fileName);
}

bool SettingsManager::writeStyleContentByName(const QString& fileName, const QString& content) {
    return m_styles->writeStyleContentByName(fileName, content);
}

void SettingsManager::saveTemporaryPreview(const QString& content) {
    m_styles->saveTemporaryPreview(content);
}

bool SettingsManager::importStyleFile(const QString& fileUrl) {
    return m_styles->importStyleFile(fileUrl);
}

bool SettingsManager::exportStyleFile(const QString& fileName, const QString& fileUrl) {
    return m_styles->exportStyleFile(fileName, fileUrl);
}

bool SettingsManager::deleteStyleFile(const QString& fileName) {
    return m_styles->deleteStyleFile(fileName);
}

QString SettingsManager::getStylePreview(const QString& fileName) {
    return m_styles->getStylePreview(fileName);
}

void SettingsManager::saveStylePreview(const QString& fileName, const QString& base64) {
    m_styles->saveStylePreview(fileName, base64);
}

bool SettingsManager::savePreviewFromClipboard(const QString& fileName) {
    return m_styles->savePreviewFromClipboard(fileName);
}

QString SettingsManager::getStyleColors(const QString& fileName) {
    return m_styles->getStyleColors(fileName);
}

QString SettingsManager::getStyleConfigVersion(const QString& fileName) {
    return m_styles->getStyleConfigVersion(fileName);
}

QString SettingsManager::getStyleAppVersion(const QString& fileName) {
    return m_styles->getStyleAppVersion(fileName);
}

QVariantList SettingsManager::getStyleFileList() {
    return m_styles->getStyleFileList();
}

QString SettingsManager::getActiveStyleName() {
    return m_styles->getActiveStyleName();
}

void SettingsManager::setActiveStyleName(const QString& name) {
    m_styles->setActiveStyleName(name);
    save();
}

QString SettingsManager::getActiveStylePath() {
    return m_styles->getActiveStylePath();
}
