#include "DiscordManager.h"
#include <QDir>
#include <QCoreApplication>
#include <QDateTime>

DiscordManager::DiscordManager(AudioEngine* audio, SettingsManager* settings, QObject* parent)
    : QObject(parent), m_audio(audio), m_settings(settings) {
    m_socket = new QLocalSocket(this);
    m_reconnectTimer = new QTimer(this);
    m_reconnectTimer->setInterval(5000);

    m_seekTimer = new QTimer(this);
    m_seekTimer->setInterval(1000);
    m_seekTimer->setSingleShot(true);

    connect(m_socket, &QLocalSocket::connected, this, &DiscordManager::onConnected);
    connect(m_socket, &QLocalSocket::disconnected, this, &DiscordManager::onDisconnected);
    connect(m_reconnectTimer, &QTimer::timeout, this, &DiscordManager::connectToDiscord);
    connect(m_seekTimer, &QTimer::timeout, this, &DiscordManager::seekUpdate);
    
    connect(m_audio, &AudioEngine::stateChanged, this, &DiscordManager::updatePresence);
    connect(m_audio, &AudioEngine::durationChanged, this, &DiscordManager::updatePresence);
    connect(m_audio, &AudioEngine::seeked, this, [this]() {
        if (!m_seekTimer->isActive()) {
            m_seekTimer->start();
        }
    });
    connect(m_settings, &SettingsManager::settingsChanged, this, &DiscordManager::onSettingsChanged);

    connectToDiscord();
}

DiscordManager::~DiscordManager() {
    if (m_socket->state() == QLocalSocket::ConnectedState) {
        QJsonObject args;
        args["pid"] = static_cast<int>(QCoreApplication::applicationPid());
        QJsonObject payload;
        payload["cmd"] = "SET_ACTIVITY";
        payload["args"] = args;
        payload["nonce"] = QString::number(QDateTime::currentMSecsSinceEpoch());
        
        QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
        QByteArray header;
        header.resize(8);
        int op = 1;
        memcpy(header.data(), &op, 4);
        uint32_t len = data.size();
        memcpy(header.data() + 4, &len, 4);

        m_socket->write(header);
        m_socket->write(data);
        m_socket->waitForBytesWritten(500);
        m_socket->close();
    }
}

void DiscordManager::connectToDiscord() {
    if (m_socket->state() != QLocalSocket::UnconnectedState || !m_settings->getDiscordRpcEnabled()) return;
    m_socket->connectToServer(getIpcPath());
}

void DiscordManager::onConnected() {
    m_reconnectTimer->stop();
    QJsonObject handshake;
    handshake["v"] = 1;
    handshake["client_id"] = m_clientId;
    sendPayload(0, handshake);
    QTimer::singleShot(500, this, &DiscordManager::updatePresence);
}

void DiscordManager::onDisconnected() {
    if (m_settings->getDiscordRpcEnabled()) {
        m_reconnectTimer->start();
    }
}

void DiscordManager::updateMetadata(const QVariantMap& track) {
    m_currentTrack = track;
    updatePresence();
}

void DiscordManager::updatePresence() {
    if (m_socket->state() != QLocalSocket::ConnectedState) return;

    if (!m_settings->getDiscordRpcEnabled() || m_currentTrack.isEmpty()) {
        QJsonObject args;
        args["pid"] = static_cast<int>(QCoreApplication::applicationPid());
        QJsonObject payload;
        payload["cmd"] = "SET_ACTIVITY";
        payload["args"] = args;
        payload["nonce"] = QString::number(QDateTime::currentMSecsSinceEpoch());
        sendPayload(1, payload);
        return;
    }

    QJsonObject activity;
    activity["type"] = 2;
    activity["details"] = m_currentTrack["title"].toString();
    activity["state"] = m_currentTrack["artist"].toString();

    QJsonObject assets;
    QString service = m_currentTrack["service"].toString().toLower();
    QString coverUrl = m_currentTrack["coverUrl"].toString();
    
    if (!coverUrl.isEmpty()) {
        assets["large_image"] = coverUrl;
        assets["large_text"] = m_currentTrack["service"].toString();
        assets["small_image"] = service == "soundcloud" ? "soundcloud" : "yandex";
        assets["small_text"] = m_currentTrack["title"].toString();
    } else {
        assets["large_image"] = service == "soundcloud" ? "soundcloud" : "yandex";
        assets["large_text"] = m_currentTrack["service"].toString();
    }
    
    activity["assets"] = assets;

    if (m_audio->isPlaying()) {
        QJsonObject timestamps;
        qint64 now = QDateTime::currentSecsSinceEpoch();
        qint64 pos = m_audio->position() / 1000;
        qint64 dur = m_audio->duration() / 1000;

        if (dur <= 0 && m_currentTrack.contains("durationMs")) {
            dur = m_currentTrack["durationMs"].toLongLong() / 1000;
        }
        
        timestamps["start"] = now - pos;
        if (dur > 0) {
            timestamps["end"] = now + (dur - pos);
        }
        activity["timestamps"] = timestamps;
    }

    QJsonObject args;
    args["pid"] = static_cast<int>(QCoreApplication::applicationPid());
    args["activity"] = activity;

    QJsonObject payload;
    payload["cmd"] = "SET_ACTIVITY";
    payload["args"] = args;
    payload["nonce"] = QString::number(QDateTime::currentMSecsSinceEpoch());

    sendPayload(1, payload);
}

void DiscordManager::sendPayload(int op, const QJsonObject& payload) {
    if (m_socket->state() != QLocalSocket::ConnectedState) return;

    QByteArray data = QJsonDocument(payload).toJson(QJsonDocument::Compact);
    QByteArray header;
    header.resize(8);
    memcpy(header.data(), &op, 4);
    uint32_t len = data.size();
    memcpy(header.data() + 4, &len, 4);

    m_socket->write(header);
    m_socket->write(data);
    m_socket->flush();
}

QString DiscordManager::getIpcPath() {
    QString runtimeDir = qgetenv("XDG_RUNTIME_DIR");
    if (runtimeDir.isEmpty()) runtimeDir = QDir::tempPath();
    
    for (int i = 0; i < 10; ++i) {
        QString path = runtimeDir + QString("/discord-ipc-%1").arg(i);
        if (QFile::exists(path)) return path;
    }

    QString flatpakPath = runtimeDir + "/app/com.discordapp.Discord/discord-ipc-0";
    if (QFile::exists(flatpakPath)) return flatpakPath;

    return "";
}

void DiscordManager::seekUpdate() {
    updatePresence();
}

void DiscordManager::onSettingsChanged() {
    if (m_settings->getDiscordRpcEnabled()) {
        m_reconnectTimer->start();
        if (m_socket->state() == QLocalSocket::ConnectedState) {
            updatePresence();
        } else {
            if (m_socket->state() != QLocalSocket::UnconnectedState) {
                m_socket->abort();
            }
            connectToDiscord();
        }
    } else {
        m_reconnectTimer->stop();
        if (m_socket->state() == QLocalSocket::ConnectedState) {
            updatePresence();
            m_socket->flush();
            m_socket->disconnectFromServer();
        } else {
            m_socket->abort();
        }
    }
}
