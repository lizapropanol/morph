#include "NetworkManager.h"

NetworkManager::NetworkManager(QObject* parent) : QObject(parent) {
    manager = new QNetworkAccessManager(this);
}

void NetworkManager::get(const QUrl& url, const QString& token, std::function<void(QNetworkReply*)> callback) {
    QNetworkRequest request(url);
    
    request.setHeader(QNetworkRequest::UserAgentHeader, "Yandex-Music-Desktop/5.92.1");
    request.setRawHeader("X-Yandex-Music-Client", "WindowsDesktop/5.92.1");
    request.setRawHeader("Accept", "application/json");
    request.setRawHeader("X-Retpath-Y", "https://music.yandex.ru/");
    request.setRawHeader("Accept-Language", "ru");
    
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", "OAuth " + token.toUtf8());
    }
    
    QNetworkReply* reply = manager->get(request);
    connect(reply, &QNetworkReply::finished, [reply, callback, url]() {
        callback(reply);
        reply->deleteLater();
    });
}

void NetworkManager::post(const QUrl& url, const QByteArray& data, const QString& token, std::function<void(QNetworkReply*)> callback) {
    QNetworkRequest request(url);
    
    request.setHeader(QNetworkRequest::UserAgentHeader, "Yandex-Music-Desktop/5.92.1");
    request.setRawHeader("X-Yandex-Music-Client", "WindowsDesktop/5.92.1");
    request.setRawHeader("Accept", "application/json");
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
    
    if (!token.isEmpty()) {
        request.setRawHeader("Authorization", "OAuth " + token.toUtf8());
    }
    
    QNetworkReply* reply = manager->post(request, data);
    connect(reply, &QNetworkReply::finished, [reply, callback, url]() {
        callback(reply);
        reply->deleteLater();
    });
}
