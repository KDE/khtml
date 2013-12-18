/*
    Large image displaying library.

    Copyright (C) 2004,2005 Maks Orlovich (maksim@kde.org)
    Copyright (C) 2012 Martin Sandsmark (martin.sandsmark@kde.org)

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
#ifndef SCALED_IMAGE_PLANE_H
#define SCALED_IMAGE_PLANE_H

#include "array2d.h"
#include "imageplane.h"
#include "rawimageplane.h"
#include "imagetile.h"

namespace khtmlImLoad {

/**
 A scaled image plane pulls data from a RawImagePlane and resizes it
*/
class ScaledImagePlane: public ImagePlane
{
public:
    virtual ~ScaledImagePlane();

    virtual void flushCache();

    ScaledImagePlane(unsigned int _width, unsigned int _height, RawImagePlane* _parent);


    virtual bool isUpToDate(unsigned int tileX, unsigned int tileY,
                            PixmapTile* tile);

    virtual void ensureUpToDate(unsigned int tileX, unsigned int tileY,
                            PixmapTile* tile);
private:
    RawImagePlane*     parent;
    Array2D<ImageTile> tiles;
    int m_width, m_height;
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
