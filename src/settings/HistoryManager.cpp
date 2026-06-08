#include "HistoryManager.h"
#include <QJsonArray>

HistoryManager::HistoryManager(QJsonObject* data, QObject* parent)
    : QObject(parent), m_data(data) {}

void HistoryManager::addSearchHistory(const QVariantMap& track) {
    QJsonArray history = (*m_data)["history"].toArray();
    QJsonObject trackObj = QJsonObject::fromVariantMap(track);

    for (int i = 0; i < history.size(); ++i) {
        if (history[i].toObject()["id"].toString() == trackObj["id"].toString()) {
            return;
        }
    }

    history.insert(0, trackObj);
    if (history.size() > 20) history.removeLast();

    (*m_data)["history"] = history;
}

QVariantList HistoryManager::getSearchHistory() {
    return (*m_data)["history"].toArray().toVariantList();
}

void HistoryManager::clearSearchHistory() {
    QJsonObject obj = *m_data;
    obj.insert("history", QJsonArray());
    *m_data = obj;
}

void HistoryManager::addVibeHistory(const QString& trackId) {
    QJsonArray history = (*m_data)["vibe_history"].toArray();
    for (const QJsonValue& val : history) {
        if (val.toString() == trackId) return;
    }
    history.append(trackId);

    if (history.size() > 10000) history.removeFirst();

    (*m_data)["vibe_history"] = history;
}

bool HistoryManager::isInVibeHistory(const QString& trackId) {
    QJsonArray history = (*m_data)["vibe_history"].toArray();
    for (const QJsonValue& val : history) {
        if (val.toString() == trackId) return true;
    }
    return false;
}

void HistoryManager::clearVibeHistory() {
    (*m_data)["vibe_history"] = QJsonArray();
}
