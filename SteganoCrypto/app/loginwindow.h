// loginwindow.h
#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include "databasemanager.h"

class SignupWindow;

class LoginWindow : public QWidget
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);

signals:
    void loginSuccessful();

private slots:
    void handleLogin();
    void showSignup();
    void onInputChanged();

private:
    void setupUI();
    void applyModernStyle();
    void centerWindow();
    bool verifyCredentials(const QString &username, const QString &password);

    // Member variables
    DatabaseManager *db;
    int m_failedAttempts = 0;
    qint64 m_lockedUntilMs = 0;
    QLineEdit *usernameInput;
    QLineEdit *passwordInput;
    QPushButton *loginButton;
    QPushButton *signupButton;
    QLabel *titleLabel;
    QLabel *subtitleLabel;
    QLabel *logoLabel;
};

#endif
