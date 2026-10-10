#include "mainwindow.h"

#include "chiffrement.h"
#include "steganography.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QMimeData>
#include <QPainter>
#include <QPushButton>
#include <QScrollArea>
#include <QSplitter>
#include <QStatusBar>
#include <QTextEdit>
#include <QVBoxLayout>

namespace {

constexpr int kMinPassphraseLength = 8;

/// Shows a wait cursor while a (potentially slow) operation runs.
struct BusyCursor {
    BusyCursor() { QApplication::setOverrideCursor(Qt::WaitCursor); }
    ~BusyCursor() { QApplication::restoreOverrideCursor(); }
};

void styleButton(QPushButton *button, const QString &normal, const QString &hover)
{
    button->setCursor(Qt::PointingHandCursor);
    button->setStyleSheet(QString(
        "QPushButton { background-color: %1; border: none; border-radius: 12px; color: white;"
        "              font-weight: bold; font-size: 14px; padding: 12px 24px; min-height: 20px; }"
        "QPushButton:hover { background-color: %2; }"
        "QPushButton:pressed { background-color: %2; padding-top: 14px; }"
        "QPushButton:disabled { background-color: #4b5563; color: #9ca3af; }").arg(normal, hover));
}

const char *kIdleStyle =
    "QLabel { border: 3px dashed #3498db; border-radius: 15px; background-color: #f8f9fa;"
    "         color: #6c757d; font-size: 16px; font-weight: 500; }";
const char *kActiveStyle =
    "QLabel { border: 3px solid #2ecc71; border-radius: 15px; background-color: #e8f5e8;"
    "         color: #27ae60; font-size: 16px; font-weight: 500; }";

} // namespace

// ---------------------------------------------------------------------------
// ImageDropZone
// ---------------------------------------------------------------------------

ImageDropZone::ImageDropZone(QWidget *parent)
    : QLabel(parent)
{
    setAcceptDrops(true);
    setMinimumSize(400, 300);
    setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored); // the pixmap must not dictate our size
    setAlignment(Qt::AlignCenter);
    applyStyle(false);
    clearImage();
}

bool ImageDropZone::isSupportedImagePath(const QString &path)
{
    static const QStringList suffixes = { "bmp", "png", "jpg", "jpeg" };
    return suffixes.contains(QFileInfo(path).suffix().toLower());
}

void ImageDropZone::applyStyle(bool dragActive)
{
    setStyleSheet(QString::fromLatin1(dragActive ? kActiveStyle : kIdleStyle));
}

void ImageDropZone::setImage(const QImage &image)
{
    m_source = QPixmap::fromImage(image);
    setText(QString());
    rescale();
}

void ImageDropZone::clearImage()
{
    m_source = QPixmap();
    QLabel::clear();
    setText("Drop an image here or click 'Load Image'\nSupported formats: BMP, PNG, JPG");
}

void ImageDropZone::rescale()
{
    if (m_source.isNull())
        return;
    const QSize target = size() - QSize(20, 20);
    const bool tooBig = m_source.width() > target.width() || m_source.height() > target.height();
    QLabel::setPixmap(tooBig ? m_source.scaled(target, Qt::KeepAspectRatio, Qt::SmoothTransformation)
                             : m_source);
}

void ImageDropZone::resizeEvent(QResizeEvent *event)
{
    QLabel::resizeEvent(event);
    rescale();
}

void ImageDropZone::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()
        && isSupportedImagePath(event->mimeData()->urls().first().toLocalFile())) {
        applyStyle(true);
        event->acceptProposedAction();
        return;
    }
    event->ignore();
}

void ImageDropZone::dragLeaveEvent(QDragLeaveEvent *event)
{
    applyStyle(false);
    QLabel::dragLeaveEvent(event);
}

void ImageDropZone::dropEvent(QDropEvent *event)
{
    applyStyle(false);
    if (!event->mimeData()->hasUrls())
        return;
    const QString path = event->mimeData()->urls().first().toLocalFile();
    if (isSupportedImagePath(path))
        emit imageDropped(path);
}

void ImageDropZone::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);

    if (m_source.isNull()) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        const QRect iconRect(width() / 2 - 30, height() / 2 - 60, 60, 40);
        painter.setPen(QPen(QColor("#6c757d"), 3));
        painter.drawRoundedRect(iconRect, 5, 5);
        painter.drawLine(iconRect.left() + 10, iconRect.top() + 10,
                         iconRect.left() + 20, iconRect.top() + 20);
        painter.drawEllipse(iconRect.left() + 35, iconRect.top() + 8, 8, 8);
    }
}

// ---------------------------------------------------------------------------
// MainWindow
// ---------------------------------------------------------------------------

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUI();
    setupStyling();

    connect(m_loadImageBtn, &QPushButton::clicked, this, &MainWindow::onChooseImageClicked);
    connect(m_saveImageBtn, &QPushButton::clicked, this, &MainWindow::onSaveImageClicked);
    connect(m_encryptBtn, &QPushButton::clicked, this, &MainWindow::onEncryptClicked);
    connect(m_decryptBtn, &QPushButton::clicked, this, &MainWindow::onDecryptClicked);
    connect(m_imageLabel, &ImageDropZone::imageDropped, this, &MainWindow::loadImageFile);
}

void MainWindow::setupUI()
{
    setWindowTitle("SteganoCrypto - Steganography Tool");
    setMinimumSize(1000, 650);
    resize(1300, 800);

    auto *splitter = new QSplitter(Qt::Horizontal);

    // Left panel: image
    auto *imagePanel = new QWidget;
    auto *imageLayout = new QVBoxLayout(imagePanel);

    m_imageLabel = new ImageDropZone;
    auto *scrollArea = new QScrollArea;
    scrollArea->setWidget(m_imageLabel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setMinimumWidth(450);
    imageLayout->addWidget(scrollArea);

    m_loadImageBtn = new QPushButton("📁 Load Image");
    m_saveImageBtn = new QPushButton("💾 Save Image");
    styleButton(m_loadImageBtn, "#3498db", "#2980b9");
    styleButton(m_saveImageBtn, "#27ae60", "#229954");
    auto *imageButtons = new QHBoxLayout;
    imageButtons->addWidget(m_loadImageBtn);
    imageButtons->addWidget(m_saveImageBtn);
    imageButtons->addStretch();
    imageLayout->addLayout(imageButtons);

    // Right panel: controls
    auto *controlPanel = new QWidget;
    controlPanel->setMinimumWidth(450);
    auto *controlLayout = new QVBoxLayout(controlPanel);

    auto *inputGroup = new QGroupBox("🔤 Secret Message");
    auto *inputLayout = new QVBoxLayout(inputGroup);
    m_textInput = new QTextEdit;
    m_textInput->setAcceptRichText(false);
    m_textInput->setPlaceholderText("Enter the text you want to encrypt and hide...");
    m_textInput->setMaximumHeight(140);
    inputLayout->addWidget(m_textInput);

    auto *keyGroup = new QGroupBox("🔑 Passphrase");
    auto *keyLayout = new QVBoxLayout(keyGroup);
    m_keyInput = new QLineEdit;
    m_keyInput->setPlaceholderText(
        QString("Leave empty to auto-generate (or type your own, min. %1 chars)").arg(kMinPassphraseLength));
    keyLayout->addWidget(m_keyInput);

    m_encryptBtn = new QPushButton("🔒 Encrypt && Hide");
    m_decryptBtn = new QPushButton("🔓 Extract && Decrypt");
    styleButton(m_encryptBtn, "#e74c3c", "#c0392b");
    styleButton(m_decryptBtn, "#f39c12", "#e67e22");
    auto *actionButtons = new QHBoxLayout;
    actionButtons->addWidget(m_encryptBtn);
    actionButtons->addWidget(m_decryptBtn);

    auto *outputGroup = new QGroupBox("📄 Decrypted Result");
    auto *outputLayout = new QVBoxLayout(outputGroup);
    m_resultOutput = new QTextEdit;
    m_resultOutput->setReadOnly(true);
    m_resultOutput->setPlaceholderText("Decrypted message will appear here...");
    outputLayout->addWidget(m_resultOutput);

    controlLayout->addWidget(inputGroup);
    controlLayout->addWidget(keyGroup);
    controlLayout->addLayout(actionButtons);
    controlLayout->addWidget(outputGroup, 1);

    splitter->addWidget(imagePanel);
    splitter->addWidget(controlPanel);
    splitter->setSizes({650, 500});

    auto *central = new QWidget;
    auto *mainLayout = new QHBoxLayout(central);
    mainLayout->addWidget(splitter);
    setCentralWidget(central);

    statusBar()->showMessage("Load an image to get started.");
}

void MainWindow::setupStyling()
{
    setStyleSheet(
        "QMainWindow { background-color: #0f0f23; }"
        "QStatusBar { color: #d1d5db; }"
        "QGroupBox { font-weight: bold; font-size: 16px; color: #2c3e50; border: 2px solid #e9ecef;"
        "            border-radius: 10px; margin-top: 10px; padding-top: 15px; background-color: #1a1a2e; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 15px; padding: 5px 10px;"
        "                   background-color: #2e1e5e; color: white; border-radius: 8px; }"
        "QTextEdit, QLineEdit { color: white; border-radius: 8px; padding: 10px; font-size: 14px;"
        "                       background-color: #1a1a2e; selection-background-color: #3498db; }"
        "QTextEdit:focus, QLineEdit:focus { border-color: #3498db; outline: none; }"
        "QScrollArea { border: none; background-color: transparent; }");
}

void MainWindow::showError(const QString &title, const QString &message)
{
    QMessageBox::warning(this, title, message);
}

void MainWindow::showStatus(const QString &message)
{
    statusBar()->showMessage(message);
}

const QImage &MainWindow::currentImage() const
{
    return m_stego.isNull() ? m_cover : m_stego;
}

void MainWindow::onChooseImageClicked()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, "Choose an Image", QString(), "Images (*.bmp *.png *.jpg *.jpeg)");
    if (!filePath.isEmpty())
        loadImageFile(filePath);
}

void MainWindow::loadImageFile(const QString &filePath)
{
    BusyCursor busy;

    QImage image(filePath);
    if (image.isNull()) {
        showError("Error", "Failed to load the image.");
        return;
    }

    m_cover = image.convertToFormat(QImage::Format_ARGB32);
    m_stego = QImage();
    m_resultOutput->clear();
    m_imageLabel->setImage(m_cover);

    showStatus(QString("Loaded %1 (%2 x %3) - can hide up to %4 bytes.")
                   .arg(QFileInfo(filePath).fileName())
                   .arg(m_cover.width()).arg(m_cover.height())
                   .arg(Steganography::capacityBytes(m_cover)));

    const QString suffix = QFileInfo(filePath).suffix().toLower();
    if (suffix == "jpg" || suffix == "jpeg") {
        QMessageBox::information(this, "JPEG image",
            "JPEG is a lossy format. You can use it as a cover image, but hidden data cannot be "
            "read back from a JPEG file.\n\nSave the result as PNG or BMP, and extract from that.");
    }
}

void MainWindow::onEncryptClicked()
{
    if (m_cover.isNull()) {
        showError("Error", "Please load an image first!");
        return;
    }

    const QString text = m_textInput->toPlainText();
    if (text.isEmpty()) {
        showError("Error", "Please enter text to encrypt!");
        return;
    }

    QString passphrase = m_keyInput->text().trimmed();
    if (passphrase.isEmpty()) {
        passphrase = Chiffrement::genererPassphrase();
        if (passphrase.isEmpty()) {
            showError("Error", "Failed to generate a passphrase.");
            return;
        }
    } else if (passphrase.length() < kMinPassphraseLength) {
        showError("Passphrase too short",
                  QString("Please use at least %1 characters, or leave the field empty to auto-generate one.")
                      .arg(kMinPassphraseLength));
        return;
    }

    BusyCursor busy;

    const QByteArray blob = Chiffrement::chiffrer(text.toUtf8(), passphrase);
    if (blob.isEmpty()) {
        showError("Error", "Encryption failed.");
        return;
    }

    // Always start from the untouched cover, so encrypting twice never stacks messages.
    QImage stego;
    const auto status = Steganography::embed(m_cover, blob, stego);
    if (status != Steganography::Status::Ok) {
        QString message = Steganography::describe(status);
        if (status == Steganography::Status::TooLarge) {
            message += QString("\n\nNeeded: %1 bytes. This image can hold: %2 bytes.")
                           .arg(blob.size()).arg(Steganography::capacityBytes(m_cover));
        }
        showError("Cannot hide data", message);
        return;
    }

    m_stego = stego;
    m_keyInput->setText(passphrase);
    m_imageLabel->setImage(m_stego);
    showStatus("Message encrypted and hidden. Now save the image as PNG or BMP, and keep your passphrase safe!");
}

void MainWindow::onDecryptClicked()
{
    const QImage &source = currentImage();
    if (source.isNull()) {
        showError("Error", "Please load an image first!");
        return;
    }

    const QString passphrase = m_keyInput->text().trimmed();
    if (passphrase.isEmpty()) {
        showError("Error", "Please provide the passphrase!");
        return;
    }

    BusyCursor busy;
    m_resultOutput->clear();

    QByteArray blob;
    const auto status = Steganography::extract(source, blob);
    if (status != Steganography::Status::Ok) {
        showError("Nothing to extract", Steganography::describe(status));
        return;
    }

    QByteArray plain;
    if (!Chiffrement::dechiffrer(blob, passphrase, plain)) {
        showError("Decryption failed", "Wrong passphrase, or the hidden data was modified.");
        return;
    }

    m_resultOutput->setPlainText(QString::fromUtf8(plain));
    showStatus("Hidden message extracted and decrypted.");
}

void MainWindow::onSaveImageClicked()
{
    const QImage &image = currentImage();
    if (image.isNull()) {
        showError("Error", "No image to save!");
        return;
    }

    QString selectedFilter;
    QString savePath = QFileDialog::getSaveFileName(
        this, "Save Image", QString(), "PNG (*.png);;BMP (*.bmp)", &selectedFilter);
    if (savePath.isEmpty())
        return;

    // Make sure the file really has a lossless extension.
    QString suffix = QFileInfo(savePath).suffix().toLower();
    if (suffix != "png" && suffix != "bmp") {
        suffix = selectedFilter.startsWith("BMP") ? "bmp" : "png";
        savePath += "." + suffix;
    }

    if (suffix == "bmp" && !Steganography::isFullyOpaque(image)) {
        showError("Cannot save as BMP",
                  "This image has transparent areas, which BMP cannot store reliably "
                  "(the hidden data could be lost). Please save it as PNG.");
        return;
    }

    BusyCursor busy;
    if (!image.save(savePath, suffix == "png" ? "PNG" : "BMP"))
        showError("Error", "Failed to save the image.");
    else
        showStatus("Image saved to " + savePath);
}
