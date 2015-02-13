#ifndef QIMAGEIO_LOADER_H
#define QIMAGEIO_LOADER_H

#include "imageloaderprovider.h"

namespace khtmlImLoad
{

class ImageLoader;

class QImageIOLoaderProvider: public ImageLoaderProvider
{
public:
    Type type() Q_DECL_OVERRIDE;
    static const QStringList &mimeTypes();
    ImageLoader *loaderFor(const QByteArray &prefix) Q_DECL_OVERRIDE;
};

}

#endif
