#ifndef CHIFFREMENT_H
#define CHIFFREMENT_H

#include <QByteArray>
#include <QString>

/**
 * Authenticated encryption (AES-256-GCM) with a passphrase-derived key.
 *
 * Blob layout produced by chiffrer():
 *   version (1) | salt (16) | iv (12) | GCM tag (16) | ciphertext
 *
 * The key is PBKDF2-HMAC-SHA256(passphrase, salt). A fresh random salt and IV
 * are generated for every call, so encrypting the same text twice never
 * produces the same blob, and any modification of the blob (or a wrong
 * passphrase) is detected on decryption.
 */
class Chiffrement
{
public:
    static constexpr int kVersion = 1;
    static constexpr int kSaltSize = 16;
    static constexpr int kIvSize = 12;
    static constexpr int kTagSize = 16;
    static constexpr int kKeySize = 32;
    static constexpr int kOverhead = 1 + kSaltSize + kIvSize + kTagSize;
    static constexpr int kPbkdf2Iterations = 600000;

    /// Random 192-bit passphrase (32 URL-safe characters). Empty on RNG failure.
    static QString genererPassphrase();

    /// Encrypts @p plain. Returns an empty array on failure.
    static QByteArray chiffrer(const QByteArray &plain, const QString &passphrase);

    /// Decrypts a blob made by chiffrer(). Returns false (and leaves @p plain
    /// empty) on a wrong passphrase, tampered data or malformed input.
    static bool dechiffrer(const QByteArray &blob, const QString &passphrase, QByteArray &plain);

    /// PBKDF2-HMAC-SHA256 (32-byte output). Shared with PasswordHasher.
    static QByteArray deriveKey(const QByteArray &secret, const QByteArray &salt, int iterations);
};

#endif // CHIFFREMENT_H
