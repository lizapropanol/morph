#ifndef FILE_WATCHER_H
#define FILE_WATCHER_H

#include <QObject>
#include <QFileSystemWatcher>

class FileWatcher : public QObject {
    Q_OBJECT
public:
    explicit FileWatcher(const QString& path, QObject* parent = nullptr);
    void setPath(const QString& path);

signals:
    void fileChanged();

private:
    QFileSystemWatcher* watcher;
};

#endif
