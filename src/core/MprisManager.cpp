#include "MprisManager.h"
#include <QtDBus/QDBusConnection>

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
