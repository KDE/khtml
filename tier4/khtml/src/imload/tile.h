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

#ifndef TILE_H
#define TILE_H

#include <cstring>

#include <QDebug>

namespace khtmlImLoad {

class TileCache;
class TileCacheNode;

/**
 We hold pointers tiles in the cache. The interface is simple: when they get
 kicked out of the cache, the discard method is invoked to have the associated
 resources freed. Note that the cache does not own the entries, but merely
 mages the more expensive part of their resources. The cacheEntry
 field is used to link the tile node to the appropriate cache entry for it.
*/
class Tile
{
public:
    enum {TileSize = 64}; 
    //Note:this can be safely reduced, but not increased --- ScaledPlane
    //relies for byte offsets in a single row in a tile fitting into bytes

    unsigned char versions[Tile::TileSize];
protected:
    friend class TileCache;

    Tile(): cacheNode(0)
    {
        std::memset(versions, 0, Tile::TileSize);
    }
    virtual ~Tile(){}
    virtual void discard() = 0;

    TileCacheNode* cacheNode;
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
