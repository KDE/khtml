#ifndef PNG_LOADER_H
#define PNG_LOADER_H

#include "imageloaderprovider.h"

namespace khtmlImLoad
{

class ImageLoader;

class PNGLoaderProvider: public ImageLoaderProvider
{
public:
    Type type() override;

    ImageLoader *loaderFor(const QByteArray &prefix) override;
};

}

#endif
