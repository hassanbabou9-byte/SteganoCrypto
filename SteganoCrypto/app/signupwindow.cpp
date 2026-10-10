#include "signupwindow.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QSpacerItem>
#include <QApplication>
#include <QFont>
#include <QScreen>
#include <QGuiApplication>
#include <QRegularExpression>

namespace {
constexpr int kMinPasswordLength = 8;
}

SignupWindow::SignupWindow(DatabaseManager *database, QWidget *parent)
    : QWidget(parent), db(database)
{
    // Improved window attributes for better rendering
    setAttribute(Qt::WA_OpaquePaintEvent, false);
    setAttribute(Qt::WA_NoSystemBackground, false);
    setAttribute(Qt::WA_TranslucentBackground, false);

    setupUI();
    applyModernStyle();
    centerWindow();
}

void SignupWindow::setupUI()
{
    setWindowTitle("SteganoCrypto - Create Account");

    setMinimumSize(480, 720);
    resize(520, 760);

    // Set window flags for better control
    setWindowFlags(Qt::Window | Qt::WindowTitleHint | Qt::WindowCloseButtonHint |
                   Qt::WindowMinimizeButtonHint);

    // Create main container widget
    QWidget *container = new QWidget(this);
    container->setObjectName("container");

    // Create widgets with improved font handling
    QFont titleFont("Segoe UI", 24, QFont::Bold);
    QFont subtitleFont("Segoe UI", 12);
    QFont inputFont("Segoe UI", 11);
    QFont logoFont("Arial", 36);

    logoLabel = new QLabel("🔐", container);
    logoLabel->setObjectName("logoLabel");
    logoLabel->setAlignment(Qt::AlignCenter);
    logoLabel->setFont(logoFont);

    titleLabel = new QLabel("Create Account", container);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setFont(titleFont);

    subtitleLabel = new QLabel("Join SteganoCrypto today", container);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    subtitleLabel->setFont(subtitleFont);

    taglineLabel = new QLabel("Secure • Hidden • Encrypted", container);
    taglineLabel->setObjectName("taglineLabel");
    taglineLabel->setAlignment(Qt::AlignCenter);
    taglineLabel->setFont(subtitleFont);

    // Input fields with consistent sizing
    usernameInput = new QLineEdit(container);
    usernameInput->setPlaceholderText("Username");
    usernameInput->setObjectName("modernInput");
    usernameInput->setFont(inputFont);
    usernameInput->setFixedHeight(45);
    usernameInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    emailInput = new QLineEdit(container);
    emailInput->setPlaceholderText("Email Address");
    emailInput->setObjectName("modernInput");
    emailInput->setFont(inputFont);
    emailInput->setFixedHeight(45);
    emailInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    passwordInput = new QLineEdit(container);
    passwordInput->setPlaceholderText("Password");
    passwordInput->setEchoMode(QLineEdit::Password);
    passwordInput->setObjectName("modernInput");
    passwordInput->setFont(inputFont);
    passwordInput->setFixedHeight(45);
    passwordInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    confirmPasswordInput = new QLineEdit(container);
    confirmPasswordInput->setPlaceholderText("Confirm Password");
    confirmPasswordInput->setEchoMode(QLineEdit::Password);
    confirmPasswordInput->setObjectName("modernInput");
    confirmPasswordInput->setFont(inputFont);
    confirmPasswordInput->setFixedHeight(45);
    confirmPasswordInput->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Buttons with consistent sizing
    signupButton = new QPushButton("Create Account", container);
    signupButton->setObjectName("primaryButton");
    signupButton->setEnabled(false);
    signupButton->setFont(inputFont);
    signupButton->setFixedHeight(45);
    signupButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    backToLoginButton = new QPushButton("← Back to Login", container);
    backToLoginButton->setObjectName("secondaryButton");
    backToLoginButton->setFont(inputFont);
    backToLoginButton->setFixedHeight(45);
    backToLoginButton->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // Create layout for container with responsive spacing
    QVBoxLayout *containerLayout = new QVBoxLayout(container);
    containerLayout->setSpacing(15);
    containerLayout->setContentsMargins(40, 30, 40, 30);

    // Add widgets with controlled spacing
    containerLayout->addStretch(1);
    containerLayout->addWidget(logoLabel);
    containerLayout->addSpacing(15);
    containerLayout->addWidget(titleLabel);
    containerLayout->addSpacing(8);
    containerLayout->addWidget(subtitleLabel);
    containerLayout->addSpacing(5);
    containerLayout->addWidget(taglineLabel);
    containerLayout->addSpacing(25);

    // Input fields with consistent spacing
    containerLayout->addWidget(usernameInput);
    containerLayout->addSpacing(12);
    containerLayout->addWidget(emailInput);
    containerLayout->addSpacing(12);
    containerLayout->addWidget(passwordInput);
    containerLayout->addSpacing(12);
    containerLayout->addWidget(confirmPasswordInput);
    containerLayout->addSpacing(20);

    // Buttons
    containerLayout->addWidget(signupButton);
    containerLayout->addSpacing(10);
    containerLayout->addWidget(backToLoginButton);
    containerLayout->addStretch(1);

    // Main layout for the window
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(container);

    // Connections
    connect(signupButton, &QPushButton::clicked, this, &SignupWindow::handleSignup);
    connect(backToLoginButton, &QPushButton::clicked, this, &SignupWindow::close);
    connect(usernameInput, &QLineEdit::textChanged, this, &SignupWindow::onInputChanged);
    connect(emailInput, &QLineEdit::textChanged, this, &SignupWindow::onInputChanged);
    connect(passwordInput, &QLineEdit::textChanged, this, &SignupWindow::onInputChanged);
    connect(confirmPasswordInput, &QLineEdit::textChanged, this, &SignupWindow::onInputChanged);
    connect(confirmPasswordInput, &QLineEdit::returnPressed, this, &SignupWindow::handleSignup);
}

void SignupWindow::centerWindow()
{
    // Center the window on the screen using modern Qt API
    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen) {
        QRect screenGeometry = screen->availableGeometry();
        int x = (screenGeometry.width() - width()) / 2;
        int y = (screenGeometry.height() - height()) / 2;
        move(x, y);
    }
}

void SignupWindow::onInputChanged()
{
    bool hasContent = !usernameInput->text().trimmed().isEmpty() &&
                      !emailInput->text().trimmed().isEmpty() &&
                      !passwordInput->text().isEmpty() &&
                      !confirmPasswordInput->text().isEmpty();
    signupButton->setEnabled(hasContent);
}

void SignupWindow::handleSignup()
{
    if (!signupButton->isEnabled()) return;

    QString username = usernameInput->text().trimmed();
    QString email = emailInput->text().trimmed();
    QString password = passwordInput->text();
    QString confirmPassword = confirmPasswordInput->text();

    // Enhanced validation
    if (username.length() < 3) {
        QMessageBox::warning(this, "Invalid Input", "Username must be at least 3 characters long.");
        usernameInput->setFocus();
        return;
    }

    static const QRegularExpression emailPattern(
        QStringLiteral(R"(^[^@\s]+@[^@\s]+\.[^@\s]{2,}$)"));
    if (!emailPattern.match(email).hasMatch()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter a valid email address.");
        emailInput->setFocus();
        return;
    }

    if (password.length() < kMinPasswordLength) {
        QMessageBox::warning(this, "Invalid Input",
                             QString("Password must be at least %1 characters long.").arg(kMinPasswordLength));
        passwordInput->setFocus();
        return;
    }

    if (password != confirmPassword) {
        QMessageBox::warning(this, "Password Mismatch", "Passwords do not match. Please try again.");
        confirmPasswordInput->clear();
        confirmPasswordInput->setFocus();
        return;
    }

    // Check database availability
    if (!db) {
        QMessageBox::critical(this, "Database Error", "Database connection is not available.");
        return;
    }

    // Try to create user
    using Result = DatabaseManager::CreateResult;
    switch (db->createUser(username, email, password)) {
    case Result::Ok:
        QMessageBox::information(this, "Success",
                                 "Account created successfully!\nYou can now log in with your credentials.");
        close();
        return;
    case Result::UsernameTaken:
        QMessageBox::warning(this, "Registration Failed",
                             "This username is already taken. Please choose a different username.");
        usernameInput->setFocus();
        usernameInput->selectAll();
        return;
    case Result::EmailTaken:
        QMessageBox::warning(this, "Registration Failed",
                             "This email address is already registered. Please use a different email or try logging in.");
        emailInput->setFocus();
        emailInput->selectAll();
        return;
    case Result::BothTaken:
        QMessageBox::warning(this, "Registration Failed",
                             "Both the username and email are already registered. Please choose different credentials.");
        usernameInput->setFocus();
        return;
    case Result::DatabaseError:
        QMessageBox::critical(this, "Registration Failed",
                              "The account could not be created. Please restart the application and try again.\n\nDetails: "
                                  + db->lastError());
        return;
    }
}

void SignupWindow::applyModernStyle()
{
    setStyleSheet(R"(
        QWidget {
            background: #0f0f23;
            color: white;
            font-family: 'Segoe UI', 'Arial', sans-serif;
        }

        #container {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #0f0f23,
                stop:1 #1a1a2e);
            border-radius: 15px;
            margin: 8px;
            border: 1px solid rgba(255, 255, 255, 0.1);
        }

        QLabel {
            background: transparent;
        }

        #logoLabel {
            color: #fbbf24;
            margin: 8px;
            padding: 5px;
        }

        #titleLabel {
            color: #ffffff;
            margin: 8px;
            padding: 5px;
            font-weight: bold;
        }

        #subtitleLabel {
            color: #e5e7eb;
            margin: 5px;
            padding: 3px;
        }

        #taglineLabel {
            color: #9ca3af;
            margin: 5px;
            padding: 3px;
            font-style: italic;
        }

        #modernInput {
            background-color: rgba(255, 255, 255, 0.1);
            border: 2px solid rgba(255, 255, 255, 0.2);
            border-radius: 8px;
            padding: 10px 12px;
            color: white;
            selection-background-color: #6366f1;
        }

        #modernInput:focus {
            border: 2px solid #6366f1;
            background-color: rgba(255, 255, 255, 0.15);
            outline: none;
        }

        #primaryButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #6366f1,
                stop:1 #8b5cf6);
            border: none;
            border-radius: 8px;
            padding: 12px 16px;
            color: white;
            font-weight: bold;
        }

        #primaryButton:hover:enabled {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #5856eb,
                stop:1 #7c3aed);
        }

        #primaryButton:disabled {
            background: rgba(100, 100, 120, 0.5);
            color: rgba(255, 255, 255, 0.5);
        }

        #secondaryButton {
            background-color: transparent;
            border: 2px solid rgba(255, 255, 255, 0.3);
            border-radius: 8px;
            padding: 10px 16px;
            color: #d1d5db;
        }

        #secondaryButton:hover {
            border-color: rgba(255, 255, 255, 0.5);
            background-color: rgba(255, 255, 255, 0.05);
        }

    )");
}
