#include "MprisManager.h"
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>

MprisManager::MprisManager(AudioEngine* audio, QObject* parent) : QObject(parent) {
    m_root = new MprisRootAdaptor(this);
    m_player = new MprisPlayerAdaptor(audio, this);

    connect(m_player, &MprisPlayerAdaptor::nextRequested, this, &MprisManager::nextRequested);
    connect(m_player, &MprisPlayerAdaptor::previousRequested, this, &MprisManager::previousRequested);
    connect(m_player, &MprisPlayerAdaptor::metadataChanged, this, &MprisManager::metadataChanged);

    QDBusConnection::sessionBus().registerService("org.mpris.MediaPlayer2.morph");
    QDBusConnection::sessionBus().registerObject("/org/mpris/MediaPlayer2", this);
}

QString MprisManager::currentTitle() const {
    return m_player->Metadata().value("xesam:title").toString();
}

QString MprisManager::currentArtist() const {
    QStringList artists = m_player->Metadata().value("xesam:artist").toStringList();
    return artists.isEmpty() ? "" : artists.first();
}

MprisPlayerAdaptor::MprisPlayerAdaptor(AudioEngine* audio, QObject* parent) 
    : QDBusAbstractAdaptor(parent), m_audio(audio) {
    
    connect(m_audio, &AudioEngine::stateChanged, this, [this]() {
        emit playbackStatusChanged();
        sendPropertiesChanged("PlaybackStatus", PlaybackStatus());
    });
    connect(m_audio, &AudioEngine::volumeChanged, this, [this]() {
        emit volumeChanged();
        sendPropertiesChanged("Volume", Volume());
    });
    connect(m_audio, &AudioEngine::positionChanged, this, &MprisPlayerAdaptor::positionChanged);
    connect(m_audio, &AudioEngine::seeked, this, [this](qint64 pos) {
        emit Seeked(pos * 1000);
    });
    connect(m_audio, &AudioEngine::durationChanged, this, [this](){
        if(!m_metadata.isEmpty()) {
            m_metadata["mpris:length"] = (qint64)(m_audio->duration() * 1000);
            emit metadataChanged();
            sendPropertiesChanged("Metadata", m_metadata);
        }
    });
}

void MprisPlayerAdaptor::sendPropertiesChanged(const QString& propertyName, const QVariant& value) {
    QVariantMap changedProps;
    changedProps[propertyName] = value;
    QDBusMessage msg = QDBusMessage::createSignal("/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "PropertiesChanged");
    msg << "org.mpris.MediaPlayer2.Player" << changedProps << QStringList();
    QDBusConnection::sessionBus().send(msg);
}

void MprisPlayerAdaptor::updateMetadata(const QVariantMap& track) {
    m_metadata.clear();
    
    QString trackId = track["id"].toString();
    if (trackId.isEmpty()) trackId = "0";
    
    QString safeId = QString::number(qHash(trackId));
    m_metadata["mpris:trackid"] = QVariant::fromValue(QDBusObjectPath("/org/mpris/MediaPlayer2/Track/" + safeId));
    m_metadata["xesam:title"] = track["title"].toString();
    m_metadata["xesam:artist"] = QStringList({track["artist"].toString()});
    m_metadata["xesam:album"] = track["album"].toString().isEmpty() ? "morph" : track["album"].toString();
    m_metadata["mpris:artUrl"] = track["coverUrl"].toString();
    m_metadata["mpris:length"] = (qint64)(m_audio->duration() * 1000);
    
    emit metadataChanged();
    sendPropertiesChanged("Metadata", m_metadata);
}
