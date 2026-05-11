#ifndef MPRIS_MANAGER_H
#define MPRIS_MANAGER_H

#include <QObject>
#include <QtDBus/QDBusAbstractAdaptor>
#include <QtDBus/QDBusObjectPath>
#include <QCoreApplication>
#include <QStringList>
#include <QVariantMap>
#include "../audio/AudioEngine.h"

class MprisRootAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2")
    Q_PROPERTY(bool CanQuit READ CanQuit)
    Q_PROPERTY(bool CanRaise READ CanRaise)
    Q_PROPERTY(bool HasTrackList READ HasTrackList)
    Q_PROPERTY(QString Identity READ Identity)
    Q_PROPERTY(QString DesktopEntry READ DesktopEntry)
    Q_PROPERTY(QStringList SupportedUriSchemes READ SupportedUriSchemes)
    Q_PROPERTY(QStringList SupportedMimeTypes READ SupportedMimeTypes)

public:
    explicit MprisRootAdaptor(QObject* parent) : QDBusAbstractAdaptor(parent) {}
    bool CanQuit() const { return true; }
    bool CanRaise() const { return true; }
    bool HasTrackList() const { return false; }
    QString Identity() const { return "morph"; }
    QString DesktopEntry() const { return "morph"; }
    QStringList SupportedUriSchemes() const { return {}; }
    QStringList SupportedMimeTypes() const { return {}; }

public slots:
    void Raise() {}
    void Quit() { qApp->quit(); }
};

class MprisPlayerAdaptor : public QDBusAbstractAdaptor {
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.mpris.MediaPlayer2.Player")
    Q_PROPERTY(QString PlaybackStatus READ PlaybackStatus NOTIFY playbackStatusChanged)
    Q_PROPERTY(double Volume READ Volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qint64 Position READ Position NOTIFY positionChanged)
    Q_PROPERTY(QVariantMap Metadata READ Metadata NOTIFY metadataChanged)
    Q_PROPERTY(bool CanGoNext READ CanGoNext)
    Q_PROPERTY(bool CanGoPrevious READ CanGoPrevious)
    Q_PROPERTY(bool CanPlay READ CanPlay)
    Q_PROPERTY(bool CanPause READ CanPause)
    Q_PROPERTY(bool CanSeek READ CanSeek)
    Q_PROPERTY(bool CanControl READ CanControl)

public:
    explicit MprisPlayerAdaptor(AudioEngine* audio, QObject* parent);

    QString PlaybackStatus() const { return m_audio->isPlaying() ? "Playing" : "Paused"; }
    double Volume() const { return m_audio->volume() / 100.0; }
    void setVolume(double v) { m_audio->setVolume(v * 100); }
    qint64 Position() const { return m_audio->position() * 1000; }
    QVariantMap Metadata() const { return m_metadata; }
    
    bool CanGoNext() const { return true; }
    bool CanGoPrevious() const { return true; }
    bool CanPlay() const { return true; }
    bool CanPause() const { return true; }
    bool CanSeek() const { return true; }
    bool CanControl() const { return true; }

    void updateMetadata(const QVariantMap& track);

public slots:
    void Next() { emit nextRequested(); }
    void Previous() { emit previousRequested(); }
    void Pause() { m_audio->pause(); }
    void PlayPause() { m_audio->isPlaying() ? m_audio->pause() : m_audio->resume(); }
    void Stop() { m_audio->stop(); }
    void Play() { m_audio->resume(); }
    void Seek(qint64 offset) { m_audio->setPosition(m_audio->position() + offset / 1000); }
    void SetPosition(const QDBusObjectPath& trackId, qint64 position) { Q_UNUSED(trackId); m_audio->setPosition(position / 1000); }

signals:
    void nextRequested();
    void previousRequested();
    void playbackStatusChanged();
    void volumeChanged();
    void positionChanged();
    void metadataChanged();
    void Seeked(qint64 Position);

private:
    AudioEngine* m_audio;
    QVariantMap m_metadata;
    void sendPropertiesChanged(const QString& propertyName, const QVariant& value);
};

class MprisManager : public QObject {
    Q_OBJECT
public:
    explicit MprisManager(AudioEngine* audio, QObject* parent = nullptr);
    QString currentTitle() const;
    QString currentArtist() const;

public slots:
    void updateMetadata(const QVariantMap& track) { m_player->updateMetadata(track); }

signals:
    void nextRequested();
    void previousRequested();
    void metadataChanged();

private:
    MprisRootAdaptor* m_root;
    MprisPlayerAdaptor* m_player;
};

#endif
