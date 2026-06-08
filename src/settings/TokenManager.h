#ifndef TOKEN_MANAGER_H
#define TOKEN_MANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QString>

class TokenManager : public QObject {
    Q_OBJECT
public:
    explicit TokenManager(QJsonObject* data, QObject* parent = nullptr);

    void setYandexToken(const QString& token);
    QString getYandexToken();
    void setSoundCloudToken(const QString& token);
    QString getSoundCloudToken();

signals:
    void settingsChanged();

private:
    QJsonObject* m_data;
};

#endif
