/*
    Progressive image displaying library.

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

#include "imagepainter.h"
#include "image.h"
#include "pixmapplane.h"
#include "imagemanager.h"

namespace khtmlImLoad {

ImagePainter::ImagePainter(Image* _image):image(_image), sizeRefd(false)
{ //No need to ref, default size.
    size = image->size();
}

ImagePainter::ImagePainter(Image* _image, QSize _size):image(_image), size(_size), sizeRefd(false)
{
    if (!ImageManager::isAcceptableScaleSize(_size.width(), _size.height()))
        setDefaultSize();
}

ImagePainter::~ImagePainter()
{
    if (sizeRefd)
        image->derefSize(size);
}

ImagePainter::ImagePainter(const ImagePainter& src)
{
    image = src.image;
    size  = src.size;
    sizeRefd = false;
}

ImagePainter& ImagePainter::operator=(const ImagePainter& src)
{
    if (sizeRefd)
        image->derefSize(size);
    image = src.image;
    size  = src.size;
    sizeRefd = false;
    return *this;
}

void ImagePainter::setSize(QSize _size)
{
    if (!ImageManager::isAcceptableScaleSize(_size.width(), _size.height())) {
        setDefaultSize();
        return;
    }
    
    // Don't do anything if size didn't change, 
    // to avoid dropping the image..
    if (size == _size)
        return;

    if (sizeRefd)
        image->derefSize(size);
    size = _size;
    sizeRefd = false;
}

void ImagePainter::setDefaultSize()
{
    if (sizeRefd)
        image->derefSize(size);
    size = image->size();
    sizeRefd = false;
}

void ImagePainter::paint(int dx, int dy, QPainter* p, int sx, int sy,
                         int width, int height)
{
    if (!image->mayPaint())
        // ### fallback painting in case bg?
        return;

    // Do our lazy ref if needed. Safe due to above
    if (!sizeRefd && size != image->size())
    {
        image->refSize(size);
        sizeRefd = true;
    }

    PixmapPlane* plane = image->getSize(size);

    if (plane->animProvider)
    {
        // Clip the request ourselves when animating..
        
        if (width == -1)
            width  = size.width();
        if (height == -1)
            height = size.height();

        QRect clippedRect = QRect(0, 0, size.width(), size.height())
                            & QRect(sx, sy, width, height);
        plane->animProvider->paint(dx, dy, p, clippedRect.x(), clippedRect.y(),
                                          clippedRect.width(), clippedRect.height());
        return;
    }

    // non-animated, go straight to PixmapPlane; it clips itself
    plane->paint(dx, dy, p, sx, sy, width, height);
}

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
