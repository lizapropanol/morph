#include "CryptoUtils.h"
#include <QCryptographicHash>
#include <QFile>
#include <QSysInfo>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/crypto.h>
#include <cstring>

static const unsigned char MAGIC[4] = {0x4D, 0x52, 0x50, 0x02};
static constexpr int IV_SIZE = 12;
static constexpr int TAG_SIZE = 16;
static constexpr int KEY_SIZE = 32;
static constexpr int DERIVED_KEY_LEN = 64;
static constexpr int PBKDF2_ITERATIONS = 600000;
static const unsigned char PBKDF2_SALT[] = "morph_aes256gcm_pbkdf2_v2_x9k4m";

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

QByteArray CryptoUtils::getSystemIdentifier() {
    QByteArray id;
    QFile machineId("/etc/machine-id");
    if (machineId.open(QIODevice::ReadOnly)) {
        id = machineId.readAll().trimmed();
        machineId.close();
    }
    id += QSysInfo::machineHostName().toUtf8();
    id += QSysInfo::productType().toUtf8();
    id += QSysInfo::currentCpuArchitecture().toUtf8();
    return id;
}

QByteArray CryptoUtils::derivePrimaryKey() {
    static QByteArray cached;
    if (!cached.isEmpty()) return cached;

    QByteArray id = getSystemIdentifier();
    unsigned char out[DERIVED_KEY_LEN];
    PKCS5_PBKDF2_HMAC(id.constData(), id.size(),
                       PBKDF2_SALT, sizeof(PBKDF2_SALT) - 1,
                       PBKDF2_ITERATIONS, EVP_sha512(),
                       DERIVED_KEY_LEN, out);
    cached = QByteArray(reinterpret_cast<char*>(out), DERIVED_KEY_LEN);
    OPENSSL_cleanse(out, DERIVED_KEY_LEN);
    return cached;
}

QByteArray CryptoUtils::aesGcmEncrypt(const QByteArray& plaintext, const QByteArray& key) {
    unsigned char iv[IV_SIZE];
    RAND_bytes(iv, IV_SIZE);

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr);
    EVP_EncryptInit_ex(ctx, nullptr, nullptr,
                       reinterpret_cast<const unsigned char*>(key.constData()), iv);

    int outLen = 0;
    QByteArray ciphertext(plaintext.size(), '\0');
    EVP_EncryptUpdate(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()), &outLen,
                      reinterpret_cast<const unsigned char*>(plaintext.constData()), plaintext.size());
    int totalLen = outLen;

    EVP_EncryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(ciphertext.data()) + totalLen, &outLen);
    totalLen += outLen;
    ciphertext.resize(totalLen);

    unsigned char tag[TAG_SIZE];
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, TAG_SIZE, tag);
    EVP_CIPHER_CTX_free(ctx);

    QByteArray result;
    result.reserve(IV_SIZE + TAG_SIZE + totalLen);
    result.append(reinterpret_cast<char*>(iv), IV_SIZE);
    result.append(reinterpret_cast<char*>(tag), TAG_SIZE);
    result.append(ciphertext);
    return result;
}

QByteArray CryptoUtils::aesGcmDecrypt(const QByteArray& data, const QByteArray& key) {
    if (data.size() < IV_SIZE + TAG_SIZE) return QByteArray();

    const unsigned char* ptr = reinterpret_cast<const unsigned char*>(data.constData());
    const unsigned char* iv = ptr;
    const unsigned char* tag = ptr + IV_SIZE;
    const unsigned char* ciphertext = ptr + IV_SIZE + TAG_SIZE;
    int ciphertextLen = data.size() - IV_SIZE - TAG_SIZE;

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, IV_SIZE, nullptr);
    EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                       reinterpret_cast<const unsigned char*>(key.constData()), iv);

    int outLen = 0;
    QByteArray plaintext(ciphertextLen, '\0');
    EVP_DecryptUpdate(ctx, reinterpret_cast<unsigned char*>(plaintext.data()), &outLen,
                      ciphertext, ciphertextLen);
    int totalLen = outLen;

    EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, TAG_SIZE,
                        const_cast<unsigned char*>(tag));

    int ret = EVP_DecryptFinal_ex(ctx, reinterpret_cast<unsigned char*>(plaintext.data()) + totalLen, &outLen);
    EVP_CIPHER_CTX_free(ctx);

    if (ret <= 0) return QByteArray();

    totalLen += outLen;
    plaintext.resize(totalLen);
    return plaintext;
}

QString CryptoUtils::legacyDecrypt(const QByteArray& data) {
    QByteArray key = getSystemKey();
    QByteArray decrypted = data;
    for (int i = 0; i < decrypted.size(); ++i) {
        decrypted[i] = decrypted[i] ^ key[i % key.size()];
    }
    return QString::fromUtf8(decrypted);
}

QString CryptoUtils::encrypt(const QString& text) {
    if (text.isEmpty()) return "";

    QByteArray keyMaterial = derivePrimaryKey();
    QByteArray key1 = keyMaterial.left(KEY_SIZE);
    QByteArray key2 = keyMaterial.mid(KEY_SIZE, KEY_SIZE);

    QByteArray plaintext = text.toUtf8();
    QByteArray firstLayer = aesGcmEncrypt(plaintext, key1);
    QByteArray secondLayer = aesGcmEncrypt(firstLayer, key2);

    QByteArray result;
    result.reserve(4 + secondLayer.size());
    result.append(reinterpret_cast<const char*>(MAGIC), 4);
    result.append(secondLayer);

    return result.toBase64();
}

QString CryptoUtils::decrypt(const QString& text) {
    if (text.isEmpty()) return "";

    QByteArray data = QByteArray::fromBase64(text.toUtf8());

    if (data.size() >= 4 && memcmp(data.constData(), MAGIC, 4) == 0) {
        QByteArray keyMaterial = derivePrimaryKey();
        QByteArray key1 = keyMaterial.left(KEY_SIZE);
        QByteArray key2 = keyMaterial.mid(KEY_SIZE, KEY_SIZE);

        QByteArray secondLayer = data.mid(4);
        QByteArray firstLayer = aesGcmDecrypt(secondLayer, key2);
        if (firstLayer.isEmpty()) return "";

        QByteArray plaintext = aesGcmDecrypt(firstLayer, key1);
        if (plaintext.isEmpty()) return "";

        return QString::fromUtf8(plaintext);
    }

    return legacyDecrypt(data);
}
