#include "FileWatcher.h"

FileWatcher::FileWatcher(const QString& path, QObject* parent) : QObject(parent) {
    watcher = new QFileSystemWatcher(this);
    watcher->addPath(path);
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &FileWatcher::fileChanged);
}

void FileWatcher::setPath(const QString& path) {
    QStringList paths = watcher->files();
    if (!paths.isEmpty()) watcher->removePaths(paths);
    watcher->addPath(path);
}
