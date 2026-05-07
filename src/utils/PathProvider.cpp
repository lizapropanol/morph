#include "PathProvider.h"
#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QTextStream>

QString PathProvider::getConfigPath() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    if (!path.endsWith("/morph")) path = QDir::cleanPath(path + "/morph");
    return path;
}

QString PathProvider::getDataPath() {
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (!path.endsWith("/morph")) path = QDir::cleanPath(path + "/morph");
    return path;
}

QString PathProvider::getStyleFilePath() {
    return getConfigPath() + "/style.qml";
}

void PathProvider::ensureConfigExists() {
    QString configPath = getConfigPath();
    QString dataPath = getDataPath();
    
    QDir().mkpath(configPath);
    QDir().mkpath(dataPath);

    QString oldSettings = configPath + "/settings.json";
    QString newSettings = dataPath + "/settings.json";
    if (QFile::exists(oldSettings) && !QFile::exists(newSettings)) {
        QFile::rename(oldSettings, newSettings);
    }

    QString styleFile = getStyleFilePath();
    if (!QFile::exists(styleFile)) {
        QFile file(styleFile);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << "import QtQuick 2.15\n"
                << "import QtQuick.Controls 2.15\n\n"
                << "ApplicationWindow {\n"
                << "    visible: true\n"
                << "    width: 800\n"
                << "    height: 600\n"
                << "    title: \"morph\"\n"
                << "    color: \"#0a0a0a\"\n\n"
                << "    Text {\n"
                << "        anchors.centerIn: parent\n"
                << "        text: \"morph\"\n"
                << "        color: \"white\"\n"
                << "        font.pixelSize: 32\n"
                << "    }\n"
                << "}\n";
        }
    }
}
