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

#ifndef IMAGE_LOADER_PROVIDER_H
#define IMAGE_LOADER_PROVIDER_H

#include <QByteArray>

namespace khtmlImLoad {

class ImageLoader;
/**
To register new image formats, new copies of ImageLoaderProvider's must be 
created and registered with ImageManager::loaderDatabase(). 
*/
class ImageLoaderProvider
{
public:
    virtual ~ImageLoaderProvider(){}
    enum Type
    {
        Efficient,
        Foreign
    };
    
    /**
     Returns the type of the loader. An "efficient" loader does not duplicate any data, and will therefore be preferred;
     while a "foreign" loader has to duplicate a large amount of image data to fit in w/the original 
     framework, and should therefore only be used when a better loader is not available
    */
    virtual Type type() = 0;

    
    /**
     Creates a loader for an image format that can decode a file starting with given data, or 0 if that's not possible.
    */
    virtual ImageLoader* loaderFor(const QByteArray& prefix) = 0;
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
