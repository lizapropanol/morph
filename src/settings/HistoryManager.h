#ifndef HISTORY_MANAGER_H
#define HISTORY_MANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QVariantMap>
#include <QVariantList>
#include <QString>

class HistoryManager : public QObject {
    Q_OBJECT
public:
    explicit HistoryManager(QJsonObject* data, QObject* parent = nullptr);

    void addSearchHistory(const QVariantMap& track);
    QVariantList getSearchHistory();
    void clearSearchHistory();

    void addVibeHistory(const QString& trackId);
    bool isInVibeHistory(const QString& trackId);
    void clearVibeHistory();

private:
    QJsonObject* m_data;
};

#endif
