#ifndef PNG_LOADER_H
#define PNG_LOADER_H

#include "imageloaderprovider.h"

namespace khtmlImLoad
{

class ImageLoader;

class PNGLoaderProvider: public ImageLoaderProvider
{
public:
    Type type() Q_DECL_OVERRIDE;

    ImageLoader *loaderFor(const QByteArray &prefix) Q_DECL_OVERRIDE;
};

}

#endif
