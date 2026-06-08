#ifndef PLAYLIST_MANAGER_H
#define PLAYLIST_MANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariantMap>
#include <QVariantList>

class PlaylistManager : public QObject {
    Q_OBJECT
public:
    explicit PlaylistManager(QJsonObject* data, QObject* parent = nullptr);

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

signals:
    void likesChanged();
    void playlistsChanged();

private:
    QJsonObject* m_data;
};

#endif
