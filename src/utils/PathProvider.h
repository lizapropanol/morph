#ifndef PATH_PROVIDER_H
#define PATH_PROVIDER_H

#include <QObject>
#include <QString>

class PathProvider : public QObject {
    Q_OBJECT
public:
    static PathProvider& instance();
    static QString getConfigPath();
    static QString getDataPath();
    static QString getTrackCachePath();
    static QString getCoverCachePath();
    static QString getStyleFilePath();
    void ensureConfigExists();

signals:
    void configRestored(const QString& message);

private:
    explicit PathProvider(QObject* parent = nullptr) : QObject(parent) {}
};

#endif
