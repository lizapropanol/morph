#ifndef STYLE_MANAGER_H
#define STYLE_MANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QVariantList>

class StyleManager : public QObject {
    Q_OBJECT
public:
    explicit StyleManager(QJsonObject* data, QObject* parent = nullptr);

    QString getStyleFileContent();
    QString getStyleContentByName(const QString& fileName);
    bool writeStyleFileContent(const QString& content);
    bool writeStyleContentByName(const QString& fileName, const QString& content);
    void saveTemporaryPreview(const QString& content);
    bool importStyleFile(const QString& fileUrl);
    bool exportStyleFile(const QString& fileName, const QString& fileUrl);
    bool deleteStyleFile(const QString& fileName);

    QString getStylePreview(const QString& fileName);
    void saveStylePreview(const QString& fileName, const QString& base64);
    bool savePreviewFromClipboard(const QString& fileName);
    QString getStyleColors(const QString& fileName);
    QString getStyleConfigVersion(const QString& fileName);
    QString getStyleAppVersion(const QString& fileName);

    QVariantList getStyleFileList();
    QString getActiveStyleName();
    void setActiveStyleName(const QString& name);
    QString getActiveStylePath();

signals:
    void settingsChanged();

private:
    QJsonObject* m_data;
};

#endif
