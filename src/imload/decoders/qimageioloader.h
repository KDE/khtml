#ifndef QIMAGEIO_LOADER_H
#define QIMAGEIO_LOADER_H

#include "imageloaderprovider.h"

namespace khtmlImLoad {

class ImageLoader;

class QImageIOLoaderProvider: public ImageLoaderProvider
{
public:
    virtual Type type();
    static const QStringList& mimeTypes();
    virtual ImageLoader* loaderFor(const QByteArray& prefix);
};

}

#endif
