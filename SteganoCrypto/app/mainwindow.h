#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QImage>
#include <QLabel>
#include <QMainWindow>
#include <QPixmap>

class QDragEnterEvent;
class QDragLeaveEvent;
class QDropEvent;
class QGroupBox;
class QLineEdit;
class QPushButton;
class QResizeEvent;
class QTextEdit;

/// Label that shows the current image, scales it to fit, and accepts dropped image files.
class ImageDropZone : public QLabel
{
    Q_OBJECT
public:
    explicit ImageDropZone(QWidget *parent = nullptr);

    void setImage(const QImage &image);
    void clearImage();

    static bool isSupportedImagePath(const QString &path);

signals:
    void imageDropped(const QString &filePath);

protected:
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private:
    void applyStyle(bool dragActive);
    void rescale();

    QPixmap m_source;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void onChooseImageClicked();
    void onEncryptClicked();
    void onDecryptClicked();
    void onSaveImageClicked();
    void loadImageFile(const QString &filePath);

private:
    void setupUI();
    void setupStyling();
    void showError(const QString &title, const QString &message);
    void showStatus(const QString &message);
    const QImage &currentImage() const; // image with hidden data if present, else the loaded one

    ImageDropZone *m_imageLabel = nullptr;
    QPushButton *m_loadImageBtn = nullptr;
    QPushButton *m_saveImageBtn = nullptr;
    QTextEdit *m_textInput = nullptr;
    QLineEdit *m_keyInput = nullptr;
    QPushButton *m_encryptBtn = nullptr;
    QPushButton *m_decryptBtn = nullptr;
    QTextEdit *m_resultOutput = nullptr;

    QImage m_cover; ///< image exactly as loaded; never modified
    QImage m_stego; ///< m_cover with the hidden message (null until "Encrypt & Hide")
};

#endif // MAINWINDOW_H
