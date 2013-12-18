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
#ifndef PLANE_H
#define PLANE_H

#include "tile.h"

namespace khtmlImLoad {

/**
 All picture data, whether image or pixmap-related, logically
 denotes a plane: logically, some collection of tiles, which has
 some geometry. This class incorporates this abstraction, and
 provides helper methods for dealing with the tiles
*/
class Plane
{
public:
    unsigned int width;  //width  in pixels
    unsigned int height; //height in pixels
    unsigned int tilesWidth;  //width  in tiles
    unsigned int tilesHeight; //height in tiles
    
    Plane(unsigned int _width, unsigned int _height)
    {
        width   = _width;
        height  = _height;
        tilesWidth  = (width  + Tile::TileSize - 1)/Tile::TileSize;
        tilesHeight = (height + Tile::TileSize - 1)/Tile::TileSize;
    }
    
    unsigned int tileWidth(unsigned int tileX)
    {
        if (tileX == (tilesWidth - 1))
            return width - tileX * Tile::TileSize;
        return Tile::TileSize;
    }
    
    unsigned int tileHeight(unsigned int tileY)
    {
        if (tileY == (tilesHeight - 1))
            return height - tileY * Tile::TileSize;
        return Tile::TileSize;
    }
    
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
