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
#include "scaledimageplane.h"
#include "imagemanager.h"

namespace khtmlImLoad {
ScaledImagePlane::ScaledImagePlane(unsigned int _width, unsigned int _height, RawImagePlane* _parent):
    ImagePlane(_width, _height), parent(_parent), tiles(tilesWidth, tilesHeight), m_width(_width), m_height(_height)
{
}
    
bool ScaledImagePlane::isUpToDate(unsigned int tileX, unsigned int tileY,
                            PixmapTile* tile)
{
    Q_UNUSED(tileX);
    if (!tile->pixmap) return false;

    int yStep = (parent->image.height()<<8) / height;
    for (unsigned int line = 0, srcLine=0; line < tileHeight(tileY); ++line, srcLine += yStep)
    {
        if (tile->versions[line] < parent->versions[(srcLine+yStep)>>8])
            return false;
    }

    return true;
}

// Trick from here: http://www.compuphase.com/graphic/scale3.htm
#define AVG(a,b)  ( ((((a)^(b)) & 0xfefefefeUL) >> 1) + ((a)&(b)) )
static void scaleLoop32bit(QImage* dst, const QImage& src, int tileX, int tileY,
        int height, int width, unsigned char *versions, unsigned char *parentVersions)
{
    int yStep = (src.height()<<8) / height; // We bitshift for when we get large ratios
    int xStep = (src.width()<<8) / width;
    for (int yTarget=0, ySource=tileY * Tile::TileSize * yStep; yTarget<dst->height(); yTarget++, ySource+=yStep) {
        const quint32 *srcBelow = reinterpret_cast<const quint32*>(src.scanLine(ySource>>8));
        const quint32 *srcAbove = reinterpret_cast<const quint32*>(src.scanLine((ySource+yStep/2)>>8));
        quint32 *destination = reinterpret_cast<quint32*>(dst->scanLine(yTarget));
            
        versions[yTarget] = parentVersions[ySource>>8];
        for (int xTarget=0, xSource=tileX * Tile::TileSize * xStep; xTarget<dst->width(); xTarget++, xSource+=xStep) {
            quint32 average1 = AVG(srcBelow[xSource>>8], srcAbove[(xSource+xStep/2)>>8]);
            quint32 average2 = AVG(srcBelow[(xSource+xStep/2)>>8], srcAbove[xSource>>8]);
            destination[xTarget] = AVG(average1, average2);
        }
    }
}

static void scaleLoop8bit(QImage* dst, const QImage& src, int tileX, int tileY,
        int height, int width, unsigned char *versions, unsigned char *parentVersions)
{
    int yStep = (src.height()<<8) / height; // We bitshift for when we get large ratios
    int xStep = (src.width()<<8) / width;
    for (int yTarget=0, ySource=tileY * Tile::TileSize * yStep; yTarget<dst->height(); yTarget++, ySource+=yStep) {
        const quint8 *srcPix = reinterpret_cast<const quint8*>(src.scanLine(ySource>>8));
        quint8 *destination = reinterpret_cast<quint8*>(dst->scanLine(yTarget));
        versions[yTarget] = parentVersions[ySource>>8];
        for (int xTarget=0, xSource=tileX * Tile::TileSize * xStep; xTarget<dst->width(); xTarget++, xSource+=xStep) {
            destination[xTarget] = srcPix[xSource>>8];
        }
    }
}

void ScaledImagePlane::flushCache()
{
    for (unsigned tileX = 0; tileX < tilesWidth; ++tileX) {
        for (unsigned tileY = 0; tileY < tilesHeight; ++tileY) {
            ImageTile& imageTile = tiles.at(tileX, tileY);
            if (!imageTile.image.isNull())
                ImageManager::imageCache()->removeEntry(&imageTile);
        }
    }
}

void ScaledImagePlane::ensureUpToDate(unsigned int tileX, unsigned int tileY,
                            PixmapTile* tile)
{
    ImageTile& imageTile = tiles.at(tileX, tileY);

    //Create the image if need be.
    if (imageTile.image.isNull())
    {
        imageTile.image = parent->format.makeImage(tileWidth (tileX),
                                                   tileHeight(tileY));
        ImageManager::imageCache()->addEntry(&imageTile);
        std::memset(imageTile.versions, 0, Tile::TileSize);
    }
    else ImageManager::imageCache()->touchEntry(&imageTile);

    // Do the actual scaling
    if (parent->format.depth() == 1)
        scaleLoop8bit(&imageTile.image, parent->image, tileX, tileY, m_height, m_width, imageTile.versions, parent->versions);
    else
        scaleLoop32bit(&imageTile.image, parent->image, tileX, tileY, m_height, m_width, imageTile.versions, parent->versions);

    //Now, push stuff into the pixmap.
    updatePixmap(tile, imageTile.image, tileX, tileY, 0, 0, imageTile.versions);
}

ScaledImagePlane::~ScaledImagePlane()
{
}


}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
