#include "CryptoUtils.h"

const QByteArray CryptoUtils::m_key = "morph_secret_key_2024";

QString CryptoUtils::encrypt(const QString& text) {
    if (text.isEmpty()) return "";
    QByteArray data = text.toUtf8();
    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i] ^ m_key[i % m_key.size()];
    }
    return data.toBase64();
}

QString CryptoUtils::decrypt(const QString& text) {
    if (text.isEmpty()) return "";
    QByteArray data = QByteArray::fromBase64(text.toUtf8());
    for (int i = 0; i < data.size(); ++i) {
        data[i] = data[i] ^ m_key[i % m_key.size()];
    }
    return QString::fromUtf8(data);
}
