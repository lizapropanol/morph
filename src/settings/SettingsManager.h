#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QClipboard>
#include <QGuiApplication>
#include <QQuickTextDocument>
#include "../utils/CacheManager.h"
#include "../utils/QmlHighlighter.h"
#include "PlaylistManager.h"
#include "TokenManager.h"
#include "HistoryManager.h"
#include "StyleManager.h"

class SettingsManager : public QObject {
    Q_OBJECT
public:
    explicit SettingsManager(CacheManager* cache, QObject* parent = nullptr);

    Q_INVOKABLE void attachHighlighter(QObject* textDocument);

public slots:
    void copyToClipboard(const QString& text);
    void saveSession(const QVariantMap& session);
    QVariantMap loadSession();

    void toggleLike(const QVariantMap& track);
    bool isLiked(const QString& trackId);
    QVariantList getLikedTracks();

    void createPlaylist(const QString& name, const QString& coverUrl = "");
    void createPlaylistWithTracks(const QString& name, const QString& coverUrl, const QVariantList& tracks);
    void renamePlaylist(const QString& oldName, const QString& newName, const QString& coverUrl);
    void deletePlaylist(const QString& name);
    void addToPlaylist(const QString& playlistName, const QVariantMap& track);
    void removeFromPlaylist(const QString& playlistName, const QString& trackId);
    QVariantMap getPlaylists();

    void setYandexToken(const QString& token);
    QString getYandexToken();
    void setSoundCloudToken(const QString& token);
    QString getSoundCloudToken();

    void addSearchHistory(const QVariantMap& track);
    QVariantList getSearchHistory();
    void clearSearchHistory();

    void addVibeHistory(const QString& trackId);
    bool isInVibeHistory(const QString& trackId);
    void clearVibeHistory();

    void setAudioQuality(const QString& quality);
    QString getAudioQuality();

    void setCacheLimit(qint64 bytes);
    qint64 getCacheLimit();

    void setSaveTrackCache(bool enable);
    bool getSaveTrackCache();
    void setSaveCoverCache(bool enable);
    bool getSaveCoverCache();

    void setDiscordRpcEnabled(bool enabled);
    bool getDiscordRpcEnabled();

    Q_INVOKABLE QVariantMap getAboutInfo();

    Q_INVOKABLE QString getStyleFileContent();
    Q_INVOKABLE bool writeStyleFileContent(const QString& content);
    Q_INVOKABLE QString getStyleContentByName(const QString& fileName);
    Q_INVOKABLE bool writeStyleContentByName(const QString& fileName, const QString& content);
    Q_INVOKABLE void saveTemporaryPreview(const QString& content);
    Q_INVOKABLE bool importStyleFile(const QString& fileUrl);
    Q_INVOKABLE bool exportStyleFile(const QString& fileName, const QString& fileUrl);
    Q_INVOKABLE bool deleteStyleFile(const QString& fileName);

    Q_INVOKABLE QString getStylePreview(const QString& fileName);    void saveStylePreview(const QString& fileName, const QString& base64);
    Q_INVOKABLE bool savePreviewFromClipboard(const QString& fileName);
    Q_INVOKABLE QString getStyleColors(const QString& fileName);
    Q_INVOKABLE QString getStyleConfigVersion(const QString& fileName);
    Q_INVOKABLE QString getStyleAppVersion(const QString& fileName);

    Q_INVOKABLE QVariantList getStyleFileList();
    Q_INVOKABLE QString getActiveStyleName();
    Q_INVOKABLE void setActiveStyleName(const QString& name);
    QString getActiveStylePath();

signals:
    void likesChanged();
    void playlistsChanged();
    void settingsChanged();

private:
    QString m_path;
    QJsonObject m_data;
    CacheManager* cache;
    PlaylistManager* m_playlists;
    TokenManager* m_tokens;
    HistoryManager* m_history;
    StyleManager* m_styles;
    void load();
    void save();
};

#endif
