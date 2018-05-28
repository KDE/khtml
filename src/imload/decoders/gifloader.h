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
    Type type() override;

    ImageLoader *loaderFor(const QByteArray &prefix) override;
};

}

#endif
