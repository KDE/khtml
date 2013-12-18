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
#ifndef PIXMAP_PLANE_H
#define PIXMAP_PLANE_H

#include "array2d.h"
#include "animprovider.h"
#include "imageplane.h"
#include "pixmaptile.h"

namespace khtmlImLoad {

class AnimProvider;

/**
 A pixmap plane is responsible for drawing data of an image plane
 @internal
*/
class PixmapPlane: public Plane
{
public:
    ImagePlane*         parent;
    int                 refCount;//For image's use
private:
    Array2D<PixmapTile> tiles;
public:
    PixmapPlane(unsigned int _width, unsigned int _height, ImagePlane* _parent):
            Plane(_width, _height), parent(_parent), tiles(tilesWidth, tilesHeight)
    {
        nextFrame    = 0;
        animProvider = 0;
        refCount     = 0;
    }

    PixmapPlane*  nextFrame;
    AnimProvider* animProvider;
    
    void flushCache();

    ~PixmapPlane()
    {
        delete animProvider;
        delete parent;
        delete nextFrame;
    }

    /**
     Paints a portion of the frame on the painter 'p' at dx and dy. 
     The source rectangle starts at sx, sy and has dimension width * height.
    */
    void paint(int dx, int dy, QPainter* p,
               int sx, int sy, int width = -1, int height = -1);
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
