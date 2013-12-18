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

#include "image.h"
#include "imageloader.h"
#include "imagemanager.h"
#include "imageowner.h"
#include "pixmapplane.h"
#include "rawimageplane.h"
#include "scaledimageplane.h"

#include <QPainter>
#include <limits.h>
#include <QDebug>

namespace khtmlImLoad {

Image::Image(ImageOwner* _owner)
{
    owner       = _owner;
    loader      = 0;
    loaderPlane = 0;
    original    = 0;
    loaderScanline = 0;

    fullyDecoded = false;
    inError      = false;

    width = height = 0;
    animationAdvice = KHTMLSettings::KAnimationEnabled;

    noUpdates();
}

Image::~Image()
{
    ImageManager::updater()->destroyed(this);
    delete   loader;
    delete   original;
    assert(scaled.isEmpty());
}

void Image::requestUpdate(int line)
{
    updatesStartLine = qMin(line, updatesStartLine);
    updatesEndLine   = qMax(line, updatesEndLine);
    if (!updatesPending)
    {
        updatesPending = true;
        ImageManager::updater()->haveUpdates(this);
    }
}

void Image::noUpdates()
{
    updatesPending   = false;
    updatesStartLine = INT_MAX;
    updatesEndLine   = 0;
}

void Image::notifyPerformUpdate()
{
    owner->imageChange(this, QRect(0, updatesStartLine,
                                   width, updatesEndLine - updatesStartLine + 1));
    noUpdates();
}

void Image::notifyFrameChange()
{
    owner->imageChange(this, QRect(0, 0, width, height));
}

void Image::loadError()
{
    inError = true;
    delete loader;
    loader = 0;

    //Make sure to call this last, since we may get deleted here.
    owner->imageError(this);
}

bool Image::processData(uchar* data, int length)
{
    if (inError)
        return false;

    //...if we don't have a loder
    if (!loader)
    {
        if (original)
        {
            //We could have already discarded it as we're all done. remind the caller about it
            return false;
        }
        else
        {
            //need to to do auto detection... so append all the data into a buffer
            int oldSize = bufferPreDetect.size();
            bufferPreDetect.resize(oldSize + length);
            memcpy(bufferPreDetect.data() + oldSize, data, length);

            //Attempt to create a loader
            loader = ImageManager::loaderDatabase()->loaderFor(bufferPreDetect);

            //if can't, return...
            if (!loader)
            {
                //if there is more than 4K of data,
                //and we see no use for it, abort.
                if (bufferPreDetect.size() > 4096)
                {
                    loadError();
                    return false;
                }
                return true;
            }

            loader->setImage(this);

            //All the data is now in the buffer
            length = 0;
        }
    }

    int pos = 0, stat = 0;

    //If we got this far, we have the loader.
    //just feed it any buffered data, and the new data.
    if (!bufferPreDetect.isEmpty())
    {
        do
        {
            stat = loader->processData(reinterpret_cast<uchar*>(bufferPreDetect.data() + pos),
                                           bufferPreDetect.size() - pos);
            if (stat == bufferPreDetect.size() - pos)
                break;

            pos += stat;
        }
        while (stat > 0);
        bufferPreDetect.resize(0);
    }

    if (length) //if there is something we did not feed from the buffer already..
    {
        pos = 0;
        do
        {
            stat = loader->processData(data + pos, length - pos);

            if (stat == length - pos)
                break;

            pos  += stat;
        }
        while (stat > 0);
    }

    //If we just finished decoding...
    if (stat == ImageLoader::Done)
    {
        fullyDecoded = true;
        owner->imageDone(this);
        return false;
    }

    if (stat == ImageLoader::Error)
    {
        loadError();
        return false;
    }

    return true; //Need more stuff
}

void Image::processEOF()
{
    if (inError) //Input error already - nothing to do
        return;

    //If no loader detected, and we're at EOF, it's an error
    if (!loader)
    {
        loadError();
        return;
    }

    //Otherwise, simply tell the loader, and check whether we decoded all right
    bool decodedOK = loader->processEOF() == ImageLoader::Done;

    //... and get rid of it
    delete loader;
    loader = 0;

    if (!decodedOK)
    {
        loadError();
    }
    else
    {
        if (original && original->animProvider)
            original->animProvider->setShowAnimations(animationAdvice);

        fullyDecoded = true;
        owner->imageDone(this);
    }
}


void Image::notifyImageInfo(int _width, int _height)
{
    if (!ImageManager::isAcceptableSize(_width, _height)) {
        qWarning() << "ImageLoader somehow fed us an illegal size, killing it!";
        loadError();
        return;
    }
    width  = _width;
    height = _height;

    owner->imageHasGeometry(this, width, height);
}

void Image::notifyAppendFrame(int fwidth, int fheight, const ImageFormat& format)
{
    if (!ImageManager::isAcceptableSize(fwidth, fheight)) {
        qWarning() << "ImageLoader somehow fed us an illegal size, killing it!";
        loadError();
        return;
    }

    //Create the new frame.
    QImage image = format.makeImage (fwidth, fheight);
    //IMPORTANT: we use image.width(), etc., below for security/paranoia
    //reasons -- so we e.g. end up with a size 0 image if QImage overflow
    //checks kick in, etc. This is on top of the policy enforcement
    //enough, in case someone breaks it or such
    RawImagePlane* iplane = new RawImagePlane(image.width(), image.height());
    iplane->image         = image;
    iplane->format        = format;
    PixmapPlane*   plane  = new PixmapPlane  (image.width(), image.height(), iplane);

    if (loaderPlane) //Had a previous plane
    {
        loaderPlane->nextFrame = plane;
        loaderPlane            = plane;
    }
    else
    {
        //Created the first one
        loaderPlane = original = plane;
    }

    //Go through the list of scaled sizes, and build frames for that.

    loaderScanline = 0;
}

static inline unsigned char premulComponent(unsigned original, unsigned alpha)
{
    unsigned product = original * alpha; // this is conceptually 255 * intended value.
    return (unsigned char)((product + product/256 + 128)/256);
}

void Image::notifyQImage(uchar version, const QImage *image)
{
    RawImagePlane* plane = static_cast<RawImagePlane*>(loaderPlane->parent);

    plane->image = *image;

    //Set the versions.
    for(unsigned int i=0; i<plane->height;i++) {
        plane->versions[i] = version;
    }

    updatesStartLine = 0;
    updatesEndLine   = plane->height;
    if (!updatesPending)
    {
        updatesPending = true;
        ImageManager::updater()->haveUpdates(this);
    }
}

void Image::notifyScanline(uchar version, uchar* data)
{
    RawImagePlane* plane = static_cast<RawImagePlane*>(loaderPlane->parent);
    if (loaderScanline >= plane->height)
        return;

    //Load the data in..
    if (plane->format.type != ImageFormat::Image_ARGB_32)
    {
        //Can just copy
        std::memcpy(plane->image.scanLine(loaderScanline), data,
            plane->format.depth() * plane->image.width());
    }
    else
    {
        //Premultiply. Note that this is assuming that any combination
        //Will not actually look at the pixel.
        QRgb* dst = reinterpret_cast<QRgb*>(plane->image.scanLine(loaderScanline));
        uchar* src = data;
        int planeImageWidth = plane->image.width();
        for (int x = 0; x < planeImageWidth; ++x)
        {
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
            unsigned a = src[0];
            unsigned r = src[1];
            unsigned g = src[2];
            unsigned b = src[3];
#else
            unsigned a = src[3];
            unsigned r = src[2];
            unsigned g = src[1];
            unsigned b = src[0];
#endif
            dst[x]  = qRgba(premulComponent(r, a), premulComponent(g, a),
                              premulComponent(b, a), a);
            src += 4;
        }
    }

    //Set the version.
    plane->versions[loaderScanline] = version;

    //calculate update region. Note that we ignore scaling when doing this, and just emit the
    //scaled version when delivering the event. It's easier this way, and we don't have to worry
    //about what happens to updates in case of change of scaling.
    //We only do this for the first frame --- other stuff will only
    //be full-frame switches from the animation controller
    if  (loaderPlane == original)
        requestUpdate(loaderScanline);

    loaderScanline++;
    if (loaderScanline == plane->height) //Next pass of progressive image
        loaderScanline = 0;
}

void Image::requestScanline(unsigned int lineNum, uchar* lineBuf)
{
    RawImagePlane* plane = static_cast<RawImagePlane*>(loaderPlane->parent);
    if (lineNum >= plane->height)
        return;

    std::memcpy(lineBuf, plane->image.scanLine(lineNum),
                plane->image.width() * plane->format.depth());
}

QSize Image::size() const
{
    return QSize(width, height);
}

bool Image::complete() const
{
    //### FIXME: this isn't quite right in case of animation
    //controller -- e.g. if animation is disabled, we only
    //care about frame 1.
    return fullyDecoded;
}

static QPair<int, int> trSize(QSize size)
{
    return qMakePair(size.width(), size.height());
}

PixmapPlane* Image::getSize(QSize size)
{
    // If we're empty, we use ourselves as a placeholder,
    // to avoid trying scaling a 0x0 image
    if (size == this->size() || this->size().isEmpty())
        return original;

    return scaled[trSize(size)];
}

void Image::derefSize(QSize size)
{
    assert(original);

    if (size == this->size() || this->size().isEmpty()) return;

    QPair<int, int> key = trSize(size);
    PixmapPlane* plane = scaled[key];
    --plane->refCount;
    if (plane->refCount == 0)
    {
        delete plane;
        scaled.remove(key);
    }
}

void Image::refSize(QSize size)
{
    assert(original);

    if (size == this->size() || this->size().isEmpty()) return;

    QPair<int, int> key = trSize(size);
    PixmapPlane* plane = scaled[key];
    if (plane)
    {
        ++plane->refCount;
    }
    else
    {
        // Note: ImagePainter enforces our maximum image size, much like the load code does,
        // so we don't have to worry about extreme size.

        // Compute scaling ratios. divide-by-zero should not happen
        // due to check above.
        double wRatio = size.width()  / double(width);
        double hRatio = size.height() / double(height);

        //Go through and make scaled planes for each position
        PixmapPlane* first = 0, *prev = 0;

        //### might need unification with ScaledImagePlane's size handling
        for (PixmapPlane* cur = original; cur; cur = cur->nextFrame)
        {
            int newWidth  = qRound(cur->width  * wRatio);
            int newHeight = qRound(cur->height * hRatio);

            // Make 100% sure we do it precisely for the original image
            if (cur->width == width)
                newWidth = size.width();
            if (cur->height == height)
                newHeight = size.height();

            // Ensure non-empty..
            if (newWidth  <= 0) newWidth  = 1;
            if (newHeight <= 0) newHeight = 1;

            ScaledImagePlane* splane = new ScaledImagePlane(
                    newWidth, newHeight,
                    static_cast<RawImagePlane*>(cur->parent));
            PixmapPlane*      plane  = new PixmapPlane(
                    newWidth, newHeight, splane);
            if (cur->animProvider)
                plane->animProvider = cur->animProvider->clone(plane);

            if (prev)
                prev->nextFrame = plane;
            else
                first           = plane;

            prev            = plane;
        }

        first->refCount = 1;
        scaled[key]     = first;
    }
}

QImage* Image::qimage() const
{
    if (!original || !original->parent)
        return 0;

    return &static_cast<RawImagePlane*>(original->parent)->image;
}

bool Image::hasAlpha() const
{
    if (!original || !original->parent)
        return false;
    return original->parent->format.hasAlpha();
}

void Image::setShowAnimations(KHTMLSettings::KAnimationAdvice newAdvice)
{
    if (animationAdvice != newAdvice)
    {
        animationAdvice = newAdvice;
        if (original && original->animProvider)
            original->animProvider->setShowAnimations(newAdvice);
    }
}

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
