#include "passwordhasher.h"
#include "chiffrement.h"

#include <QStringList>

#include <openssl/crypto.h>
#include <openssl/rand.h>

namespace {
const QString kScheme = QStringLiteral("pbkdf2-sha256");
constexpr int kMaxIterations = 10'000'000; // sanity cap against a tampered database
}

QString PasswordHasher::hash(const QString &password)
{
    QByteArray salt(kSaltSize, 0);
    if (RAND_bytes(reinterpret_cast<unsigned char *>(salt.data()), salt.size()) != 1)
        return {};

    const QByteArray digest = Chiffrement::deriveKey(password.toUtf8(), salt, kIterations);
    if (digest.isEmpty())
        return {};

    return QStringLiteral("%1$%2$%3$%4")
        .arg(kScheme)
        .arg(kIterations)
        .arg(QString::fromLatin1(salt.toBase64()), QString::fromLatin1(digest.toBase64()));
}

bool PasswordHasher::verify(const QString &password, const QString &encoded)
{
    const QStringList parts = encoded.split(QLatin1Char('$'));
    if (parts.size() != 4 || parts.at(0) != kScheme)
        return false;

    bool ok = false;
    const int iterations = parts.at(1).toInt(&ok);
    if (!ok || iterations < 1 || iterations > kMaxIterations)
        return false;

    const QByteArray salt = QByteArray::fromBase64(parts.at(2).toLatin1());
    const QByteArray expected = QByteArray::fromBase64(parts.at(3).toLatin1());
    if (salt.isEmpty() || expected.size() != Chiffrement::kKeySize)
        return false;

    const QByteArray actual = Chiffrement::deriveKey(password.toUtf8(), salt, iterations);
    if (actual.size() != expected.size())
        return false;

    return CRYPTO_memcmp(actual.constData(), expected.constData(),
                         static_cast<size_t>(expected.size())) == 0;
}
