/*
    Large image displaying library.

    Copyright (C) 2004 Maks Orlovich (maksim@kde.org)

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

#ifndef IMAGE_OWNER_H
#define IMAGE_OWNER_H

namespace khtmlImLoad {

/**
 The users of Image's need to inherit off ImageOwner, in order to receive the information about
 their geometry, needs to repaint due to animation and progressive loading, etc.
 */
class ImageOwner
{
public:
    virtual ~ImageOwner() {}
    /**
     Called to notify the owner when the intrinic size is available
    */
    virtual void imageHasGeometry(Image* img, int width, int height) = 0;

    /**
     Called to notify the owner that a portion has changed
     */
    virtual void imageChange(Image* img, QRect region) = 0;

    /**
     Called to notify the owner that the image is broken
    */
    virtual void imageError(Image* img) = 0;

    /**
     Called to notify the owner that the image is done
    */
    virtual void imageDone(Image* img) = 0;
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
