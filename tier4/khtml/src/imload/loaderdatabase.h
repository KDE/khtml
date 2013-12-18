/*
    Large image displaying library.

    Copyright (C) 2004 Maks Orlovich (maksim@kde.org)

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
    AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
    AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
    CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#ifndef LOADER_DATABASE_H
#define LOADER_DATABASE_H

#include "imageloaderprovider.h"
#include "qimageioloader.h"
#include <QStringList>
#include <QVector>

namespace khtmlImLoad {

class ImageLoader;

class LoaderDatabase
{
public:
    LoaderDatabase() {}

    unsigned int numLoaders()
    {
        return efficientProviders.size() + foreignProviders.size();
    }

    QStringList supportedMimeTypes() {
        //### FIXME/TODO: this should be more dynamic
        QStringList ret;
        ret << QLatin1String("image/jpg") << QLatin1String("image/jpeg")
            << QLatin1String("image/png") << QLatin1String("image/gif")
            << QImageIOLoaderProvider::mimeTypes();
        return ret;
    }

    void registerLoaderProvider(ImageLoaderProvider* provider)
    {
        if (provider->type() == ImageLoaderProvider::Efficient)
            efficientProviders.append(provider);
        else
            foreignProviders.append(provider);
    }

    ImageLoader* loaderFor(const QByteArray& prefix)
    {
        ImageLoader* toRet;
        for (int c = 0; c < efficientProviders.size(); c++)
        {
            toRet = efficientProviders[c]->loaderFor(prefix);
            if (toRet)
                return toRet;
        }

        for (int c = 0; c < foreignProviders.size(); c++)
        {
            toRet = foreignProviders[c]->loaderFor(prefix);
            if (toRet)
                return toRet;
        }

        return 0;
    }

    ~LoaderDatabase()
    {
        for (int c = 0; c < efficientProviders.size(); c++)
            delete efficientProviders[c];

        for (int c = 0; c < foreignProviders.size(); c++)
            delete foreignProviders[c];
    }
private:
    QVector<ImageLoaderProvider*> efficientProviders;
    QVector<ImageLoaderProvider*> foreignProviders;
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
