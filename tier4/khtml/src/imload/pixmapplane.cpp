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

#include "pixmapplane.h"

#include <QPainter>


namespace khtmlImLoad {

void PixmapPlane::paint(int dx, int dy, QPainter* p,
                  int sx, int sy, int sWidth, int sHeight)
{
    //Do some basic clipping, discarding invalid requests and adjusting sizes of others.
    if (sy >= (int)height)
        return;
    if (sx >= (int)width)
        return;

    if (sWidth == -1)
        sWidth = width;

    if (sHeight == -1)
        sHeight = height;

    unsigned int ey = sy + sHeight - 1;
    if (ey > height - 1) 
        ey = height - 1;

    unsigned int ex = sx + sWidth - 1;
    if (ex > width - 1)
        ex = width - 1;

    sHeight = ey - sy + 1;
    sWidth  = ex - sx + 1;

    //Calculate the range of tiles to paint, in both directions
    unsigned int startTileY = sy / Tile::TileSize;
    unsigned int endTileY   = ey / Tile::TileSize;

    unsigned int startTileX = sx / Tile::TileSize;
    unsigned int endTileX   = ex / Tile::TileSize;

    //Walk through all the rows
    unsigned int paintY = dy;
    for (unsigned int tileY = startTileY; tileY <= endTileY; ++tileY)
    {
        //see how much we have to paint -- end points are different
        unsigned int startY = 0;
        unsigned int endY   = Tile::TileSize - 1;
        
        if (tileY == startTileY)
            startY = sy % Tile::TileSize;
            
        if (tileY == endTileY)
            endY   = ey % Tile::TileSize;
        
        unsigned int paintHeight = endY - startY + 1;
        
        //Now through some columns
        unsigned int paintX = dx;
        for (unsigned int tileX = startTileX; tileX <= endTileX; ++tileX)
        {
            //calculate the horizontal size. Some redundancy here, 
            //since these are the same for all rows, but I'd rather
            //avoid heap allocation or alloca..
            unsigned int startX = 0;
            unsigned int endX   = Tile::TileSize - 1;

            if (tileX == startTileX)
                startX = sx % Tile::TileSize;
                
            if (tileX == endTileX)
                endX   = ex % Tile::TileSize;
                
            int paintWidth = endX - startX + 1;

            //Update from image plane if need be
            PixmapTile& tile = tiles.at(tileX, tileY);
            if (!parent->isUpToDate(tileX, tileY, &tile))
                parent->ensureUpToDate(tileX, tileY, &tile);

            //Draw as much as we have
            if (tile.pixmap)
            {
                //Scan the versions to see how much to paint.
                unsigned int h = 0;
                for (int checkY = startY; checkY < Tile::TileSize && tile.versions[checkY]; ++checkY)
                    ++h;

                //Draw it, if there is anything (note: Qt would interpret 0 as everything)
                if (h)
                    p->drawPixmap(paintX, paintY, *tile.pixmap, startX, startY,
                                  paintWidth, qMin(h, paintHeight));
            }
            paintX += paintWidth;
        }
        paintY += paintHeight;
    }
}

void PixmapPlane::flushCache()
{
    parent->flushCache();
    for (unsigned tileX = 0; tileX < tilesWidth; ++tileX) {
        for (unsigned tileY = 0; tileY < tilesHeight; ++tileY) {
            PixmapTile& pixTile = tiles.at(tileX, tileY);
            if (pixTile.pixmap)
                ImageManager::pixmapCache()->removeEntry(&pixTile);
        }
    }
}

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
