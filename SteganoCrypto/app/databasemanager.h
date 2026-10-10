#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QObject>
#include <QSqlDatabase>
#include <QString>

/**
 * Local SQLite user store. Passwords are never stored in clear text: only a
 * salted PBKDF2 hash (see PasswordHasher) is written to the database.
 */
class DatabaseManager : public QObject
{
    Q_OBJECT

public:
    enum class CreateResult {
        Ok,
        UsernameTaken,
        EmailTaken,
        BothTaken,
        DatabaseError
    };

    /// @param dbPath  SQLite file to use; empty = the per-user app data folder.
    explicit DatabaseManager(QObject *parent = nullptr, const QString &dbPath = QString());
    ~DatabaseManager() override;

    bool isOpen() const;
    CreateResult createUser(const QString &username, const QString &email, const QString &password);
    bool verifyUser(const QString &username, const QString &password);

    QString lastError() const { return m_lastError; }

private:
    QSqlDatabase database() const;
    bool initialize(const QString &dbPath);
    void setLastError(const QString &error);

    QString m_connectionName;
    QString m_lastError;
};

#endif // DATABASEMANAGER_H
