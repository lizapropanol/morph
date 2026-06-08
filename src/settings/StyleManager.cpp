#include "StyleManager.h"
#include "../utils/PathProvider.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QTextStream>
#include <QRegularExpression>
#include <QClipboard>
#include <QGuiApplication>
#include <QBuffer>
#include <QImage>

StyleManager::StyleManager(QJsonObject* data, QObject* parent)
    : QObject(parent), m_data(data) {}

QString StyleManager::getStyleFileContent() {
    QFile file(getActiveStylePath());
    if (file.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(file.readAll());
    }
    return "";
}

QString StyleManager::getStyleContentByName(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        return QString::fromUtf8(file.readAll());
    }
    return "";
}

bool StyleManager::writeStyleFileContent(const QString& content) {
    QFile file(getActiveStylePath());
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content.toUtf8());
        file.close();
        return true;
    }
    return false;
}

bool StyleManager::writeStyleContentByName(const QString& fileName, const QString& content) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content.toUtf8());
        file.close();
        return true;
    }
    return false;
}

void StyleManager::saveTemporaryPreview(const QString& content) {
    QString path = PathProvider::getConfigPath() + "/.PreviewIde.qml";
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(content.toUtf8());
        file.close();
    }
}

bool StyleManager::importStyleFile(const QString& fileUrl) {
    QString localPath = QUrl(fileUrl).toLocalFile();
    if (localPath.isEmpty()) localPath = fileUrl;

    if (!QFile::exists(localPath)) return false;

    QFileInfo info(localPath);
    QString baseFileName = info.baseName();
    QString extension = info.completeSuffix();
    if (extension != "qml") return false;

    QString configPath = PathProvider::getConfigPath();
    QString finalFileName = baseFileName + "." + extension;
    QString dest = configPath + "/" + finalFileName;

    int counter = 1;
    while (QFile::exists(dest)) {
        finalFileName = QString("%1 (%2).%3").arg(baseFileName).arg(counter).arg(extension);
        dest = configPath + "/" + finalFileName;
        counter++;
    }

    if (QFile::copy(localPath, dest)) {
        setActiveStyleName(finalFileName);
        return true;
    }
    return false;
}

bool StyleManager::exportStyleFile(const QString& fileName, const QString& fileUrl) {
    QString localPath = QUrl(fileUrl).toLocalFile();
    if (localPath.isEmpty()) localPath = fileUrl;

    QString src = PathProvider::getConfigPath() + "/" + fileName;
    if (!QFile::exists(src)) return false;

    if (QFile::exists(localPath)) QFile::remove(localPath);

    return QFile::copy(src, localPath);
}

bool StyleManager::deleteStyleFile(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    if (QFile::exists(path)) {
        return QFile::remove(path);
    }
    return false;
}

QVariantList StyleManager::getStyleFileList() {
    QVariantList list;
    QDir dir(PathProvider::getConfigPath());
    QStringList filters;
    filters << "*.qml";
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);
    for (const QFileInfo& info : files) {
        list.append(info.fileName());
    }
    return list;
}

QString StyleManager::getActiveStyleName() {
    if (!m_data->contains("active_config")) return "style.qml";
    return (*m_data)["active_config"].toString();
}

void StyleManager::setActiveStyleName(const QString& name) {
    (*m_data)["active_config"] = name;
    emit settingsChanged();
}

QString StyleManager::getActiveStylePath() {
    return PathProvider::getConfigPath() + "/" + getActiveStyleName();
}

QString StyleManager::getStylePreview(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return "";

    QTextStream in(&file);
    QString firstLine = in.readLine();
    if (firstLine.startsWith("// MORPH_PREVIEW: ")) {
        return firstLine.mid(18);
    }
    return "";
}

void StyleManager::saveStylePreview(const QString& fileName, const QString& base64) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (!file.open(QIODevice::ReadWrite)) return;

    QString content = QString::fromUtf8(file.readAll());
    QRegularExpression re("// MORPH_PREVIEW: [A-Za-z0-9+/=]+\\n");
    content.remove(re);

    QString newContent = "// MORPH_PREVIEW: " + base64 + "\n" + content;
    file.resize(0);
    file.write(newContent.toUtf8());
    file.close();

    emit settingsChanged();
}

bool StyleManager::savePreviewFromClipboard(const QString& fileName) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    QImage image = clipboard->image();
    if (image.isNull()) return false;

    QImage scaled = image.scaled(800, 450, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    int x = (scaled.width() - 800) / 2;
    int y = (scaled.height() - 450) / 2;
    scaled = scaled.copy(x, y, 800, 450);

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    scaled.save(&buffer, "JPG", 100);
    QString base64 = ba.toBase64();

    saveStylePreview(fileName, base64);
    return true;
}

QString StyleManager::getStyleColors(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return "";

    QString content = QString::fromUtf8(file.readAll());
    QRegularExpression previewRe("// MORPH_PREVIEW: [A-Za-z0-9+/=]+\\n");
    content.remove(previewRe);

    QRegularExpression re("#[0-9A-Fa-f]{3,8}");
    QRegularExpressionMatchIterator i = re.globalMatch(content);

    QStringList colors;
    while (i.hasNext()) {
        QRegularExpressionMatch match = i.next();
        QString color = match.captured(0).toLower();
        int len = color.length();
        if (len != 4 && len != 5 && len != 7 && len != 9) continue;

        if (len == 5) color = "#" + color.mid(1);
        if (len == 9) color = "#" + color.mid(3);
        if (len == 4) {
            QString r = color.at(1);
            QString g = color.at(2);
            QString b = color.at(3);
            color = QString("#%1%1%2%2%3%3").arg(r, g, b);
        }
        if (!colors.contains(color) && color != "#000000" && color != "#ffffff" && color != "#111111" && color != "#222222") {
            colors << color;
            if (colors.size() >= 6) break;
        }
    }
    return colors.join(",");
}

QString StyleManager::getStyleConfigVersion(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return "1.0";

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("// MORPH_CONFIG_VERSION: ")) {
            return line.mid(25).trimmed();
        }
    }
    return "1.0";
}

QString StyleManager::getStyleAppVersion(const QString& fileName) {
    QString path = PathProvider::getConfigPath() + "/" + fileName;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return "Unknown";

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith("// MORPH_APP_VERSION: ")) {
            return line.mid(22).trimmed();
        }
    }
    return "Unknown";
}
