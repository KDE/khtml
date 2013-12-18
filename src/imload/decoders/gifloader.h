#ifndef GIF_LOADER_H
#define GIF_LOADER_H

#include "imageloaderprovider.h"

namespace khtmlImLoad
{

class ImageLoader;

class GIFLoaderProvider: public ImageLoaderProvider
{
public:
    virtual ~GIFLoaderProvider() {}
    virtual Type type();

    virtual ImageLoader *loaderFor(const QByteArray &prefix);
};

}

#endif
