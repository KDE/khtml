#ifndef PNG_LOADER_H
#define PNG_LOADER_H

#include "imageloaderprovider.h"

namespace khtmlImLoad
{

class ImageLoader;

class PNGLoaderProvider: public ImageLoaderProvider
{
public:
    virtual Type type();

    virtual ImageLoader *loaderFor(const QByteArray &prefix);
};

}

#endif
