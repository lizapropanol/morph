#ifndef DISCORD_MANAGER_H
#define DISCORD_MANAGER_H

#include <QObject>
#include <QLocalSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QTimer>
#include <QVariantMap>
#include "../audio/AudioEngine.h"
#include "../settings/SettingsManager.h"

class DiscordManager : public QObject {
    Q_OBJECT
public:
    explicit DiscordManager(AudioEngine* audio, SettingsManager* settings, QObject* parent = nullptr);
    ~DiscordManager();

public slots:
    void updateMetadata(const QVariantMap& track);

private slots:
    void connectToDiscord();
    void onConnected();
    void onDisconnected();
    void updatePresence();

private:
    void sendPayload(int op, const QJsonObject& payload);
    QString getIpcPath();

    AudioEngine* m_audio;
    SettingsManager* m_settings;
    QLocalSocket* m_socket;
    QTimer* m_reconnectTimer;
    QVariantMap m_currentTrack;
    QString m_clientId = "1504308070342459413";
};

#endif
