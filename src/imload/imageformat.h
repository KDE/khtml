/*
    Large image displaying library.

    Copyright (C) 2004,2005 Maks Orlovich (maksim@kde.org)

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

#ifndef IMAGE_FORMAT_H
#define IMAGE_FORMAT_H

#include <QColor>
#include <QVector>
#include <QImage>
#include <assert.h>

namespace khtmlImLoad {

struct ImageFormat
{
    ImageFormat(): type(Image_INVALID)
    {}

    enum Type
    {
        Image_RGB_32,     // 32-bit RGB
        Image_ARGB_32,    // 32-bit ARGB; this will be premultiplied upon reception
                          // so should not be used in combination with requestScanline
        Image_ARGB_32_DontPremult,  // 32-bit ARGB that will be kept as-is
                                         // should only be used for interlaced images
                                         // as it is slower
        Image_Palette_8,   //8-bit paletted image
        Image_INVALID
    } type;

    int depth() const
    {
        switch (type)
        {
        case Image_RGB_32:
        case Image_ARGB_32:
        case Image_ARGB_32_DontPremult:
            return 4;
        default:
            return 1;
        }
    }

    QImage makeImage(int width, int height) const
    {
        QImage toRet;
        switch (type)
        {
        case Image_RGB_32:
            toRet = QImage(width, height, QImage::Format_RGB32);
            break;
        case Image_ARGB_32:
            toRet = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
            break;
        case Image_ARGB_32_DontPremult:
            toRet = QImage(width, height, QImage::Format_ARGB32);
            break;
        case Image_Palette_8:
            toRet = QImage(width, height, QImage::Format_Indexed8);
            // Make sure we're completely robust if the loader is buggy..
            while (palette.size() < 256)
                palette.append(0);
            toRet.setColorTable(palette);
            break;
        default:
            assert(false);
        }

        return toRet;
    }

    bool hasAlpha() const
    {
        return  (type == Image_ARGB_32 || type == Image_ARGB_32_DontPremult);
    }

    mutable QVector<QRgb> palette;

    //A helper for setting up a format descriptor for 8-bit grayscale
    void greyscaleSetup()
    {
        palette.clear();
        for (int i=0; i<256; i++)
            palette.append(qRgb(i,i,i));
        type = ImageFormat::Image_Palette_8;
    }
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
