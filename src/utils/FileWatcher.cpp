#include "FileWatcher.h"

FileWatcher::FileWatcher(const QString& path, QObject* parent) : QObject(parent) {
    watcher = new QFileSystemWatcher(this);
    if (!path.isEmpty()) watcher->addPath(path);
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &FileWatcher::fileChanged);
    connect(watcher, &QFileSystemWatcher::directoryChanged, this, &FileWatcher::directoryChanged);
}

void FileWatcher::setPath(const QString& path) {
    QStringList paths = watcher->files();
    if (!paths.isEmpty()) watcher->removePaths(paths);
    if (!path.isEmpty()) watcher->addPath(path);
}

void FileWatcher::setDirectory(const QString& path) {
    QStringList dirs = watcher->directories();
    if (!dirs.isEmpty()) watcher->removePaths(dirs);
    if (!path.isEmpty()) watcher->addPath(path);
}
