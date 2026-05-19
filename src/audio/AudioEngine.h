#ifndef AUDIO_ENGINE_H
#define AUDIO_ENGINE_H

#include <QObject>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QString>
#include <QTimer>

class AudioEngine : public QObject {
    Q_OBJECT
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)
    Q_PROPERTY(qint64 position READ position WRITE setPosition NOTIFY positionChanged)
    Q_PROPERTY(qint64 duration READ duration NOTIFY durationChanged)
    Q_PROPERTY(bool isPlaying READ isPlaying NOTIFY stateChanged)
    Q_PROPERTY(int bitrate READ bitrate NOTIFY bitrateChanged)

public:
    explicit AudioEngine(QObject* parent = nullptr);

    int volume() const;
    void setVolume(int volume);

    qint64 position() const;
    Q_INVOKABLE void setPosition(qint64 position);
    qint64 duration() const;
    bool isPlaying() const;
    int bitrate() const;

public slots:
    void play(const QString& url);
    void load(const QString& url);
    void pause();
    void resume();
    void stop();

signals:
    void volumeChanged();
    void positionChanged();
    void durationChanged();
    void stateChanged();
    void bitrateChanged();
    void error(const QString& errorString);
    void seeked(qint64 position);
    void finished();

private:
    QMediaPlayer* player;
    QAudioOutput* audioOutput;
    QTimer* pollTimer;
};

#endif
