/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "hdrimage.h"

#include <QFile>
#include <QFileInfo>
#include <QDebug>

#include <cmath>

namespace QmlDesigner {

// Hdr loading code was copied from QtQuick3D and adapted to create QImage instead of texture.

typedef unsigned char RGBE[4];
#define R 0
#define G 1
#define B 2
#define E 3

struct M8E8
{
    quint8 m;
    quint8 e;
    M8E8() : m(0), e(0){
    }
    M8E8(const float val) {
        float l2 = 1.f + std::floor(log2f(val));
        float mm = val / powf(2.f, l2);
        m = quint8(mm * 255.f);
        e = quint8(l2 + 128);
    }
    M8E8(const float val, quint8 exp) {
        if (val <= 0) {
            m = e = 0;
            return;
        }
        float mm = val / powf(2.f, exp - 128);
        m = quint8(mm * 255.f);
        e = exp;
    }
};

QByteArray fileToByteArray(QString const &filename)
{
    QFile file(filename);
    QFileInfo info(file);

    if (info.exists() && file.open(QFile::ReadOnly))
        return file.readAll();

    return {};
}

inline int calculateLine(int width, int bitdepth) { return ((width * bitdepth) + 7) / 8; }

inline int calculatePitch(int line) { return (line + 3) & ~3; }

float convertComponent(int exponent, int val)
{
    float v = val / (256.0f);
    float d = powf(2.0f, (float)exponent - 128.0f);
    return v * d;
}

void decrunchScanline(const char *&p, const char *pEnd, RGBE *scanline, int w)
{
    scanline[0][R] = *p++;
    scanline[0][G] = *p++;
    scanline[0][B] = *p++;
    scanline[0][E] = *p++;

    if (scanline[0][R] == 2 && scanline[0][G] == 2 && scanline[0][B] < 128) {
        // new rle, the first pixel was a dummy
        for (int channel = 0; channel < 4; ++channel) {
            for (int x = 0; x < w && p < pEnd; ) {
                unsigned char c = *p++;
                if (c > 128) { // run
                    if (p < pEnd) {
                        int repCount = c & 127;
                        c = *p++;
                        while (repCount--)
                            scanline[x++][channel] = c;
                    }
                } else { // not a run
                    while (c-- && p < pEnd)
                        scanline[x++][channel] = *p++;
                }
            }
        }
    } else {
        // old rle
        scanline[0][R] = 2;
        int bitshift = 0;
        int x = 1;
        while (x < w && pEnd - p >= 4) {
            scanline[x][R] = *p++;
            scanline[x][G] = *p++;
            scanline[x][B] = *p++;
            scanline[x][E] = *p++;

            if (scanline[x][R] == 1 && scanline[x][G] == 1 && scanline[x][B] == 1) { // run
                int repCount = scanline[x][3] << bitshift;
                while (repCount--) {
                    memcpy(scanline[x], scanline[x - 1], 4);
                    ++x;
                }
                bitshift += 8;
            } else { // not a run
                ++x;
                bitshift = 0;
            }
        }
    }
}

void decodeScanlineToImageData(RGBE *scanline, int width, void *outBuf, quint32 offset)
{
    quint8 *target = reinterpret_cast<quint8 *>(outBuf);
    target += offset;

    float rgbaF32[4];
    for (int i = 0; i < width; ++i) {
        rgbaF32[R] = convertComponent(scanline[i][E], scanline[i][R]);
        rgbaF32[G] = convertComponent(scanline[i][E], scanline[i][G]);
        rgbaF32[B] = convertComponent(scanline[i][E], scanline[i][B]);
        rgbaF32[3] = 1.0f;

        float max = qMax(rgbaF32[R], qMax(rgbaF32[G], rgbaF32[B]));
        M8E8 ex(max);
        M8E8 a(rgbaF32[R], ex.e);
        M8E8 b(rgbaF32[G], ex.e);
        M8E8 c(rgbaF32[B], ex.e);
        quint8 *dst = target + i * 4;
        dst[0] = c.m;
        dst[1] = b.m;
        dst[2] = a.m;
        dst[3] = 255;
    }
}

HdrImage::HdrImage(const QString &fileName)
    : m_fileName(fileName)
{
    loadHdr();
}

QPixmap HdrImage::toPixmap() const
{
    // Copy is used to ensure pixmap will be valid after HdrImage goes out of scope
    return QPixmap::fromImage(m_image, Qt::NoFormatConversion).copy();
}

void HdrImage::loadHdr()
{
    QByteArray buf(fileToByteArray(m_fileName));

    auto handleError = [this](const QString &error) {
        qWarning() << QStringLiteral("Failed to load HDR image '%1': %2.").arg(m_fileName, error).toUtf8();
    };

    if (buf.isEmpty()) {
        handleError("File open failed");
        return;
    }

    if (!buf.startsWith(QByteArrayLiteral("#?RADIANCE\n"))) {
        handleError("Non-HDR file");
        return;
    }

    const char *p = buf.constData();
    const char *pEnd = p + buf.size();

    // Process lines until the empty one.
    QByteArray line;
    QByteArray formatTag {"FORMAT="};
    QByteArray formatId {"32-bit_rle_rgbe"};
    while (p < pEnd) {
        char c = *p++;
        if (c == '\n') {
            if (line.isEmpty())
                break;
            if (line.startsWith(formatTag)) {
                const QByteArray format = line.mid(7).trimmed();
                if (format != formatId) {
                    handleError(QStringLiteral("Unsupported HDR format '%1'")
                                .arg(QString::fromUtf8(format)));
                    return;
                }
            }
            line.clear();
        } else {
            line.append(c);
        }
    }
    if (p == pEnd) {
        handleError("Malformed HDR image data at property strings");
        return;
    }

    // Get the resolution string.
    while (p < pEnd) {
        char c = *p++;
        if (c == '\n')
            break;
        line.append(c);
    }
    if (p == pEnd) {
        handleError("Malformed HDR image data at resolution string");
        return;
    }

    int width = 0;
    int height = 0;
    // We only care about the standard orientation
    if (!::sscanf(line.constData(), "-Y %d +X %d", &height, &width)) {
        handleError(QStringLiteral("Unsupported HDR resolution string '%1'")
                    .arg(QString::fromUtf8(line)));
        return;
    }
    if (width <= 0 || height <= 0) {
        handleError("Invalid HDR resolution");
        return;
    }

    const int bytesPerPixel = 4;
    const int bitCount = bytesPerPixel * 8;
    const int pitch = calculatePitch(calculateLine(width, bitCount));
    const int dataSize = int(height * pitch);

    m_buf = QByteArray {dataSize, 0};

    // Allocate a scanline worth of RGBE data
    RGBE *scanline = new RGBE[width];

    for (int y = 0; y < height; ++y) {
        quint32 byteOffset = quint32(y * width * bytesPerPixel);
        if (pEnd - p < 4) {
            handleError("Unexpected end of HDR data");
            delete[] scanline;
            return;
        }
        decrunchScanline(p, pEnd, scanline, width);
        decodeScanlineToImageData(scanline, width, m_buf.data(), byteOffset);
    }

    delete[] scanline;

    m_image = QImage {reinterpret_cast<const uchar *>(m_buf.constData()), width, height, pitch,
                      QImage::Format_ARGB32};
}

} // namespace QmlDesigner
