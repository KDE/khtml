/*
    KHTML image displaying library.

    Copyright (C) 2007 Maks Orlovich (maksim@kde.org)

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
#include "canvasimage.h"
#include "imageowner.h"
#include "pixmapplane.h"
#include "rawimageplane.h"
#include "scaledimageplane.h"

namespace khtmlImLoad {

class TrivialImageOwner : public ImageOwner
{
public:
    virtual void imageHasGeometry(Image*, int, int) {}
    virtual void imageChange(Image*, QRect) {}
    virtual void imageError(Image*) {};
    virtual void imageDone(Image*) {};
};

ImageOwner* CanvasImage::s_trivialOwner = 0;

ImageOwner* CanvasImage::trivialOwner()
{
    if (!s_trivialOwner)
        s_trivialOwner = new TrivialImageOwner;
    return s_trivialOwner;
}

void CanvasImage::setupOriginalPlane(int width, int height)
{
    fullyDecoded = true;
    this->width  = width;
    this->height = height;

    RawImagePlane* imgPlane = new RawImagePlane(width, height, 1 /*already "loaded"*/);
    imgPlane->format.type = ImageFormat::Image_ARGB_32;
    imgPlane->image = QImage(width, height, QImage::Format_ARGB32_Premultiplied);
    original = new PixmapPlane(width, height, imgPlane);

}

CanvasImage::CanvasImage(int width, int height): Image(trivialOwner())
{
    setupOriginalPlane(width, height);
}

void CanvasImage::contentUpdated()
{
    flushAllCaches();
}

void CanvasImage::flushAllCaches()
{
    // Flush all the planes, including any scaled ones, etc.
    original->flushCache();

    for (QMap<QPair<int, int>, PixmapPlane*>::iterator i = scaled.begin(); i != scaled.end(); ++i)
        i.value()->flushCache();
}

void CanvasImage::resizeImage(int width, int height)
{
    // Dump any cached info, it's useless
    flushAllCaches();

    // Create a new master pixmap and raw image planes
    delete original;
    setupOriginalPlane(width, height);
    RawImagePlane* imgPlane = static_cast<RawImagePlane*>(original->parent);

    // Now go through the scaling cache, and fix things up.
    for (QMap<QPair<int, int>, PixmapPlane*>::iterator i = scaled.begin(); i != scaled.end(); ++i) {
        PixmapPlane* scaledPix = i.value();
        delete scaledPix->parent;
        scaledPix->parent = new ScaledImagePlane(scaledPix->width, scaledPix->height, imgPlane);
    }
}

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
