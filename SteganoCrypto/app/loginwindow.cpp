#include "loginwindow.h"
#include "signupwindow.h"
#include <QVBoxLayout>
#include <QMessageBox>
#include <QDateTime>
#include <QGuiApplication>
#include <QScreen>

namespace {
constexpr int kMaxFailedAttempts = 5;
constexpr int kLockoutSeconds = 30;
}

LoginWindow::LoginWindow(QWidget *parent)
    : QWidget(parent)
{
    db = new DatabaseManager(this);
    setupUI();
    applyModernStyle();
    centerWindow();
}

void LoginWindow::setupUI()
{
    setWindowTitle("SteganoCrypto - Login");

    setMinimumSize(480, 640);
    resize(520, 700);

    // Create widgets
    logoLabel = new QLabel("🔐");
    logoLabel->setObjectName("logoLabel");
    logoLabel->setAlignment(Qt::AlignCenter);


    titleLabel = new QLabel("SteganoCrypto");
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);

    subtitleLabel = new QLabel("Secure • Hidden • Encrypted");
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    // Input fields with proper sizing
    usernameInput = new QLineEdit();
    usernameInput->setPlaceholderText("Username");
    usernameInput->setObjectName("modernInput");
    usernameInput->setMinimumHeight(50);
    usernameInput->setMaximumHeight(50);

    passwordInput = new QLineEdit();
    passwordInput->setPlaceholderText("Password");
    passwordInput->setEchoMode(QLineEdit::Password);
    passwordInput->setObjectName("modernInput");
    passwordInput->setMinimumHeight(50);
    passwordInput->setMaximumHeight(50);

    // Buttons with proper sizing
    loginButton = new QPushButton("Sign In");
    loginButton->setObjectName("primaryButton");
    loginButton->setEnabled(false);
    loginButton->setMinimumHeight(50);
    loginButton->setMaximumHeight(50);

    signupButton = new QPushButton("Create New Account");
    signupButton->setObjectName("secondaryButton");
    signupButton->setMinimumHeight(50);
    signupButton->setMaximumHeight(50);

    // Layout with controlled spacing
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setSpacing(20);
    layout->setContentsMargins(50, 50, 50, 50);

    // Add widgets with proper spacing
    layout->addStretch(1);
    layout->addWidget(logoLabel);
    layout->addSpacing(10);
    layout->addWidget(titleLabel);
    layout->addSpacing(8);
    layout->addWidget(subtitleLabel);
    layout->addSpacing(35);

    // Input fields
    layout->addWidget(usernameInput);
    layout->addSpacing(15);
    layout->addWidget(passwordInput);
    layout->addSpacing(25);

    // Buttons
    layout->addWidget(loginButton);
    layout->addSpacing(10);
    layout->addWidget(signupButton);
    layout->addStretch(1);

    // Connections
    connect(loginButton, &QPushButton::clicked, this, &LoginWindow::handleLogin);
    connect(signupButton, &QPushButton::clicked, this, &LoginWindow::showSignup);
    connect(usernameInput, &QLineEdit::textChanged, this, &LoginWindow::onInputChanged);
    connect(passwordInput, &QLineEdit::textChanged, this, &LoginWindow::onInputChanged);
    connect(passwordInput, &QLineEdit::returnPressed, this, &LoginWindow::handleLogin);
}

void LoginWindow::onInputChanged()
{
    bool hasContent = !usernameInput->text().trimmed().isEmpty() &&
                      !passwordInput->text().isEmpty();
    loginButton->setEnabled(hasContent);
}

void LoginWindow::handleLogin()
{
    if (!loginButton->isEnabled()) return;

    QString username = usernameInput->text().trimmed();
    QString password = passwordInput->text();

    // Basic validation
    if (username.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter your username.");
        usernameInput->setFocus();
        return;
    }

    if (password.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Please enter your password.");
        passwordInput->setFocus();
        return;
    }

    if (!db || !db->isOpen()) {
        QMessageBox::critical(this, "Database Error",
                              "The account database could not be opened.\n\n" + (db ? db->lastError() : QString()));
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now < m_lockedUntilMs) {
        const int secondsLeft = static_cast<int>((m_lockedUntilMs - now + 999) / 1000);
        QMessageBox::warning(this, "Too Many Attempts",
                             QString("Too many failed attempts. Please wait %1 seconds.").arg(secondsLeft));
        return;
    }

    if (verifyCredentials(username, password)) {
        m_failedAttempts = 0;
        emit loginSuccessful();
        close();
    } else {
        if (++m_failedAttempts >= kMaxFailedAttempts) {
            m_failedAttempts = 0;
            m_lockedUntilMs = now + kLockoutSeconds * 1000;
        }
        QMessageBox::warning(this, "Login Failed",
                             "Invalid username or password.\nPlease check your credentials and try again.");
        passwordInput->clear();
        passwordInput->setFocus();
    }
}

void LoginWindow::centerWindow()
{
    if (const QScreen *screen = QGuiApplication::primaryScreen())
        move(screen->availableGeometry().center() - rect().center());
}

void LoginWindow::showSignup()
{
    // Hide the login window
    hide();

    SignupWindow *signup = new SignupWindow(db, this);
    signup->setAttribute(Qt::WA_DeleteOnClose);

    // Connect to show login window when signup window closes
    connect(signup, &QWidget::destroyed, this, [this]() {
        show();
        // Clear password field for security
        passwordInput->clear();
        // Reset focus to username field
        usernameInput->setFocus();
    });

    signup->show();
}

bool LoginWindow::verifyCredentials(const QString &username, const QString &password)
{
    return db && db->verifyUser(username, password);
}

void LoginWindow::applyModernStyle()
{
    setStyleSheet(R"(
        QWidget {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                stop:0 #0f0f23,
                stop:1 #1a1a2e);
            color: white;
            font-family: 'Segoe UI', Arial, sans-serif;
        }

        QLabel {
            background: transparent;
        }

        #logoLabel {
            font-size: 42px;
            margin: 5px 0;
            padding: 5px;
            color: #fbbf24;
        }

        #titleLabel {
            font-size: 32px;
            font-weight: bold;
            color: #ffffff;
            margin: 5px 0;
            padding: 5px;
            letter-spacing: 1px;
        }

        #subtitleLabel {
            font-size: 13px;
            color: #9ca3af;
            margin: 5px 0;
            padding: 5px;
            font-style: italic;
            letter-spacing: 0.5px;
        }

        #modernInput {
            background: rgba(255, 255, 255, 0.1);
            border: 2px solid rgba(255, 255, 255, 0.2);
            border-radius: 12px;
            padding: 12px 16px;
            font-size: 14px;
            color: white;
            min-height: 20px;
        }

        #modernInput:focus {
            border: 2px solid #6366f1;
            background: rgba(255, 255, 255, 0.15);
            outline: none;
        }

        #primaryButton {
            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                stop:0 #6366f1,
                stop:1 #8b5cf6);
            border: none;
            border-radius: 12px;
            padding: 12px 16px;
            font-size: 15px;
            font-weight: bold;
            color: white;
            min-height: 20px;
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
            background: transparent;
            border: 2px solid rgba(255, 255, 255, 0.3);
            border-radius: 12px;
            padding: 12px 16px;
            font-size: 14px;
            color: #d1d5db;
            min-height: 20px;
        }

        #secondaryButton:hover {
            border-color: rgba(255, 255, 255, 0.5);
            background: rgba(255, 255, 255, 0.05);
        }
    )");
}
