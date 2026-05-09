#ifndef TRACK_DATA_H
#define TRACK_DATA_H

#include <QString>
#include <QVariantMap>

struct TrackData {
    QString id;
    QString title;
    QString artist;
    QString album;
    QString coverUrl;
    QString streamUrl;
    QString service;

    QVariantMap toVariantMap() const {
        QVariantMap map;
        map["id"] = id;
        map["title"] = title;
        map["artist"] = artist;
        map["album"] = album;
        map["coverUrl"] = coverUrl;
        map["streamUrl"] = streamUrl;
        map["service"] = service;
        return map;
    }
};

#endif
