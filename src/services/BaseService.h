#ifndef BASE_SERVICE_H
#define BASE_SERVICE_H

#include <QObject>
#include <QString>
#include <QVariantList>

class BaseService : public QObject {
    Q_OBJECT
public:
    explicit BaseService(QObject* parent = nullptr) : QObject(parent) {}
    virtual void search(const QString& query) = 0;
    virtual void resolveStreamUrl(const QString& trackId) = 0;
    virtual void getCharts() = 0;
    virtual void getWave() = 0;
    virtual void reportPlay(const QString& trackId, const QString& albumId) = 0;
    virtual void importPlaylist(const QString& url) = 0;

signals:
    void searchResultsReady(const QString& serviceName, const QVariantList& results);
    void streamUrlReady(const QString& trackId, const QString& streamUrl);
    void chartsReady(const QString& serviceName, const QVariantList& tracks);
    void waveReady(const QString& serviceName, const QVariantList& tracks);
    void playlistImported(const QString& name, const QString& coverUrl, const QVariantList& tracks);
    void errorOccurred(const QString& message);
};

#endif
