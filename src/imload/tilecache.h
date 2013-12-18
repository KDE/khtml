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

#ifndef TILE_CACHE_H
#define TILE_CACHE_H

#include <assert.h>

#include "tile.h"

namespace khtmlImLoad {


class TileCacheNode
{
public:
    //Interface to the cache LRU chains
    TileCacheNode* cacheNext;
    TileCacheNode* cachePrev;
    
    void unlink()
    {
        cacheNext->cachePrev = cachePrev;
        cachePrev->cacheNext = cacheNext;
        cacheNext = 0;
        cachePrev = 0;
    }
    
    void linkBefore(TileCacheNode* node)
    {
        this->cacheNext = node;
        this->cachePrev = node->cachePrev;
        
        node->cachePrev = this;
        this->cachePrev->cacheNext = this;
    }

    Tile* tile;
    
    TileCacheNode(): cacheNext(0), cachePrev(0), tile(0)
    {}
};


/** 
 An LRU-replacement cache for tiles.
*/
class TileCache
{
public:
    typedef TileCacheNode Node;
    

    Node* poolHead;

    //### TODO: consider smarter allocation for these?.
    Node* create()
    {
        if (poolHead)
        {
            Node* toRet = poolHead;
            poolHead    = poolHead->cacheNext;
            return toRet;
        }
        else
            return new Node();
    }
    
    void release(Node* entry)
    {
        //### TODO: Limit ??
        entry->cacheNext = poolHead;
        poolHead         = entry;
    }

private:
    int sizeLimit;
    int size;
    
    /**
     We keep tiles in a double-linked list, with the most recent one being at the rear
    */
    Node* front;
    Node* rear;

    /**
     Discard the node, and removes it from the list. Does not free the node.
    */
    void doDiscard(Node* node)
    {
        assert(node->tile->cacheNode == node);
        node->tile->discard();
        node->tile->cacheNode = 0;
        node->unlink();
        --size;
        assert(size >= 0);
    }
public:

    TileCache(int _sizeLimit):sizeLimit(_sizeLimit), size(0)
    {
        front = new Node;
        rear  = new Node;
        
        front->cacheNext = rear;
        rear ->cachePrev = front;
        
        poolHead = 0;
    }
    
    /**
     Add an entry to the cache. 
     */
    void addEntry(Tile* tile)
    {
        assert (tile->cacheNode == 0);

        Node* node;
        
        //Must have room
        if (size >= sizeLimit)
        {
            //Remove the front entry, reuse it
            //for the new node
            node = front->cacheNext;
            doDiscard(node);
        }
        else
            node = create();

        //Link in before the end sentinel, i.e. at the very last spot, increment usage count
        node->tile            = tile;
        tile->cacheNode       = node;
        node->linkBefore(rear);
        size++;
    }
    
    
    /**
     "Touches" the entry, making it the most recent 
     (i.e. moves the entry to the rear of the chain)
     */
    void touchEntry(Tile* tile)
    {
        Node* node = tile->cacheNode;
        node->unlink();
        node->linkBefore(rear);
    }
    
    /**
     Removes the entry from the cache, discards it.
     */
    void removeEntry(Tile* tile)
    {
        Node* node = tile->cacheNode;
        doDiscard(node);

        release(node);
    }
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
