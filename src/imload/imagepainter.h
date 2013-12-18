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

#ifndef IMAGE_PAINTER_H
#define IMAGE_PAINTER_H

class QPainter;

#include <QSize>

namespace khtmlImLoad {

class Image;

/**
 An image painter let's one paint an image at the given size.
*/
class ImagePainter
{
public:
    /**
     Creates an image painter for the given image...
    */
    ImagePainter(Image* image);

    /**
     Creates an image painter for the given image,
     of given size
    */
    ImagePainter(Image* image, QSize size);

    ImagePainter(const ImagePainter& src);
    ImagePainter& operator=(const ImagePainter& src);
    

    /**
     Sets the size of the image.
    */
    void setSize(QSize size);

    /**
     Sets the image to the default size
    */
    void setDefaultSize();
    
    /**
     Cleans up
    */
    ~ImagePainter();
    
    /**
     Paints a portion of the image frame on the painter 'p' at dx and dy.
     The source rectangle starts at sx, sy and has dimension width * height
    */
    void paint(int dx, int dy, QPainter* p, int sx = 0, int sy = 0,
               int width = -1, int height = -1);
private:
    // Note: we actually request/ref scaled version from Image
    // lazily. This is because we may be constructed before there
    // is anything to scale, so we check when painting..
    Image* image;
    QSize  size;
    bool   sizeRefd;
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
