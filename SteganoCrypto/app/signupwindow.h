// signupwindow.h
#ifndef SIGNUPWINDOW_H
#define SIGNUPWINDOW_H

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QMessageBox>
#include "databasemanager.h"

class SignupWindow : public QWidget
{
    Q_OBJECT

public:
    explicit SignupWindow(DatabaseManager *dbManager, QWidget *parent = nullptr);

private slots:
    void handleSignup();
    void onInputChanged();

private:
    void setupUI();
    void applyModernStyle();
    void centerWindow();  // Added method

    // Member variables
    DatabaseManager *db;
    QLineEdit *usernameInput;
    QLineEdit *emailInput;
    QLineEdit *passwordInput;
    QLineEdit *confirmPasswordInput;
    QPushButton *signupButton;
    QPushButton *backToLoginButton;
    QLabel *logoLabel;
    QLabel *titleLabel;
    QLabel *subtitleLabel;
    QLabel *taglineLabel;
};

#endif // SIGNUPWINDOW_H
