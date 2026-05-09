#include "AudioEngine.h"
#include <QUrl>

AudioEngine::AudioEngine(QObject* parent) : QObject(parent) {
    player = new QMediaPlayer(this);
    pollTimer = new QTimer(this);
    pollTimer->setInterval(16);
    
    connect(player, &QMediaPlayer::volumeChanged, this, &AudioEngine::volumeChanged);
    connect(player, &QMediaPlayer::durationChanged, this, &AudioEngine::durationChanged);
    connect(player, &QMediaPlayer::stateChanged, [this]() {
        if (player->state() == QMediaPlayer::PlayingState) pollTimer->start();
        else pollTimer->stop();
        emit stateChanged();
    });
    
    connect(player, &QMediaPlayer::mediaStatusChanged, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) emit finished();
    });
    
    connect(player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &AudioEngine::bitrateChanged);
    
    connect(pollTimer, &QTimer::timeout, this, &AudioEngine::positionChanged);
}

int AudioEngine::volume() const { return player->volume(); }

void AudioEngine::setVolume(int volume) { player->setVolume(volume); }

qint64 AudioEngine::position() const { return player->position(); }

void AudioEngine::setPosition(qint64 position) { 
    player->setPosition(position); 
    emit seeked(position);
}

qint64 AudioEngine::duration() const { return player->duration(); }

bool AudioEngine::isPlaying() const { return player->state() == QMediaPlayer::PlayingState; }

int AudioEngine::bitrate() const {
    int b = player->metaData("AudioBitRate").toInt();
    if (b <= 0) b = player->metaData("bitrate").toInt();
    return (b + 500) / 1000;
}

void AudioEngine::play(const QString& url) {
    player->setMedia(QUrl(url));
    player->play();
}

void AudioEngine::pause() { player->pause(); }

void AudioEngine::resume() { player->play(); }

void AudioEngine::stop() { player->stop(); }
