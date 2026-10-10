#ifndef PASSWORDHASHER_H
#define PASSWORDHASHER_H

#include <QString>

/**
 * Salted PBKDF2-HMAC-SHA256 password hashing.
 * Stored format:  pbkdf2-sha256$<iterations>$<salt base64>$<hash base64>
 * The iteration count is stored with the hash so it can be raised later
 * without breaking existing accounts.
 */
class PasswordHasher
{
public:
    static constexpr int kIterations = 600000;
    static constexpr int kSaltSize = 16;

    /// Returns the encoded hash, or an empty string on failure / empty password.
    static QString hash(const QString &password);

    /// Constant-time check of @p password against an encoded hash.
    /// Anything that is not a well-formed hash (e.g. legacy plaintext) returns false.
    static bool verify(const QString &password, const QString &encoded);
};

#endif // PASSWORDHASHER_H
