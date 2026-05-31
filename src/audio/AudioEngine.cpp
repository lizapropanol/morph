#include "AudioEngine.h"
#include <QUrl>
#include <QMediaMetaData>
#include <QProcess>
#include <QFile>

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
        if (status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::BufferedMedia) {
            emit bitrateChanged();
        }
    });

    connect(player, &QMediaPlayer::errorOccurred, [this](QMediaPlayer::Error error, const QString &errorString) {
        emit this->error(errorString);
    });
    
    connect(player, &QMediaPlayer::metaDataChanged, [this]() {
        QMediaMetaData meta = player->metaData();
        int b = meta.value(QMediaMetaData::AudioBitRate).toInt();
        if (b > 0) {
            emit bitrateChanged();
        }
    });
    
    connect(pollTimer, &QTimer::timeout, this, &AudioEngine::positionChanged);

    m_targetVolume = audioOutput->volume();

    fadeAnimation = new QVariantAnimation(this);
    fadeAnimation->setDuration(200);
    fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(fadeAnimation, &QVariantAnimation::valueChanged, [this](const QVariant &value) {
        audioOutput->setVolume(value.toFloat());
    });
    connect(fadeAnimation, &QVariantAnimation::finished, [this]() {
        if (m_isPausing) {
            player->pause();
            m_isPausing = false;
        }
    });
}

int AudioEngine::volume() const { return qRound(m_targetVolume * 100); }

void AudioEngine::setVolume(int volume) { 
    m_targetVolume = volume / 100.0f;
    if (fadeAnimation->state() != QAbstractAnimation::Running) {
        audioOutput->setVolume(m_targetVolume);
    }
}

qint64 AudioEngine::position() const { return player->position(); }

void AudioEngine::setPosition(qint64 position) { 
    player->setPosition(position); 
    emit seeked(position);
}

qint64 AudioEngine::duration() const { return player->duration(); }

bool AudioEngine::isPlaying() const { return !m_isInternalPaused && player->playbackState() == QMediaPlayer::PlayingState; }

int AudioEngine::bitrate() const {
    if (m_manualBitrate > 0) return m_manualBitrate;
    QMediaMetaData meta = player->metaData();
    
    int b = meta.value(QMediaMetaData::AudioBitRate).toInt();
    if (b <= 0) return 0;
    return (b + 500) / 1000;
}

void AudioEngine::setBitrate(int bitrate) {
    if (bitrate <= 0) return;
    m_manualBitrate = bitrate;
    emit bitrateChanged();
}

void AudioEngine::play(const QString& url) {
    m_manualBitrate = 0;
    m_isInternalPaused = false;
    m_isPausing = false;
    fadeAnimation->stop();
    audioOutput->setVolume(m_targetVolume);
    player->setSource(QUrl(url));
    player->play();
    
    if (url.startsWith("file://")) {
        QString source = QUrl(url).toLocalFile();
        if (QFile::exists(source)) {
            QProcess* proc = new QProcess(this);
            connect(proc, &QProcess::finished, [this, proc](int exitCode) {
                if (exitCode == 0) {
                    QString out = proc->readAllStandardOutput().trimmed();
                    if (!out.isEmpty() && out != "N/A") {
                        int b = (out.toLongLong() + 500) / 1000;
                        if (b > 0) {
                            this->m_manualBitrate = b;
                            emit bitrateChanged();
                        }
                    }
                }
                proc->deleteLater();
            });
            proc->start("ffprobe", {"-v", "error", "-show_entries", "format=bit_rate", "-of", "default=noprint_wrappers=1:nokey=1", source});
        }
    }
}

void AudioEngine::load(const QString& url) {
    player->setSource(QUrl(url));
}

void AudioEngine::pause() { 
    if (m_isInternalPaused || player->playbackState() != QMediaPlayer::PlayingState) return;
    
    m_isInternalPaused = true;
    m_isPausing = true;
    emit stateChanged(); 
    
    fadeAnimation->stop();
    fadeAnimation->setStartValue(audioOutput->volume());
    fadeAnimation->setEndValue(0.0f);
    fadeAnimation->start();
}

void AudioEngine::resume() { 
    m_isPausing = false;
    m_isInternalPaused = false;
    emit stateChanged(); 
    
    fadeAnimation->stop();
    player->play();
    
    fadeAnimation->setStartValue(audioOutput->volume());
    fadeAnimation->setEndValue(m_targetVolume);
    fadeAnimation->start();
}

void AudioEngine::stop() { 
    m_isInternalPaused = false;
    fadeAnimation->stop();
    player->stop(); 
    audioOutput->setVolume(m_targetVolume);
}
