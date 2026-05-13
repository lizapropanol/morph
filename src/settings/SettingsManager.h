#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <QObject>
#include <QVariantMap>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "../utils/CacheManager.h"

#include <QClipboard>
#include <QGuiApplication>

class SettingsManager : public QObject {
    Q_OBJECT
public:
    explicit SettingsManager(CacheManager* cache, QObject* parent = nullptr);

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
    QVariantMap getPlaylists();

    void setYandexToken(const QString& token);
    QString getYandexToken();
    void setSoundCloudToken(const QString& token);
    QString getSoundCloudToken();

    void addSearchHistory(const QVariantMap& track);
    QVariantList getSearchHistory();
    void clearSearchHistory();

    void setAudioQuality(const QString& quality);
    QString getAudioQuality();

    void setCacheLimit(qint64 bytes);
    qint64 getCacheLimit();

signals:
    void likesChanged();
    void playlistsChanged();
    void settingsChanged();

private:
    QString m_path;
    QJsonObject m_data;
    CacheManager* cache;
    void load();
    void save();
};

#endif
