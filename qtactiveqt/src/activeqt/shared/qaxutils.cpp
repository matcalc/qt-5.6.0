/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the ActiveQt framework of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qaxutils_p.h"

#include <QtWidgets/QWidget>
#include <QtGui/QPixmap>
#include <QtGui/QRegion>
#include <QtGui/QWindow>
#include <QtGui/QGuiApplication>
#include <private/qhighdpiscaling_p.h>
#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformpixmap.h>
#include <QtGui/private/qpixmap_raster_p.h>
#include <QtCore/QScopedArrayPointer>
#include <QtCore/QRect>

#include <QtCore/QDebug>

QT_BEGIN_NAMESPACE

static inline QWindow *windowForWidget(QWidget *widget)
{
    if (QWindow *window = widget->windowHandle())
        return window;
    if (QWidget *nativeParent = widget->nativeParentWidget())
        return nativeParent->windowHandle();
    return 0;
}

HWND hwndForWidget(QWidget *widget)
{
    if (QWindow *window = windowForWidget(widget))
        return static_cast<HWND> (QGuiApplication::platformNativeInterface()->nativeResourceForWindow("handle", window));
    return 0;
}

// Code courtesy of the Windows platform plugin (see pixmaputils.cpp/h).
HBITMAP qaxPixmapToWinHBITMAP(const QPixmap &p, HBitmapFormat format)
{
    if (p.isNull())
        return 0;

    HBITMAP bitmap = 0;
    if (p.handle()->classId() != QPlatformPixmap::RasterClass) {
        QRasterPlatformPixmap *data = new QRasterPlatformPixmap(p.depth() == 1 ?
            QRasterPlatformPixmap::BitmapType : QRasterPlatformPixmap::PixmapType);
        data->fromImage(p.toImage(), Qt::AutoColor);
        return qaxPixmapToWinHBITMAP(QPixmap(data), format);
    }

    QRasterPlatformPixmap *d = static_cast<QRasterPlatformPixmap*>(p.handle());
    const QImage *rasterImage = d->buffer();
    const int w = rasterImage->width();
    const int h = rasterImage->height();

    HDC display_dc = GetDC(0);

    // Define the header
    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = w * h * 4;

    // Create the pixmap
    uchar *pixels = 0;
    bitmap = CreateDIBSection(display_dc, &bmi, DIB_RGB_COLORS, (void **) &pixels, 0, 0);
    ReleaseDC(0, display_dc);
    if (!bitmap) {
        qErrnoWarning("%s, failed to create dibsection", __FUNCTION__);
        return 0;
    }
    if (!pixels) {
        qErrnoWarning("%s, did not allocate pixel data", __FUNCTION__);
        return 0;
    }

    // Copy over the data
    QImage::Format imageFormat = QImage::Format_ARGB32;
    if (format == HBitmapAlpha)
        imageFormat = QImage::Format_RGB32;
    else if (format == HBitmapPremultipliedAlpha)
        imageFormat = QImage::Format_ARGB32_Premultiplied;
    const QImage image = rasterImage->convertToFormat(imageFormat);
    const int bytes_per_line = w * 4;
    for (int y=0; y < h; ++y)
        memcpy(pixels + y * bytes_per_line, image.scanLine(y), bytes_per_line);

    return bitmap;
}

QPixmap qaxPixmapFromWinHBITMAP(HBITMAP bitmap, HBitmapFormat format)
{
    // Verify size
    BITMAP bitmap_info;
    memset(&bitmap_info, 0, sizeof(BITMAP));

    const int res = GetObject(bitmap, sizeof(BITMAP), &bitmap_info);
    if (!res) {
        qErrnoWarning("QPixmap::fromWinHBITMAP(), failed to get bitmap info");
        return QPixmap();
    }
    const int w = bitmap_info.bmWidth;
    const int h = bitmap_info.bmHeight;

    BITMAPINFO bmi;
    memset(&bmi, 0, sizeof(bmi));
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    bmi.bmiHeader.biSizeImage   = w * h * 4;

    // Get bitmap bits
    QScopedArrayPointer<uchar> data(new uchar[bmi.bmiHeader.biSizeImage]);
    HDC display_dc = GetDC(0);
    if (!GetDIBits(display_dc, bitmap, 0, h, data.data(), &bmi, DIB_RGB_COLORS)) {
        ReleaseDC(0, display_dc);
        qWarning("%s, failed to get bitmap bits", __FUNCTION__);
        return QPixmap();
    }

    QImage::Format imageFormat = QImage::Format_ARGB32_Premultiplied;
    uint mask = 0;
    if (format == HBitmapNoAlpha) {
        imageFormat = QImage::Format_RGB32;
        mask = 0xff000000;
    }

    // Create image and copy data into image.
    QImage image(w, h, imageFormat);
    if (image.isNull()) { // failed to alloc?
        ReleaseDC(0, display_dc);
        qWarning("%s, failed create image of %dx%d", __FUNCTION__, w, h);
        return QPixmap();
    }
    const int bytes_per_line = w * sizeof(QRgb);
    for (int y = 0; y < h; ++y) {
        QRgb *dest = (QRgb *) image.scanLine(y);
        const QRgb *src = (const QRgb *) (data.data() + y * bytes_per_line);
        for (int x = 0; x < w; ++x) {
            const uint pixel = src[x];
            if ((pixel & 0xff000000) == 0 && (pixel & 0x00ffffff) != 0)
                dest[x] = pixel | 0xff000000;
            else
                dest[x] = pixel | mask;
        }
    }
    ReleaseDC(0, display_dc);
    return QPixmap::fromImage(image);
}

// Code courtesy of QWindowsXPStyle
static void addRectToHrgn(HRGN &winRegion, const QRect &r)
{
    HRGN rgn = CreateRectRgn(r.left(), r.top(), r.x() + r.width(), r.y() + r.height());
    if (rgn) {
        HRGN dest = CreateRectRgn(0,0,0,0);
        int result = CombineRgn(dest, winRegion, rgn, RGN_OR);
        if (result) {
            DeleteObject(winRegion);
            winRegion = dest;
        }
        DeleteObject(rgn);
    }
}

HRGN qaxHrgnFromQRegion(QRegion region, const QWindow *window)
{
    region = QHighDpi::toNativeLocalRegion(region, window);
    HRGN hRegion = CreateRectRgn(0, 0, 0, 0);
    if (region.rectCount() == 1) {
        addRectToHrgn(hRegion, region.boundingRect());
        return hRegion;
    }
    foreach (const QRect &rect, region.rects())
        addRectToHrgn(hRegion, rect);
    return hRegion;
}

// HIMETRICS scaling
static const qreal himetricsPerInch = 2540;

static inline long qaxMapPixToLogHiMetrics(int x, qreal logicalDpi, qreal factor)
{
    return qRound((qreal(x) * himetricsPerInch * factor) / logicalDpi);
}

static inline int qaxMapLogHiMetricsToPix(long x, qreal logicalDpi, qreal factor)
{
    return qRound(logicalDpi * qreal(x) / (himetricsPerInch * factor));
}

SIZEL qaxMapPixToLogHiMetrics(const QSize &s, const QDpi &d, const QWindow *w)
{
    const qreal factor = QHighDpiScaling::factor(w);
    const SIZEL result = {
        qaxMapPixToLogHiMetrics(s.width(), d.first, factor),
        qaxMapPixToLogHiMetrics(s.height(), d.second, factor)
    };
    return result;
}

QSize qaxMapLogHiMetricsToPix(const SIZEL &s, const QDpi &d, const QWindow *w)
{
    const qreal factor = QHighDpiScaling::factor(w);
    const QSize result(qaxMapLogHiMetricsToPix(s.cx, d.first, factor),
                       qaxMapLogHiMetricsToPix(s.cy, d.second, factor));
    return result;
}

// Cache logical DPI in case High DPI scaling is active (which case
// the fake logical DPI it calculates is not suitable).

static QDpi cachedSystemLogicalDpi(-1, -1);

void qaxClearCachedSystemLogicalDpi() // Call from WM_DISPLAYCHANGE
{
    cachedSystemLogicalDpi = QDpi(-1, -1);
}

static inline QDpi systemLogicalDpi()
{
    if (cachedSystemLogicalDpi.first < 0) {
        const HDC displayDC = GetDC(0);
        cachedSystemLogicalDpi = QDpi(GetDeviceCaps(displayDC, LOGPIXELSX), GetDeviceCaps(displayDC, LOGPIXELSY));
        ReleaseDC(0, displayDC);
    }
    return cachedSystemLogicalDpi;
}

static inline QDpi paintDeviceLogicalDpi(const QPaintDevice *d)
{
    return QDpi(d->logicalDpiX(), d->logicalDpiY());
}

#ifdef QT_WIDGETS_LIB

SIZEL qaxMapPixToLogHiMetrics(const QSize &s, const QWidget *widget)
{
    return qaxMapPixToLogHiMetrics(s,
                                   QHighDpiScaling::isActive() ? systemLogicalDpi() : paintDeviceLogicalDpi(widget),
                                   widget->windowHandle());
}

QSize qaxMapLogHiMetricsToPix(const SIZEL &s, const QWidget *widget)
{
    return qaxMapLogHiMetricsToPix(s,
                                   QHighDpiScaling::isActive() ? systemLogicalDpi() : paintDeviceLogicalDpi(widget),
                                   widget->windowHandle());
}

QPoint qaxFromNativePosition(const QWidget *w, const QPoint &nativePos)
{
    const qreal factor = QHighDpiScaling::factor(w->windowHandle());
    return qFuzzyCompare(factor, 1)
        ? nativePos : (QPointF(nativePos) / factor).toPoint();
}

QPoint qaxNativeWidgetPosition(const QWidget *w)
{
    return qaxFromNativePosition(w, w->geometry().topLeft());
}

QSize qaxToNativeSize(const QWidget *w, const QSize &size)
{
    const qreal factor = QHighDpiScaling::factor(w->windowHandle());
    return qFuzzyCompare(factor, 1) ? size : (QSizeF(size) * factor).toSize();
}

QSize qaxNativeWidgetSize(const QWidget *w)
{
    return qaxToNativeSize(w, w->size());
}

QSize qaxFromNativeSize(const QWidget *w, const QSize &size)
{
    const qreal factor = QHighDpiScaling::factor(w->windowHandle());
    return qFuzzyCompare(factor, 1) ? size : (QSizeF(size) / factor).toSize();
}

RECT qaxNativeWidgetRect(const QWidget *w)
{
    return QHighDpiScaling::isActive()
        ? qaxQRect2Rect(QRect(qaxNativeWidgetPosition(w), qaxNativeWidgetSize(w)))
        : qaxQRect2Rect(w->geometry());
}

QRect qaxFromNativeRect(const RECT &r, const QWidget *w)
{
    const QRect qr = qaxRect2QRect(r);
    const qreal factor = QHighDpiScaling::factor(w->windowHandle());
    return qFuzzyCompare(factor, 1)
        ? qr
        : QRect((QPointF(qr.topLeft()) / factor).toPoint(), (QSizeF(qr.size()) / factor).toSize());
}

HRGN qaxHrgnFromQRegion(const QRegion &region, const QWidget *widget)
{
    return qaxHrgnFromQRegion(region, widget->windowHandle());
}

#endif // QT_WIDGETS_LIB

QT_END_NAMESPACE
