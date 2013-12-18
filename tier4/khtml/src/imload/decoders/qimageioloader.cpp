/*
    Large image load library -- QImageIO decoder

    Copyright (C) 2007-2009 Allan Sandfeld Jensen <sandfeld@kde.org>

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
    AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <QBuffer>
#include <QByteArray>
#include <QStringList>
#include <QImageReader>

#include "qimageioloader.h"
#include "imageloader.h"
#include "kservice.h"
#include "kservicetypetrader.h"
#include "QDebug"
#include "imagemanager.h"

namespace khtmlImLoad {

class QImageIOLoader: public ImageLoader
{
    QByteArray array;
    QImage image;
public:
    QImageIOLoader()
    {
    }

    ~QImageIOLoader()
    {
    }

    virtual int processData(uchar* data, int length)
    {
        //Collect data in the buffer
        int pos = array.size();
        array.resize(array.size() + length);
        memcpy(array.data() + pos, data, length);
        return length;
    }

    virtual int processEOF()
    {
        QBuffer buffer(&array);
        buffer.open(QIODevice::ReadOnly);

        QByteArray qformat = QImageReader::imageFormat(&buffer);
        QImageReader reader(&buffer, qformat);

        if (!reader.canRead()) {
            return Error;
        }

        QSize size = reader.size();
        if (size.isValid()) {
            if (ImageManager::isAcceptableSize(size.width(), size.height()))
                notifyImageInfo(size.width(), size.height());
            else
                return Error;
        }

        if (!reader.read(&image)) {
            return Error;
        }

        if (!size.isValid()) {
            // Might be too late by now..
            if (ImageManager::isAcceptableSize(image.width(), image.height()))
                notifyImageInfo(image.width(), image.height());
            else
                return Error;
        }

        ImageFormat format;
        if (!imageFormat(image, format)) {
            return Error;
        }
        notifyAppendFrame(image.width(), image.height(), format);

        notifyQImage(1, &image);

        return Done;
    }
    bool imageFormat(QImage &image, ImageFormat &format) {
        switch(image.format()) {
        case QImage::Format_RGB32:
            format.type  = ImageFormat::Image_RGB_32;
            break;
        case QImage::Format_ARGB32:
            format.type  = ImageFormat::Image_ARGB_32_DontPremult;
            break;
        case QImage::Format_ARGB32_Premultiplied:
            format.type  = ImageFormat::Image_ARGB_32;
            break;
        case QImage::Format_Indexed8:
            format.type  = ImageFormat::Image_Palette_8;
            format.palette = image.colorTable();
            break;
        case QImage::Format_Mono:
        case QImage::Format_MonoLSB:
            image = image.convertToFormat(QImage::Format_Indexed8);
            format.type  = ImageFormat::Image_Palette_8;
            format.palette = image.colorTable();
            break;
        case QImage::Format_Invalid:
        default:
            // unsupported formats
            return false;
        }
        return true;
    }
};

static const char* const positiveList[] = {
    "BMP", "TIFF", "JP2", "PNM", "EXR", "XBM", "XPM", "ICO", 0
};

bool isSupportedFormat(QString format) {
    QStringList pList;
    for(int i=0; positiveList[i]; i++) {
        pList.append(QString::fromLatin1(positiveList[i]));
    }

    return pList.contains(format, Qt::CaseInsensitive);
}

static QStringList s_formats;

ImageLoaderProvider::Type QImageIOLoaderProvider::type()
{
    return Foreign;
}

const QStringList& QImageIOLoaderProvider::mimeTypes()
{
    if (!s_formats.isEmpty()) return s_formats;

//     QList<QByteArray> formats = QImageIOReader::supportedFormats();
    KService::List services = KServiceTypeTrader::self()->query("QImageIOPlugins");

    foreach(const KService::Ptr &service, services) {
        QStringList formats = service->property("X-KDE-ImageFormat").toStringList();
        QString mimetype = service->property("X-KDE-MimeType").toString();
        bool positive = false;
        foreach(const QString &format, formats) {
            if (isSupportedFormat(format)) {
                positive = true;
                break;
            }
        }
        if (!positive) continue;
        if (!mimetype.isEmpty()) {
            s_formats.append(mimetype);
            // qDebug() << "QImageIO - Format supported: " << mimetype << endl;
        }
    }
    return s_formats;
}

ImageLoader* QImageIOLoaderProvider::loaderFor(const QByteArray& prefix)
{
    QByteArray pref = prefix;
    QBuffer prefixBuffer(&pref);
    prefixBuffer.open(QIODevice::ReadOnly);
    QByteArray format = QImageReader::imageFormat(&prefixBuffer);
    prefixBuffer.close();
    if (format.isEmpty() || !isSupportedFormat(format))
        return 0;
    else
        // qDebug() << "QImageIO - Format guessed: " << format << endl;

    return new QImageIOLoader;
}

} // namespace
