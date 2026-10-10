#include "steganography.h"

#include <algorithm>
#include <cstring>
#include <limits>

namespace {

constexpr char kMagic[4] = { 'S', 'C', 'R', 'Y' };
constexpr int kHeaderBytes = 8;

inline bool isOpaque(QRgb pixel) { return qAlpha(pixel) == 0xFF; }

QImage asArgb32(const QImage &image)
{
    return image.format() == QImage::Format_ARGB32
               ? image
               : image.convertToFormat(QImage::Format_ARGB32);
}

qint64 countOpaquePixels(const QImage &argb)
{
    qint64 count = 0;
    for (int y = 0; y < argb.height(); ++y) {
        const QRgb *line = reinterpret_cast<const QRgb *>(argb.constScanLine(y));
        for (int x = 0; x < argb.width(); ++x)
            count += isOpaque(line[x]) ? 1 : 0;
    }
    return count;
}

/// Reads the LSB stream in the same order embed() writes it.
class BitReader
{
public:
    explicit BitReader(const QImage &argb) : m_image(argb) {}

    bool nextBit(int &bit)
    {
        while (m_channel >= 3) {
            if (m_y >= m_image.height())
                return false;
            const QRgb *line = reinterpret_cast<const QRgb *>(m_image.constScanLine(m_y));
            const QRgb pixel = line[m_x];
            if (++m_x >= m_image.width()) {
                m_x = 0;
                ++m_y;
            }
            if (!isOpaque(pixel))
                continue;
            m_pixel = pixel;
            m_channel = 0;
        }
        const int value = m_channel == 0 ? qRed(m_pixel)
                        : m_channel == 1 ? qGreen(m_pixel)
                                         : qBlue(m_pixel);
        ++m_channel;
        bit = value & 1;
        return true;
    }

    bool nextByte(quint8 &byte)
    {
        byte = 0;
        for (int i = 0; i < 8; ++i) {
            int bit = 0;
            if (!nextBit(bit))
                return false;
            byte = static_cast<quint8>((byte << 1) | bit);
        }
        return true;
    }

private:
    const QImage &m_image;
    int m_x = 0;
    int m_y = 0;
    int m_channel = 3; // forces loading the first opaque pixel
    QRgb m_pixel = 0;
};

} // namespace

qint64 Steganography::capacityBytes(const QImage &image)
{
    if (image.isNull())
        return 0;
    const QImage argb = asArgb32(image);
    const qint64 bytes = countOpaquePixels(argb) * 3 / 8 - kHeaderBytes;
    return std::clamp<qint64>(bytes, 0, std::numeric_limits<quint32>::max());
}

bool Steganography::isFullyOpaque(const QImage &image)
{
    if (image.isNull())
        return true;
    const QImage argb = asArgb32(image);
    return countOpaquePixels(argb) == static_cast<qint64>(argb.width()) * argb.height();
}

Steganography::Status Steganography::embed(const QImage &cover, const QByteArray &payload, QImage &out)
{
    out = QImage();
    if (cover.isNull())
        return Status::InvalidImage;
    if (payload.isEmpty())
        return Status::EmptyPayload;

    QImage img = asArgb32(cover);
    if (payload.size() > capacityBytes(img))
        return Status::TooLarge;

    const quint32 length = static_cast<quint32>(payload.size());
    QByteArray stream;
    stream.reserve(kHeaderBytes + payload.size());
    stream.append(kMagic, 4);
    stream.append(static_cast<char>((length >> 24) & 0xFF));
    stream.append(static_cast<char>((length >> 16) & 0xFF));
    stream.append(static_cast<char>((length >> 8) & 0xFF));
    stream.append(static_cast<char>(length & 0xFF));
    stream.append(payload);

    const qint64 totalBits = static_cast<qint64>(stream.size()) * 8;
    qint64 bit = 0;
    auto nextBit = [&]() -> int {
        const int value = (static_cast<quint8>(stream.at(static_cast<int>(bit >> 3))) >> (7 - (bit & 7))) & 1;
        ++bit;
        return value;
    };

    for (int y = 0; y < img.height() && bit < totalBits; ++y) {
        QRgb *line = reinterpret_cast<QRgb *>(img.scanLine(y)); // detaches from the cover
        for (int x = 0; x < img.width() && bit < totalBits; ++x) {
            const QRgb pixel = line[x];
            if (!isOpaque(pixel))
                continue;
            int r = qRed(pixel);
            int g = qGreen(pixel);
            int b = qBlue(pixel);
            if (bit < totalBits) r = (r & 0xFE) | nextBit();
            if (bit < totalBits) g = (g & 0xFE) | nextBit();
            if (bit < totalBits) b = (b & 0xFE) | nextBit();
            line[x] = qRgba(r, g, b, 0xFF);
        }
    }

    out = img;
    return Status::Ok;
}

Steganography::Status Steganography::extract(const QImage &image, QByteArray &payload)
{
    payload.clear();
    if (image.isNull())
        return Status::InvalidImage;

    const QImage argb = asArgb32(image);
    BitReader reader(argb);

    quint8 header[kHeaderBytes];
    for (quint8 &byte : header) {
        if (!reader.nextByte(byte))
            return Status::NoData;
    }
    if (std::memcmp(header, kMagic, 4) != 0)
        return Status::NoData;

    const quint32 length = (quint32(header[4]) << 24) | (quint32(header[5]) << 16)
                         | (quint32(header[6]) << 8) | quint32(header[7]);
    if (length == 0 || qint64(length) > capacityBytes(argb))
        return Status::Corrupted;

    QByteArray data;
    data.reserve(static_cast<int>(length));
    for (quint32 i = 0; i < length; ++i) {
        quint8 byte = 0;
        if (!reader.nextByte(byte))
            return Status::Corrupted;
        data.append(static_cast<char>(byte));
    }

    payload = data;
    return Status::Ok;
}

QString Steganography::describe(Status status)
{
    switch (status) {
    case Status::Ok:           return QStringLiteral("OK");
    case Status::InvalidImage: return QStringLiteral("No valid image loaded.");
    case Status::EmptyPayload: return QStringLiteral("There is nothing to hide.");
    case Status::TooLarge:     return QStringLiteral("The message is too large for this image. Use a bigger image or a shorter message.");
    case Status::NoData:       return QStringLiteral("No hidden data found in this image. (Data is lost if the image was ever saved as JPEG or resized.)");
    case Status::Corrupted:    return QStringLiteral("Hidden data was found but it is damaged or incomplete.");
    }
    return {};
}
