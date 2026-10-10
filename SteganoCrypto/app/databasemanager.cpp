#include "databasemanager.h"
#include "passwordhasher.h"

#include <QDir>
#include <QFile>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QUuid>

DatabaseManager::DatabaseManager(QObject *parent, const QString &dbPath)
    : QObject(parent)
    , m_connectionName(QUuid::createUuid().toString(QUuid::WithoutBraces))
{
    initialize(dbPath);
}

DatabaseManager::~DatabaseManager()
{
    {
        QSqlDatabase db = QSqlDatabase::database(m_connectionName, false);
        if (db.isValid() && db.isOpen())
            db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

QSqlDatabase DatabaseManager::database() const
{
    return QSqlDatabase::database(m_connectionName, false);
}

bool DatabaseManager::isOpen() const
{
    const QSqlDatabase db = database();
    return db.isValid() && db.isOpen();
}

void DatabaseManager::setLastError(const QString &error)
{
    m_lastError = error;
}

bool DatabaseManager::initialize(const QString &dbPath)
{
    QString path = dbPath;
    if (path.isEmpty()) {
        const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QDir().mkpath(dir);
        path = dir + QStringLiteral("/steganocrypto.db");
    }

    QSqlDatabase db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    db.setDatabaseName(path);
    if (!db.open()) {
        setLastError(QStringLiteral("Failed to open database: ") + db.lastError().text());
        return false;
    }

    // Keep the account database private to the current user (no-op on Windows).
    QFile::setPermissions(path, QFileDevice::ReadOwner | QFileDevice::WriteOwner);

    QSqlQuery query(db);
    const bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS users ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " username TEXT UNIQUE NOT NULL COLLATE NOCASE,"
        " email TEXT UNIQUE NOT NULL COLLATE NOCASE,"
        " password TEXT NOT NULL,"   // holds the salted hash, never the password
        " created_at DATETIME DEFAULT CURRENT_TIMESTAMP)"));
    if (!ok) {
        setLastError(QStringLiteral("Failed to create users table: ") + query.lastError().text());
        return false;
    }
    return true;
}

DatabaseManager::CreateResult DatabaseManager::createUser(const QString &username,
                                                          const QString &email,
                                                          const QString &password)
{
    m_lastError.clear();
    if (!isOpen()) {
        setLastError(QStringLiteral("Database is not open"));
        return CreateResult::DatabaseError;
    }

    QSqlDatabase db = database();
    QSqlQuery query(db);

    query.prepare(QStringLiteral("SELECT username, email FROM users WHERE username = ? OR email = ?"));
    query.addBindValue(username);
    query.addBindValue(email);
    if (!query.exec()) {
        setLastError(QStringLiteral("Failed to check existing users: ") + query.lastError().text());
        return CreateResult::DatabaseError;
    }

    bool usernameTaken = false;
    bool emailTaken = false;
    while (query.next()) {
        usernameTaken |= query.value(0).toString().compare(username, Qt::CaseInsensitive) == 0;
        emailTaken |= query.value(1).toString().compare(email, Qt::CaseInsensitive) == 0;
    }
    if (usernameTaken && emailTaken) return CreateResult::BothTaken;
    if (usernameTaken) return CreateResult::UsernameTaken;
    if (emailTaken) return CreateResult::EmailTaken;

    const QString hash = PasswordHasher::hash(password);
    if (hash.isEmpty()) {
        setLastError(QStringLiteral("Could not hash the password"));
        return CreateResult::DatabaseError;
    }

    query.prepare(QStringLiteral("INSERT INTO users (username, email, password) VALUES (?, ?, ?)"));
    query.addBindValue(username);
    query.addBindValue(email);
    query.addBindValue(hash);
    if (!query.exec()) {
        // A concurrent insert can still trip the UNIQUE constraints.
        setLastError(QStringLiteral("Failed to insert user: ") + query.lastError().text());
        return CreateResult::DatabaseError;
    }
    return CreateResult::Ok;
}

bool DatabaseManager::verifyUser(const QString &username, const QString &password)
{
    m_lastError.clear();
    if (!isOpen()) {
        setLastError(QStringLiteral("Database is not open"));
        return false;
    }

    QSqlQuery query(database());
    query.prepare(QStringLiteral("SELECT password FROM users WHERE username = ?"));
    query.addBindValue(username);
    if (!query.exec()) {
        setLastError(QStringLiteral("Failed to verify user: ") + query.lastError().text());
        return false;
    }

    if (query.next())
        return PasswordHasher::verify(password, query.value(0).toString());

    // Unknown user: still do the same amount of work so response time does not
    // reveal whether an account exists.
    static const QString dummyHash = PasswordHasher::hash(QStringLiteral("dummy-password"));
    PasswordHasher::verify(password, dummyHash);
    return false;
}
