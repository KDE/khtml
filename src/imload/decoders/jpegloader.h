#ifndef JPEG_LOADER_H
#define JPEG_LOADER_H

#include "imageloaderprovider.h"

namespace khtmlImLoad
{

class ImageLoader;

class JPEGLoaderProvider: public ImageLoaderProvider
{
public:
    Type type() Q_DECL_OVERRIDE;

    ImageLoader *loaderFor(const QByteArray &prefix) Q_DECL_OVERRIDE;
};

}

#endif
