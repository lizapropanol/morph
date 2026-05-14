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
};

#endif
