#include "TokenManager.h"
#include "../utils/CryptoUtils.h"

TokenManager::TokenManager(QJsonObject* data, QObject* parent)
    : QObject(parent), m_data(data) {}

void TokenManager::setYandexToken(const QString& token) {
    (*m_data)["yandex_token"] = CryptoUtils::encrypt(token);
    emit settingsChanged();
}

QString TokenManager::getYandexToken() {
    return CryptoUtils::decrypt((*m_data)["yandex_token"].toString());
}

void TokenManager::setSoundCloudToken(const QString& token) {
    (*m_data)["soundcloud_token"] = CryptoUtils::encrypt(token);
    emit settingsChanged();
}

QString TokenManager::getSoundCloudToken() {
    return CryptoUtils::decrypt((*m_data)["soundcloud_token"].toString());
}
