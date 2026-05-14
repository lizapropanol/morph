#include "CryptoUtils.h"
#include <QCryptographicHash>
#include <QFile>
#include <QSysInfo>

QByteArray CryptoUtils::getSystemKey() {
    QByteArray baseId;
    
    QFile machineId("/etc/machine-id");
    if (machineId.open(QIODevice::ReadOnly)) {
        baseId = machineId.readAll().trimmed();
        machineId.close();
    } else {
        baseId = QSysInfo::machineHostName().toUtf8() + QSysInfo::productType().toUtf8();
    }

    return QCryptographicHash::hash(baseId + "morph_ultra_salt_9999", QCryptographicHash::Sha256);
}

QString CryptoUtils::encrypt(const QString& text) {
    if (text.isEmpty()) return "";
    QByteArray data = text.toUtf8();
    QByteArray key = getSystemKey();
    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i] ^ key[i % key.size()];
    }
    return data.toBase64();
}

QString CryptoUtils::decrypt(const QString& text) {
    if (text.isEmpty()) return "";
    QByteArray data = QByteArray::fromBase64(text.toUtf8());
    QByteArray key = getSystemKey();
    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i] ^ key[i % key.size()];
    }
    return QString::fromUtf8(data);
}
