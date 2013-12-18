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

#ifndef IMAGE_MANAGER_H
#define IMAGE_MANAGER_H

#include "animtimer.h"
#include "loaderdatabase.h"
#include "tilecache.h"
#include "updater.h"

class QPixmap;

namespace khtmlImLoad {

class ImageManager
{
private:
    static AnimTimer* anmTimer;
    static TileCache* imgCache;
    static TileCache* pixCache;
    static LoaderDatabase* loaderDB;
    static Updater*        theUpdater;
    static QPixmap*        emptyPix;
    
    static unsigned int pixmapCacheSize();
    static unsigned int imageCacheSize();
    
    static void initLoaders();
public:
    // IMPORTANT: Don't even think about changing these to signed; it's security-critical
    // This method determines whether we'll ever accept an image so large
    static bool isAcceptableSize(unsigned width, unsigned height);
    
    // IMPORTANT: Don't even think about changing these to signed; it's security-critical
    // Says whether the target size is OK to scale an image too. This is much 
    // bigger than the above, as we store the actual data in the tile cache, so 
    // we just need some control information, which is much smaller
    static bool isAcceptableScaleSize(unsigned width, unsigned height);

    static AnimTimer* animTimer()
    {
        if (!anmTimer)
            anmTimer = new AnimTimer();
        return anmTimer;
    }

    static TileCache* imageCache() 
    {
        if (!imgCache)
            imgCache = new TileCache(imageCacheSize());
        return imgCache;
    }
    
    static TileCache* pixmapCache()
    {
        if (!pixCache)
            pixCache = new TileCache(pixmapCacheSize());
        return pixCache;
    }
    
    static Updater* updater()
    {
        if (!theUpdater)
            theUpdater = new Updater();
        return theUpdater;
    }
    
    static LoaderDatabase* loaderDatabase()
    {
        if (!loaderDB)
        {
            loaderDB = new LoaderDatabase();
            initLoaders(); //Register built-in decoders
        }
        return loaderDB;
    }
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
