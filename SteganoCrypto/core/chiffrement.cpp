#include "chiffrement.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <memory>

namespace {

struct CtxDeleter {
    void operator()(EVP_CIPHER_CTX *ctx) const { EVP_CIPHER_CTX_free(ctx); }
};
using CtxPtr = std::unique_ptr<EVP_CIPHER_CTX, CtxDeleter>;

inline const unsigned char *u8(const QByteArray &b)
{
    return reinterpret_cast<const unsigned char *>(b.constData());
}

inline unsigned char *u8(QByteArray &b)
{
    return reinterpret_cast<unsigned char *>(b.data());
}

void wipe(QByteArray &b)
{
    if (!b.isEmpty())
        OPENSSL_cleanse(b.data(), static_cast<size_t>(b.size()));
    b.clear();
}

bool randomBytes(QByteArray &target)
{
    return RAND_bytes(u8(target), target.size()) == 1;
}

} // namespace

QByteArray Chiffrement::deriveKey(const QByteArray &secret, const QByteArray &salt, int iterations)
{
    if (secret.isEmpty() || salt.isEmpty() || iterations < 1)
        return {};

    QByteArray key(kKeySize, 0);
    const int ok = PKCS5_PBKDF2_HMAC(secret.constData(), secret.size(),
                                     u8(salt), salt.size(), iterations,
                                     EVP_sha256(), key.size(), u8(key));
    if (ok != 1)
        return {};
    return key;
}

QString Chiffrement::genererPassphrase()
{
    QByteArray raw(24, 0);
    if (!randomBytes(raw))
        return {};
    const QByteArray text = raw.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    wipe(raw);
    return QString::fromLatin1(text);
}

QByteArray Chiffrement::chiffrer(const QByteArray &plain, const QString &passphrase)
{
    const QByteArray secret = passphrase.toUtf8();
    if (secret.isEmpty())
        return {};

    QByteArray salt(kSaltSize, 0);
    QByteArray iv(kIvSize, 0);
    if (!randomBytes(salt) || !randomBytes(iv))
        return {};

    QByteArray key = deriveKey(secret, salt, kPbkdf2Iterations);
    if (key.isEmpty())
        return {};

    const unsigned char aad[1] = { static_cast<unsigned char>(kVersion) };
    QByteArray ciphertext(plain.size(), 0);
    QByteArray tag(kTagSize, 0);

    CtxPtr ctx(EVP_CIPHER_CTX_new());
    int len = 0;
    int written = 0;
    bool ok = ctx != nullptr
        && EVP_EncryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1
        && EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, kIvSize, nullptr) == 1
        && EVP_EncryptInit_ex(ctx.get(), nullptr, nullptr, u8(key), u8(iv)) == 1
        && EVP_EncryptUpdate(ctx.get(), nullptr, &len, aad, sizeof(aad)) == 1;

    if (ok && !plain.isEmpty()) {
        ok = EVP_EncryptUpdate(ctx.get(), u8(ciphertext), &written, u8(plain), plain.size()) == 1;
    }
    if (ok) {
        ok = EVP_EncryptFinal_ex(ctx.get(), u8(ciphertext) + written, &len) == 1
          && EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_GET_TAG, kTagSize, u8(tag)) == 1;
    }
    wipe(key);
    if (!ok)
        return {};

    QByteArray blob;
    blob.reserve(kOverhead + ciphertext.size());
    blob.append(static_cast<char>(kVersion));
    blob.append(salt);
    blob.append(iv);
    blob.append(tag);
    blob.append(ciphertext);
    return blob;
}

bool Chiffrement::dechiffrer(const QByteArray &blob, const QString &passphrase, QByteArray &plain)
{
    plain.clear();

    const QByteArray secret = passphrase.toUtf8();
    if (secret.isEmpty() || blob.size() < kOverhead
        || static_cast<unsigned char>(blob.at(0)) != kVersion)
        return false;

    const QByteArray salt = blob.mid(1, kSaltSize);
    const QByteArray iv = blob.mid(1 + kSaltSize, kIvSize);
    QByteArray tag = blob.mid(1 + kSaltSize + kIvSize, kTagSize);
    const QByteArray ciphertext = blob.mid(kOverhead);

    QByteArray key = deriveKey(secret, salt, kPbkdf2Iterations);
    if (key.isEmpty())
        return false;

    const unsigned char aad[1] = { static_cast<unsigned char>(kVersion) };
    QByteArray decrypted(ciphertext.size(), 0);

    CtxPtr ctx(EVP_CIPHER_CTX_new());
    int len = 0;
    int written = 0;
    bool ok = ctx != nullptr
        && EVP_DecryptInit_ex(ctx.get(), EVP_aes_256_gcm(), nullptr, nullptr, nullptr) == 1
        && EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_IVLEN, kIvSize, nullptr) == 1
        && EVP_DecryptInit_ex(ctx.get(), nullptr, nullptr, u8(key), u8(iv)) == 1
        && EVP_DecryptUpdate(ctx.get(), nullptr, &len, aad, sizeof(aad)) == 1;

    if (ok && !ciphertext.isEmpty()) {
        ok = EVP_DecryptUpdate(ctx.get(), u8(decrypted), &written, u8(ciphertext), ciphertext.size()) == 1;
    }
    if (ok) {
        ok = EVP_CIPHER_CTX_ctrl(ctx.get(), EVP_CTRL_GCM_SET_TAG, kTagSize, u8(tag)) == 1
          && EVP_DecryptFinal_ex(ctx.get(), u8(decrypted) + written, &len) > 0;
    }
    wipe(key);
    if (!ok) {
        wipe(decrypted);
        return false;
    }

    plain = decrypted;
    return true;
}
