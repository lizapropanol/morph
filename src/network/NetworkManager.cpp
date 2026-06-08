#include "NetworkManager.h"

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

static void applyHeaders(QNetworkRequest& request, const QMap<QString, QByteArray>& headers) {
    for (auto it = headers.begin(); it != headers.end(); ++it) {
        request.setRawHeader(it.key().toUtf8(), it.value());
    }
}

void NetworkManager::get(const QUrl& url, const QMap<QString, QByteArray>& headers, std::function<void(QNetworkReply*)> callback) {
    QNetworkRequest request(url);
    request.setTransferTimeout(10000);
    applyHeaders(request, headers);

    QNetworkReply* reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, callback, url]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QString("Network Error: %1 (%2)").arg(reply->errorString(), url.toString()));
        }
        callback(reply);
        reply->deleteLater();
    });
}

void NetworkManager::rawGet(const QUrl& url, const QMap<QString, QByteArray>& headers, std::function<void(QNetworkReply*)> callback) {
    QNetworkRequest request(url);
    request.setTransferTimeout(10000);
    applyHeaders(request, headers);

    QNetworkReply* reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [this, reply, callback, url]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QString("Network Error: %1 (%2)").arg(reply->errorString(), url.toString()));
        }
        callback(reply);
        reply->deleteLater();
    });
}

void NetworkManager::post(const QUrl& url, const QByteArray& data, const QMap<QString, QByteArray>& headers, std::function<void(QNetworkReply*)> callback) {
    QNetworkRequest request(url);
    request.setTransferTimeout(10000);
    applyHeaders(request, headers);

    QNetworkReply* reply = manager->post(request, data);
    connect(reply, &QNetworkReply::finished, [this, reply, callback, url]() {
        if (reply->error() != QNetworkReply::NoError) {
            emit errorOccurred(QString("Network Error: %1 (%2)").arg(reply->errorString(), url.toString()));
        }
        callback(reply);
        reply->deleteLater();
    });
}
