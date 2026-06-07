#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <QString>
#include <QByteArray>

class CryptoUtils {
public:
    static QString encrypt(const QString& text);
    static QString decrypt(const QString& text);

private:
    static QByteArray getSystemKey();
    static QByteArray getSystemIdentifier();
    static QByteArray derivePrimaryKey();
    static QByteArray aesGcmEncrypt(const QByteArray& plaintext, const QByteArray& key);
    static QByteArray aesGcmDecrypt(const QByteArray& data, const QByteArray& key);
    static QString legacyDecrypt(const QByteArray& data);
};

#endif
