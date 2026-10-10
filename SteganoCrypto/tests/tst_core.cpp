#include <QtTest>
#include <QImage>
#include <QRandomGenerator>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QTemporaryDir>

#include "chiffrement.h"
#include "databasemanager.h"
#include "passwordhasher.h"
#include "steganography.h"

namespace {

QImage noiseImage(int w, int h, bool withAlpha = false)
{
    QImage img(w, h, QImage::Format_ARGB32);
    auto *rng = QRandomGenerator::global();
    for (int y = 0; y < h; ++y)
        for (int x = 0; x < w; ++x) {
            int a = 255;
            if (withAlpha && (x + y) % 5 == 0)
                a = (x % 2) ? 0 : 128;
            img.setPixel(x, y, qRgba(rng->bounded(256), rng->bounded(256), rng->bounded(256), a));
        }
    return img;
}

QByteArray randomBytes(int n)
{
    QByteArray b(n, 0);
    for (char &c : b)
        c = static_cast<char>(QRandomGenerator::global()->bounded(256));
    return b;
}

} // namespace

class CoreTests : public QObject
{
    Q_OBJECT

private slots:
    // ---- Chiffrement (AES-256-GCM) -------------------------------------
    void crypto_roundTrip()
    {
        const QByteArray msg = QStringLiteral("Hello, wörld – 你好 🔐").toUtf8();
        const QByteArray blob = Chiffrement::chiffrer(msg, QStringLiteral("correct horse"));
        QVERIFY(!blob.isEmpty());
        QCOMPARE(blob.size(), msg.size() + Chiffrement::kOverhead);
        QByteArray plain;
        QVERIFY(Chiffrement::dechiffrer(blob, QStringLiteral("correct horse"), plain));
        QCOMPARE(plain, msg);
    }

    void crypto_isRandomized()
    {
        const QByteArray msg("same message");
        const QByteArray a = Chiffrement::chiffrer(msg, QStringLiteral("pass-phrase"));
        const QByteArray b = Chiffrement::chiffrer(msg, QStringLiteral("pass-phrase"));
        QVERIFY(a != b); // fresh salt + IV every time (ECB would repeat)
    }

    void crypto_repeatedBlocksAreNotVisible()
    {
        // The classic ECB weakness: identical plaintext blocks -> identical ciphertext blocks.
        const QByteArray msg(64, 'A');
        const QByteArray blob = Chiffrement::chiffrer(msg, QStringLiteral("pass-phrase"));
        const QByteArray ct = blob.mid(Chiffrement::kOverhead);
        QVERIFY(ct.mid(0, 16) != ct.mid(16, 16));
    }

    void crypto_wrongPassphraseFails()
    {
        const QByteArray blob = Chiffrement::chiffrer("secret", QStringLiteral("right-pass"));
        QByteArray plain = "untouched";
        QVERIFY(!Chiffrement::dechiffrer(blob, QStringLiteral("wrong-pass"), plain));
        QVERIFY(plain.isEmpty());
    }

    void crypto_tamperingIsDetected()
    {
        const QByteArray blob = Chiffrement::chiffrer("secret message", QStringLiteral("right-pass"));
        // Flip one bit in each region (version, salt, IV, tag, ciphertext) - PBKDF2 makes
        // every attempt cost ~0.2s, so a sample is enough.
        const int positions[] = { 0, 1, 16, 17, 28, 29, 44, 45, int(blob.size()) - 1 };
        for (int i : positions) {
            QByteArray bad = blob;
            bad[i] = static_cast<char>(bad[i] ^ 0x01);
            QByteArray plain;
            QVERIFY2(!Chiffrement::dechiffrer(bad, QStringLiteral("right-pass"), plain),
                     qPrintable(QStringLiteral("flip at byte %1 not detected").arg(i)));
        }
    }

    void crypto_malformedInput()
    {
        QByteArray plain;
        QVERIFY(!Chiffrement::dechiffrer(QByteArray(), QStringLiteral("x"), plain));
        QVERIFY(!Chiffrement::dechiffrer(QByteArray(10, 'a'), QStringLiteral("x"), plain));
        QVERIFY(Chiffrement::chiffrer("data", QString()).isEmpty()); // empty passphrase refused
        const QByteArray blob = Chiffrement::chiffrer("data", QStringLiteral("pass"));
        QVERIFY(!Chiffrement::dechiffrer(blob, QString(), plain));
        QVERIFY(!Chiffrement::dechiffrer(blob.left(blob.size() - 1), QStringLiteral("pass"), plain));
    }

    void crypto_generatedPassphrase()
    {
        const QString a = Chiffrement::genererPassphrase();
        const QString b = Chiffrement::genererPassphrase();
        QCOMPARE(a.size(), 32);
        QVERIFY(a != b);
    }

    // ---- PasswordHasher -------------------------------------------------
    void password_hashAndVerify()
    {
        const QString h = PasswordHasher::hash(QStringLiteral("s3cret-Pässword"));
        QVERIFY(h.startsWith(QStringLiteral("pbkdf2-sha256$")));
        QVERIFY(!h.contains(QStringLiteral("s3cret")));
        QVERIFY(PasswordHasher::verify(QStringLiteral("s3cret-Pässword"), h));
        QVERIFY(!PasswordHasher::verify(QStringLiteral("s3cret-Password"), h));
        QVERIFY(!PasswordHasher::verify(QString(), h));
    }

    void password_saltedHashesDiffer()
    {
        QVERIFY(PasswordHasher::hash(QStringLiteral("same")) != PasswordHasher::hash(QStringLiteral("same")));
    }

    void password_rejectsMalformedAndPlaintext()
    {
        QVERIFY(!PasswordHasher::verify(QStringLiteral("pw"), QStringLiteral("pw"))); // legacy plaintext
        QVERIFY(!PasswordHasher::verify(QStringLiteral("pw"), QString()));
        QVERIFY(!PasswordHasher::verify(QStringLiteral("pw"), QStringLiteral("pbkdf2-sha256$abc$AAAA$AAAA")));
        QVERIFY(!PasswordHasher::verify(QStringLiteral("pw"), QStringLiteral("pbkdf2-sha256$999999999$AAAA$AAAA")));
        QVERIFY(!PasswordHasher::verify(QStringLiteral("pw"), QStringLiteral("md5$1$AAAA$AAAA")));
    }

    // ---- Steganography --------------------------------------------------
    void stego_roundTrip()
    {
        const QImage cover = noiseImage(64, 64);
        const QByteArray payload = randomBytes(1000);
        QImage out;
        QCOMPARE(Steganography::embed(cover, payload, out), Steganography::Status::Ok);
        QByteArray back;
        QCOMPARE(Steganography::extract(out, back), Steganography::Status::Ok);
        QCOMPARE(back, payload);
    }

    void stego_changesAtMostTheLowestBit()
    {
        const QImage cover = noiseImage(40, 40);
        QImage out;
        QCOMPARE(Steganography::embed(cover, randomBytes(300), out), Steganography::Status::Ok);
        for (int y = 0; y < 40; ++y)
            for (int x = 0; x < 40; ++x) {
                const QRgb a = cover.pixel(x, y), b = out.pixel(x, y);
                QVERIFY(qAbs(qRed(a) - qRed(b)) <= 1);
                QVERIFY(qAbs(qGreen(a) - qGreen(b)) <= 1);
                QVERIFY(qAbs(qBlue(a) - qBlue(b)) <= 1);
                QCOMPARE(qAlpha(a), qAlpha(b));
            }
    }

    void stego_coverIsNeverModified_andEmbeddingDoesNotStack()
    {
        const QImage cover = noiseImage(50, 50);
        const QImage snapshot = cover.copy();
        QImage first, second;
        QCOMPARE(Steganography::embed(cover, randomBytes(200), first), Steganography::Status::Ok);
        QCOMPARE(cover, snapshot);
        const QByteArray payload2 = randomBytes(50);
        QCOMPARE(Steganography::embed(cover, payload2, second), Steganography::Status::Ok);
        QByteArray back;
        QCOMPARE(Steganography::extract(second, back), Steganography::Status::Ok);
        QCOMPARE(back, payload2);
    }

    void stego_capacityAndTooLarge()
    {
        const QImage cover = noiseImage(32, 32);
        const qint64 cap = Steganography::capacityBytes(cover);
        QCOMPARE(cap, qint64(32 * 32 * 3 / 8 - 8));
        QImage out;
        QCOMPARE(Steganography::embed(cover, randomBytes(int(cap)), out), Steganography::Status::Ok);
        QCOMPARE(Steganography::embed(cover, randomBytes(int(cap) + 1), out), Steganography::Status::TooLarge);
        QVERIFY(out.isNull());
    }

    void stego_payloadLargerThan64KiB()
    {
        // The old 16-bit length header silently corrupted anything over 65535 bytes.
        const QImage cover = noiseImage(600, 600); // ~135 KB capacity
        const QByteArray payload = randomBytes(70000);
        QVERIFY(Steganography::capacityBytes(cover) >= payload.size());
        QImage out;
        QCOMPARE(Steganography::embed(cover, payload, out), Steganography::Status::Ok);
        QByteArray back;
        QCOMPARE(Steganography::extract(out, back), Steganography::Status::Ok);
        QCOMPARE(back, payload);
    }

    void stego_cleanImageHasNoData()
    {
        QByteArray back;
        QCOMPARE(Steganography::extract(noiseImage(64, 64), back), Steganography::Status::NoData);
        QCOMPARE(Steganography::extract(QImage(), back), Steganography::Status::InvalidImage);
    }

    void stego_emptyPayloadRejected()
    {
        QImage out;
        QCOMPARE(Steganography::embed(noiseImage(8, 8), QByteArray(), out), Steganography::Status::EmptyPayload);
        QCOMPARE(Steganography::embed(QImage(), "x", out), Steganography::Status::InvalidImage);
    }

    void stego_transparentPixelsSurvivePng()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QImage cover = noiseImage(80, 80, true);
        const QByteArray payload = randomBytes(300);
        QImage out;
        QCOMPARE(Steganography::embed(cover, payload, out), Steganography::Status::Ok);
        const QString file = dir.filePath("t.png");
        QVERIFY(out.save(file, "PNG"));
        QByteArray back;
        QCOMPARE(Steganography::extract(QImage(file), back), Steganography::Status::Ok);
        QCOMPARE(back, payload);
    }

    void stego_survivesBmp()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QImage cover = noiseImage(80, 80);
        QVERIFY(Steganography::isFullyOpaque(cover));
        const QByteArray payload = randomBytes(300);
        QImage out;
        QCOMPARE(Steganography::embed(cover, payload, out), Steganography::Status::Ok);
        const QString file = dir.filePath("t.bmp");
        QVERIFY(out.save(file, "BMP"));
        QByteArray back;
        QCOMPARE(Steganography::extract(QImage(file), back), Steganography::Status::Ok);
        QCOMPARE(back, payload);
    }

    void stego_jpegDestroysData()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QImage out;
        QCOMPARE(Steganography::embed(noiseImage(80, 80), randomBytes(300), out), Steganography::Status::Ok);
        const QString file = dir.filePath("t.jpg");
        QVERIFY(out.save(file, "JPG"));
        QByteArray back;
        QVERIFY(Steganography::extract(QImage(file), back) != Steganography::Status::Ok);
    }

    void stego_detectsTransparency()
    {
        QVERIFY(!Steganography::isFullyOpaque(noiseImage(20, 20, true)));
    }

    // ---- Full pipeline: encrypt -> hide -> save -> load -> extract -> decrypt
    void pipeline_endToEnd()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString message = QStringLiteral("Meet at midnight – bring the 🔑");
        const QString key = Chiffrement::genererPassphrase();

        const QByteArray blob = Chiffrement::chiffrer(message.toUtf8(), key);
        QImage stego;
        QCOMPARE(Steganography::embed(noiseImage(120, 120), blob, stego), Steganography::Status::Ok);
        const QString file = dir.filePath("secret.png");
        QVERIFY(stego.save(file, "PNG"));

        QByteArray extracted, plain;
        QCOMPARE(Steganography::extract(QImage(file), extracted), Steganography::Status::Ok);
        QVERIFY(Chiffrement::dechiffrer(extracted, key, plain));
        QCOMPARE(QString::fromUtf8(plain), message);
        QVERIFY(!Chiffrement::dechiffrer(extracted, QStringLiteral("not-the-key-1234"), plain));
    }

    // ---- DatabaseManager -----------------------------------------------
    void db_createVerifyAndHashedStorage()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString path = dir.filePath("users.db");
        {
            DatabaseManager db(nullptr, path);
            QVERIFY(db.isOpen());
            QCOMPARE(db.createUser("alice", "alice@example.com", "Tr0ub4dor&3"), DatabaseManager::CreateResult::Ok);
            QVERIFY(db.verifyUser("alice", "Tr0ub4dor&3"));
            QVERIFY(!db.verifyUser("alice", "wrong"));
            QVERIFY(!db.verifyUser("nobody", "Tr0ub4dor&3"));
            QVERIFY(db.verifyUser("ALICE", "Tr0ub4dor&3")); // usernames are case-insensitive
        }
        // Inspect the raw file: the password must not be in there.
        {
            QSqlDatabase raw = QSqlDatabase::addDatabase("QSQLITE", "raw-inspect");
            raw.setDatabaseName(path);
            QVERIFY(raw.open());
            QSqlQuery q(raw);
            QVERIFY(q.exec("SELECT password FROM users WHERE username='alice'"));
            QVERIFY(q.next());
            const QString stored = q.value(0).toString();
            QVERIFY(stored != "Tr0ub4dor&3");
            QVERIFY(stored.startsWith("pbkdf2-sha256$"));
            q.finish();
            raw.close();
        }
        QSqlDatabase::removeDatabase("raw-inspect");
    }

    void db_duplicatesAreRejected()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        DatabaseManager db(nullptr, dir.filePath("users.db"));
        QCOMPARE(db.createUser("bob", "bob@example.com", "password1"), DatabaseManager::CreateResult::Ok);
        QCOMPARE(db.createUser("BOB", "other@example.com", "password1"), DatabaseManager::CreateResult::UsernameTaken);
        QCOMPARE(db.createUser("robert", "Bob@Example.com", "password1"), DatabaseManager::CreateResult::EmailTaken);
        QCOMPARE(db.createUser("bob", "bob@example.com", "password1"), DatabaseManager::CreateResult::BothTaken);
    }

    void db_survivesSqlInjectionAttempts()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        DatabaseManager db(nullptr, dir.filePath("users.db"));
        QCOMPARE(db.createUser("carol", "carol@example.com", "password1"), DatabaseManager::CreateResult::Ok);
        QVERIFY(!db.verifyUser("carol' OR '1'='1", "x"));
        QVERIFY(!db.verifyUser("carol'--", "x"));
        QVERIFY(db.verifyUser("carol", "password1"));
    }
};

QTEST_GUILESS_MAIN(CoreTests)
#include "tst_core.moc"
