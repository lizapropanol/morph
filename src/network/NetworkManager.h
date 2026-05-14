#ifndef NETWORK_MANAGER_H
#define NETWORK_MANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class NetworkManager : public QObject {
    Q_OBJECT
public:
    explicit NetworkManager(QObject* parent = nullptr);
    void get(const QUrl& url, const QString& token, std::function<void(QNetworkReply*)> callback);
    void rawGet(const QUrl& url, std::function<void(QNetworkReply*)> callback);
    void post(const QUrl& url, const QByteArray& data, const QString& token, std::function<void(QNetworkReply*)> callback);

private:
    QNetworkAccessManager* manager;
};

#endif
