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
#ifndef RAW_IMAGE_PLANE_H
#define RAW_IMAGE_PLANE_H

#include "imageplane.h"
#include <cstring>

namespace khtmlImLoad {

/**
 A raw image plane merely contains a QImage.
*/
class RawImagePlane: public ImagePlane
{
public:
    QImage image;
    unsigned char* versions; //Versions of scanlines --- node that this is is padded to be of width / by 64

    RawImagePlane(unsigned int _width, unsigned int _height, char initialVer = 0):
        ImagePlane(_width, _height)
    {
        versions = new unsigned char[tilesHeight * Tile::TileSize];
        std::memset(versions, initialVer, tilesHeight * Tile::TileSize);
    }

    ~RawImagePlane()
    {
        delete[] versions;
    }
    
    virtual void flushCache()
    {} // Nothing caches

    /**
     Returns true if the given pixmap tile is up-to-date.
     Note that this should compare the information in the pixmap with ideal the
     up-to-date image using the decoding information thus far, and not
     with the state of the image proper. (Which might not even be in memory)
    */
    virtual bool isUpToDate(unsigned int tileX, unsigned int tileY,
                            PixmapTile* tile);

    /**
     Ensures that the given pixmap tile is up-to-date.
    */
    virtual void ensureUpToDate(unsigned int tileX, unsigned int tileY,
                            PixmapTile* tile);

};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
