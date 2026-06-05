#include "YouTubeService.h"
#include "../utils/PathProvider.h"
#include <QDebug>
#include <QUrl>
#include <QFile>
#include <QTimer>

YouTubeService::YouTubeService(QObject* parent) : BaseService(parent) {}

void YouTubeService::search(const QString& query) {
    QStringList args;
    args << "--print-json" << "--flat-playlist" << "--quiet" << "--no-warnings" 
         << "--user-agent" << "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36"
         << QString("ytsearch100:%1").arg(query);
    runYtDlp(args, "search");
}

void YouTubeService::resolveStreamUrl(const QString& trackId) {
    QStringList args;
    args << "-f" << "bestaudio" << "--quiet" << "--no-warnings" 
         << "--user-agent" << "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36"
         << "--print-json" << QString("https://www.youtube.com/watch?v=%1").arg(trackId);
    runYtDlp(args, "resolve", trackId);
}

void YouTubeService::getCharts() {
    QStringList args;
    args << "--print-json" << "--flat-playlist" << "--quiet" << "--no-warnings" 
         << "--user-agent" << "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36"
         << "https://music.youtube.com/playlist?list=PL4fGSI1pDJn6jWjX9jU1A9nF56E-7K8Xm";
    runYtDlp(args, "charts");
}

void YouTubeService::getWave() {
    emit waveReady("YouTube", {});
}

void YouTubeService::getDailyMixes() {
    emit dailyMixesReady("YouTube", {});
}

void YouTubeService::reportPlay(const QString& trackId, const QString& albumId) {
    Q_UNUSED(trackId);
    Q_UNUSED(albumId);
}

void YouTubeService::importPlaylist(const QString& url) {
    QStringList args;
    args << "--print-json" << "--flat-playlist" << "--quiet" << "--no-warnings" 
         << "--user-agent" << "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/119.0.0.0 Safari/537.36"
         << url;
    runYtDlp(args, "import");
}

void YouTubeService::runYtDlp(const QStringList& args, const QString& type, const QString& id) {
    QProcess* process = new QProcess(this);
    process->setWorkingDirectory(PathProvider::getTrackCachePath());
    
    QTimer* timer = new QTimer(process);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [process]() {
        if (process->state() != QProcess::NotRunning) {
            process->kill();
        }
    });

    process->start("yt-dlp", args);
    timer->start(20000);

    connect(process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), [this, process, timer, type, id]() {
        timer->stop();
        if (process->exitStatus() == QProcess::NormalExit && process->exitCode() == 0) {
            QString output = process->readAllStandardOutput();
            if (type == "search" || type == "charts" || type == "import") {
                QVariantList results;
                QStringList lines = output.split('\n', Qt::SkipEmptyParts);
                for (const QString& line : lines) {
                    QJsonDocument doc = QJsonDocument::fromJson(line.toUtf8());
                    if (!doc.isNull()) {
                        results.append(parseYoutubeTrack(doc.object()));
                    }
                }
                if (type == "search") emit searchResultsReady("YouTube", results);
                else if (type == "charts") emit chartsReady("YouTube", results);
                else if (type == "import") emit playlistImported("Imported Playlist", "", results);
            } else if (type == "resolve") {
                QJsonDocument doc = QJsonDocument::fromJson(output.toUtf8());
                if (!doc.isNull()) {
                    QJsonObject obj = doc.object();
                    QString url = obj["url"].toString();
                    int bitrate = qRound(obj["abr"].toDouble());
                    
                    emit streamUrlReady(id, url);
                    if (bitrate > 0) emit bitrateReady(id, bitrate);
                }
            }
        } else {
        }
        process->deleteLater();
    });
}

bool YouTubeService::isTrackCached(const QString& trackId) {
    QString path = PathProvider::getTrackCachePath() + "/" + trackId + ".mp3";
    return QFile::exists(path);
}

QVariantMap YouTubeService::parseYoutubeTrack(const QJsonObject& obj) {
    QVariantMap track;
    track["id"] = obj["id"].toString();
    track["title"] = obj["title"].toString();
    
    QString artist = obj["uploader"].toString();
    if (artist.isEmpty()) artist = obj["channel"].toString();
    track["artist"] = artist;
    
    track["album"] = "YouTube Music";
    
    QJsonArray thumbnails = obj["thumbnails"].toArray();
    if (!thumbnails.isEmpty()) {
        track["coverUrl"] = thumbnails.last().toObject()["url"].toString();
    } else {
        track["coverUrl"] = obj["thumbnail"].toString();
    }
    
    track["service"] = "YouTube";
    
    QString webUrl = obj["webpage_url"].toString();
    if (webUrl.isEmpty()) webUrl = QString("https://www.youtube.com/watch?v=%1").arg(track["id"].toString());
    track["webUrl"] = webUrl;
    
    track["durationMs"] = static_cast<qint64>(obj["duration"].toDouble() * 1000);
    
    return track;
}
