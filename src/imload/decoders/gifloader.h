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
    Type type() Q_DECL_OVERRIDE;

    ImageLoader *loaderFor(const QByteArray &prefix) Q_DECL_OVERRIDE;
};

}

#endif
