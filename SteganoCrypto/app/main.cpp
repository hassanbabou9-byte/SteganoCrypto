#include "loginwindow.h"
#include "mainwindow.h"

#include <QApplication>

#include <memory>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("SteganoCrypto"));

    LoginWindow loginWindow;
    std::unique_ptr<MainWindow> mainWindow; // only created once the user has signed in

    QObject::connect(&loginWindow, &LoginWindow::loginSuccessful, [&]() {
        mainWindow = std::make_unique<MainWindow>();
        mainWindow->show();
    });

    loginWindow.show();
    return app.exec();
}
