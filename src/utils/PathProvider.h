#ifndef PATH_PROVIDER_H
#define PATH_PROVIDER_H

#include <QString>

class PathProvider {
public:
    static QString getConfigPath();
    static QString getDataPath();
    static QString getTrackCachePath();
    static QString getCoverCachePath();
    static QString getStyleFilePath();
    static void ensureConfigExists();
};

#endif
