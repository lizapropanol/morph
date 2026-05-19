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
        likes.insert(0, QJsonObject::fromVariantMap(track));
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
    tracks.insert(0, QJsonObject::fromVariantMap(track));
    playlistData["tracks"] = tracks;
    playlists[playlistName] = playlistData;
    m_data["playlists"] = playlists;
    save();
    emit playlistsChanged();
}

void SettingsManager::removeFromPlaylist(const QString& playlistName, const QString& trackId) {
    QJsonObject playlists = m_data["playlists"].toObject();
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

#include <QUrl>

QString SettingsManager::getStyleFileContent() {
    QFile file(getActiveStylePath());
    if (file.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(file.readAll());
    }
    return "";
}

QString SettingsManager::getStyleContentByName(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(file.readAll());
    }
    return "";
}

bool SettingsManager::writeStyleContentByName(const QString& fileName, const QString& content) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content.toUtf8());
        file.close();
        return true;
    }
    return false;
}

void SettingsManager::saveTemporaryPreview(const QString& content) {
    QString path = PathProvider::getConfigPath() + "/.PreviewIde.qml";
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content.toUtf8());
        file.close();
    }
}

bool SettingsManager::writeStyleFileContent(const QString& content) {
    QFile file(getActiveStylePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content.toUtf8());
        file.close();
        return true;
    }
    return false;
}

bool SettingsManager::importStyleFile(const QString& fileUrl) {
    QString localPath = QUrl(fileUrl).toLocalFile();
    if (localPath.isEmpty()) localPath = fileUrl;
    
    if (!QFile::exists(localPath)) return false;

    QFileInfo info(localPath);
    QString baseFileName = info.baseName();
    QString extension = info.completeSuffix();
    if (extension != "qml") return false;

    QString configPath = PathProvider::getConfigPath();
    QString finalFileName = baseFileName + "." + extension;
    QString dest = configPath + "/" + finalFileName;
    
    int counter = 1;
    while (QFile::exists(dest)) {
        finalFileName = QString("%1 (%2).%3").arg(baseFileName).arg(counter).arg(extension);
        dest = configPath + "/" + finalFileName;
        counter++;
    }
    
    if (QFile::copy(localPath, dest)) {
        setActiveStyleName(finalFileName);
        return true;
    }
    return false;
}

bool SettingsManager::exportStyleFile(const QString& fileName, const QString& fileUrl) {
    QString localPath = QUrl(fileUrl).toLocalFile();
    if (localPath.isEmpty()) localPath = fileUrl;

    QString src = PathProvider::getConfigPath() + "/" + fileName;
    if (!QFile::exists(src)) return false;

    if (QFile::exists(localPath)) QFile::remove(localPath);
    
    return QFile::copy(src, localPath);
}

bool SettingsManager::deleteStyleFile(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    if (QFile::exists(path)) {
        return QFile::remove(path);
    }
    return false;
}

QVariantList SettingsManager::getStyleFileList() {
    QVariantList list;
    QDir dir(PathProvider::getConfigPath());
    QStringList filters;
    filters << "*.qml";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    for (const QFileInfo& info : files) {
        list.append(info.fileName());
    }
    return list;
}

QString SettingsManager::getActiveStyleName() {
    if (!m_data.contains("active_config")) return "style.qml";
    return m_data["active_config"].toString();
}

void SettingsManager::setActiveStyleName(const QString& name) {
    m_data["active_config"] = name;
    save();
    emit settingsChanged();
}

QString SettingsManager::getActiveStylePath() {
    return PathProvider::getConfigPath() + "/" + getActiveStyleName();
}

#include <QRegularExpression>

QString SettingsManager::getStylePreview(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return "";
    
    QTextStream in(&file);
    QString firstLine = in.readLine();
    if (firstLine.startsWith("// MORPH_PREVIEW: ")) {
        return firstLine.mid(18);
    }
    return "";
}

void SettingsManager::saveStylePreview(const QString& fileName, const QString& base64) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (!file.open(QIODevice::ReadWrite)) return;
    
    QString content = QString::fromUtf8(file.readAll());
    QRegularExpression re("// MORPH_PREVIEW: [A-Za-z0-9+/=]+\\n");
    content.remove(re);
    
    QString newContent = "// MORPH_PREVIEW: " + base64 + "\n" + content;
    file.resize(0);
    file.write(newContent.toUtf8());
    file.close();
    
    emit settingsChanged();
}

#include <QBuffer>
#include <QImage>

bool SettingsManager::savePreviewFromClipboard(const QString& fileName) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    QImage image = clipboard->image();
    if (image.isNull()) return false;

    QImage scaled = image.scaled(800, 450, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    
    int x = (scaled.width() - 800) / 2;
    int y = (scaled.height() - 450) / 2;
    scaled = scaled.copy(x, y, 800, 450);

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    scaled.save(&buffer, "JPG", 100);
    QString base64 = ba.toBase64();

    saveStylePreview(fileName, base64);
    return true;
}

QString SettingsManager::getStyleColors(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return "";

    QString content = QString::fromUtf8(file.readAll());
    QRegularExpression previewRe("// MORPH_PREVIEW: [A-Za-z0-9+/=]+\\n");
    content.remove(previewRe);

    QRegularExpression re("#[0-9A-Fa-f]{3,8}");
    QRegularExpressionMatchIterator i = re.globalMatch(content);
    
    QStringList colors;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString color = match.captured(0).toLower();
        int len = color.length();
        if (len != 4 && len != 5 && len != 7 && len != 9) continue;

        if (len == 5) color = "#" + color.mid(1);
        if (len == 9) color = "#" + color.mid(3);
        if (len == 4) {
            QString r = color.at(1);
            QString g = color.at(2);
            QString b = color.at(3);
            color = QString("#%1%1%2%2%3%3").arg(r, g, b);
        }
        if (!colors.contains(color) && color != "#000000" && color != "#ffffff" && color != "#111111" && color != "#222222") {
            colors << color;
            if (colors.size() >= 6) break;
        }
    }
    return colors.join(",");
}

QString SettingsManager::getStyleConfigVersion(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return "1.0";
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("// MORPH_CONFIG_VERSION: ")) {
            return line.mid(25).trimmed();
        }
    }
    return "1.0";
}

QString SettingsManager::getStyleAppVersion(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return "Unknown";
    
    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("// MORPH_APP_VERSION: ")) {
            return line.mid(22).trimmed();
        }
    }
    return "Unknown";
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
