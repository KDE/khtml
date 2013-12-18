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

#ifndef IMAGE_H
#define IMAGE_H

#include <QByteArray> 
#include <QSize>
#include <QTimer>
#include <QPair>
#include <QMap>

#include <khtml_settings.h>
 
#include "imageformat.h"

class QPainter;

namespace khtmlImLoad {

class ImageOwner;
class ImageLoader;
class PixmapPlane;

/**
 An image represents a static picture or an animation, that
 may be incrementally loaded.
*/
class Image
{
public:
    /**
     Creates an image with a given owner; the owner will be notified about the repaint event, the image geometry,
     and so on; and also if the image can not be decoded. The image must be fed data through the processData() 
     call; and it will take care of the rest automatically.
    */
    Image(ImageOwner* owner);
    
    /**
     Provides new data for decoding. The method will return false if there is no longer any use to feeding it more data 
     (i.e. if the image is complete, or broken, etc.); however, it is safe to do so.
    */
    bool processData(uchar* data, int length);
    
    /**
     Notifies the image that the data source is exhausted, in case it cares. This should be called at the 
     end of the data stream in order for non-incremental decoders to work
    */
    void processEOF();
    
    /**
     Cleans up
    */
    ~Image();
    
    /**
     Returns the image's size
    */
    QSize size() const;
    
    /**
     Returns true if the image has been fully loaded
    */
    bool complete() const;

    /**
     Returns true if the image may have an alpha channel
    */
    bool hasAlpha() const;

    /**
     Returns the image of basic content. Should be treated as READ ONLY.
     (but see CanvasImage)
    */
    QImage* qimage()  const;

    /**
     Enables or disables animations
    */
    void setShowAnimations(KHTMLSettings::KAnimationAdvice);
private:
    //Interface to the loader.
    friend class ImageLoader;
    
    /**
     Called from the loader to notify of canvas geometry.
     The loader must also call notifyAppendFrame to
     create 1 or more frames
    */
    void notifyImageInfo(int width, int height); 
    
    /**
     Called to notify of format of a frame
    */
    void notifyAppendFrame(int fwidth, int fheight, const ImageFormat& format);
    
    /**
     Called from the loader to feed a new scanline (in consecutive order in each frame), through
     various progressive versions
     */
    void notifyScanline(unsigned char version, unsigned char* data);
    
    /**
     Called from the loader to feed a new image, through
     various progressive versions
     */
    void notifyQImage(unsigned char version, const QImage* image);

    /**
     Called from loader to request the current contents of the line in the basic plane
     */
    void requestScanline(unsigned int lineNum, unsigned char* lineBuf);

    //### FIXME: restore the animprovider interface

private: //Interface to the painter.
    friend class ImagePainter;
    bool mayPaint() { return original; }
    void derefSize(QSize size);
    void refSize  (QSize size);
    PixmapPlane* getSize(QSize size);

protected:
    ImageOwner * owner;

    //Update reporting to owner
    friend class Updater;
    friend class AnimProvider;
    bool updatesPending;
    int updatesStartLine;
    int updatesEndLine;

    //Incorporate the scanline into update range
    void requestUpdate(int line);
    
    //Sets the state as not having updates
    void noUpdates();
    
    /**
     Called by the updater when the image should tell its owners about new changes
    */
    void notifyPerformUpdate();

    /**
     Called when animation frame changes, requesting the owner to repaint
    */
    void notifyFrameChange();

    //Loader stuff.
    QByteArray bufferPreDetect;
    ImageLoader* loader;
    PixmapPlane* loaderPlane;
    unsigned int loaderScanline;

    bool fullyDecoded;
    bool inError;

    //A little helper to set the error condition.
    void loadError();

    //Image state
    unsigned int width, height;
    PixmapPlane* original;
    QMap<QPair<int, int>, PixmapPlane*> scaled;
    KHTMLSettings::KAnimationAdvice animationAdvice;

};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
