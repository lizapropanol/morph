#include "AudioEngine.h"
#include <QUrl>
#include <QMediaMetaData>

AudioEngine::AudioEngine(QObject* parent) : QObject(parent) {
    player = new QMediaPlayer(this);
    audioOutput = new QAudioOutput(this);
    player->setAudioOutput(audioOutput);
    
    pollTimer = new QTimer(this);
    pollTimer->setInterval(16);
    
    connect(audioOutput, &QAudioOutput::volumeChanged, this, &AudioEngine::volumeChanged);
    connect(player, &QMediaPlayer::durationChanged, this, &AudioEngine::durationChanged);
    connect(player, &QMediaPlayer::playbackStateChanged, [this]() {
        if (player->playbackState() == QMediaPlayer::PlayingState) pollTimer->start();
        else pollTimer->stop();
        emit stateChanged();
    });
    
    connect(player, &QMediaPlayer::mediaStatusChanged, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) emit finished();
    });

    connect(player, &QMediaPlayer::errorOccurred, [this](QMediaPlayer::Error error, const QString &errorString) {
        emit this->error(errorString);
    });
    
    connect(player, &QMediaPlayer::metaDataChanged, this, &AudioEngine::bitrateChanged);
    
    connect(pollTimer, &QTimer::timeout, this, &AudioEngine::positionChanged);
}

int AudioEngine::volume() const { return qRound(audioOutput->volume() * 100); }

void AudioEngine::setVolume(int volume) { audioOutput->setVolume(volume / 100.0f); }

qint64 AudioEngine::position() const { return player->position(); }

void AudioEngine::setPosition(qint64 position) { 
    player->setPosition(position); 
    emit seeked(position);
}

qint64 AudioEngine::duration() const { return player->duration(); }

bool AudioEngine::isPlaying() const { return player->playbackState() == QMediaPlayer::PlayingState; }

int AudioEngine::bitrate() const {
    QMediaMetaData meta = player->metaData();
    int b = meta.value(QMediaMetaData::AudioBitRate).toInt();
    return (b + 500) / 1000;
}

void AudioEngine::play(const QString& url) {
    player->setSource(QUrl(url));
    player->play();
}

void AudioEngine::load(const QString& url) {
    player->setSource(QUrl(url));
}

void AudioEngine::pause() { player->pause(); }

void AudioEngine::resume() { player->play(); }

void AudioEngine::stop() { player->stop(); }
