#ifndef QIMAGEIO_LOADER_H
#define QIMAGEIO_LOADER_H

#include "imageloaderprovider.h"

namespace khtmlImLoad
{

class ImageLoader;

class QImageIOLoaderProvider: public ImageLoaderProvider
{
public:
    Type type() override;
    static const QStringList &mimeTypes();
    ImageLoader *loaderFor(const QByteArray &prefix) override;
};

}

#endif
