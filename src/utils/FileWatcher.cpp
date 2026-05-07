#include "FileWatcher.h"

FileWatcher::FileWatcher(const QString& path, QObject* parent) : QObject(parent) {
    watcher = new QFileSystemWatcher(this);
    watcher->addPath(path);
    connect(watcher, &QFileSystemWatcher::fileChanged, this, &FileWatcher::fileChanged);
}
