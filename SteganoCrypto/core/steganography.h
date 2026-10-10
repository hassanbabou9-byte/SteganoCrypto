#ifndef STEGANOGRAPHY_H
#define STEGANOGRAPHY_H

#include <QByteArray>
#include <QImage>
#include <QString>

/**
 * LSB steganography: hides a byte payload in the least significant bit of the
 * R, G and B channels (3 bits per pixel). Fully opaque pixels only - pixels
 * with any transparency are skipped on both embed and extract, so transparent
 * PNGs survive a save/load round trip.
 *
 * Stream layout:  "SCRY" magic (4) | payload length, big endian (4) | payload
 *
 * The carrier must be stored losslessly (PNG / BMP). JPEG recompression
 * destroys the hidden bits.
 */
class Steganography
{
public:
    enum class Status {
        Ok,
        InvalidImage,   ///< null image
        EmptyPayload,
        TooLarge,       ///< payload does not fit in the image
        NoData,         ///< no hidden-data marker found
        Corrupted       ///< marker found but length/data is inconsistent
    };

    /// Maximum payload size (bytes) the image can carry.
    static qint64 capacityBytes(const QImage &image);

    /// True if every pixel is fully opaque.
    static bool isFullyOpaque(const QImage &image);

    /// Hides @p payload in a copy of @p cover; the cover itself is never modified.
    static Status embed(const QImage &cover, const QByteArray &payload, QImage &out);

    /// Reads a payload previously hidden by embed().
    static Status extract(const QImage &image, QByteArray &payload);

    static QString describe(Status status);
};

#endif // STEGANOGRAPHY_H
