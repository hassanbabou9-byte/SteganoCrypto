#include <QtTest>
#include <QApplication>
#include <QImage>
#include <QLineEdit>
#include <QPushButton>
#include <QRandomGenerator>
#include <QTemporaryDir>
#include <QTextEdit>
#include <QTimer>

#include "mainwindow.h"

// Drives the real MainWindow (offscreen): load image -> encrypt & hide -> extract & decrypt.
class GuiTests : public QObject
{
    Q_OBJECT

    int m_dialogsSeen = 0;

    QPushButton *button(MainWindow &w, const QString &needle)
    {
        for (QPushButton *b : w.findChildren<QPushButton *>())
            if (b->text().contains(needle))
                return b;
        return nullptr;
    }

    QTextEdit *textEdit(MainWindow &w, bool readOnly)
    {
        for (QTextEdit *t : w.findChildren<QTextEdit *>())
            if (t->isReadOnly() == readOnly)
                return t;
        return nullptr;
    }

private slots:
    void initTestCase()
    {
        // Any unexpected modal dialog would block the test: close it and count it.
        auto *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this]() {
            if (QWidget *modal = QApplication::activeModalWidget()) {
                ++m_dialogsSeen;
                modal->close();
            }
        });
        timer->start(50);
    }

    void encryptThenDecrypt()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        QImage cover(120, 120, QImage::Format_RGB32);
        for (int y = 0; y < 120; ++y)
            for (int x = 0; x < 120; ++x)
                cover.setPixel(x, y, qRgb(QRandomGenerator::global()->bounded(256),
                                          QRandomGenerator::global()->bounded(256),
                                          QRandomGenerator::global()->bounded(256)));
        const QString file = dir.filePath("cover.png");
        QVERIFY(cover.save(file, "PNG"));

        MainWindow w;
        w.show();
        QVERIFY(QMetaObject::invokeMethod(&w, "loadImageFile", Q_ARG(QString, file)));

        QTextEdit *input = textEdit(w, false);
        QTextEdit *result = textEdit(w, true);
        QLineEdit *key = w.findChild<QLineEdit *>();
        QVERIFY(input && result && key);

        // 1) auto-generated passphrase
        input->setPlainText(QStringLiteral("First secret – 你好"));
        button(w, "Encrypt")->click();
        QCOMPARE(key->text().size(), 32);
        const QString firstKey = key->text();

        result->clear();
        button(w, "Decrypt")->click();
        QCOMPARE(result->toPlainText(), QStringLiteral("First secret – 你好"));

        // 2) encrypting again must start from the clean cover (nothing stacks)
        input->setPlainText(QStringLiteral("Second secret"));
        button(w, "Encrypt")->click();
        QCOMPARE(key->text(), firstKey); // same passphrase reused while the field is filled
        result->clear();
        button(w, "Decrypt")->click();
        QCOMPARE(result->toPlainText(), QStringLiteral("Second secret"));

        QCOMPARE(m_dialogsSeen, 0);
    }
};

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
        qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    GuiTests tests;
    return QTest::qExec(&tests, argc, argv);
}

#include "tst_gui.moc"
