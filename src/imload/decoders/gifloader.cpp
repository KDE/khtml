/*
    Large image load library -- GIF decoder

    Copyright (C) 2004 Maksim Orlovich <maksim@kde.org>
    Based almost fully on animated gif playback code,
         (C) 2004 Daniel Duley (Mosfet) <dan.duley@verizon.net>
    
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

#include "gifloader.h"

#include "animprovider.h"

#include "imageloader.h"
#include "imagemanager.h"
#include "pixmapplane.h"
#include "updater.h"

#include <QByteArray>
#include <QPainter>
#include <QVector>
#include <QDebug>

#include <stdlib.h>

extern "C" {
#include <gif_lib.h>
}

/* avoid cpp warning about undefined macro, old giflib had no GIFLIB_MAJOR */
#ifndef GIFLIB_MAJOR
#define GIFLIB_MAJOR 4
#endif

// #define DEBUG_GIFLOADER

namespace khtmlImLoad {

static int INTERLACED_OFFSET[] = { 0, 4, 2, 1 };
static int INTERLACED_JUMP  [] = { 8, 8, 4, 2 };


enum GIFConstants
{
    //Graphics control extension has most of animation info
    GCE_Code                = 0xF9,
    GCE_Size                = 4,
    //Fields of the above
    GCE_Flags               = 0,
    GCE_Delay               = 1,
    GCE_TransColor          = 3,
    //Contents of mask
    GCE_DisposalMask        = 0x1C,
    GCE_DisposalUnspecified = 0x00,
    GCE_DisposalLeave       = 0x04,
    GCE_DisposalBG          = 0x08,
    GCE_DisposalRestore     = 0x0C,
    GCE_UndocumentedMode4   = 0x10,
    GCE_TransColorMask      = 0x01
};

struct GIFFrameInfo
{
    bool         trans;
    QColor       bg;
    QRect        geom;
    unsigned int delay;
    char         mode;
    //###
};

/**
 An anim provider for the animated GIFs. We keep a backing store for
 the screen.
*/
class GIFAnimProvider: public AnimProvider
{
protected:
    QVector<GIFFrameInfo> frameInfo;
    int                   frame; // refers to the /current/ frame

    // State of gif screen after the previous image.
    // we paint the current frame on top of it, and don't touch until 
    // the current frame is disposed
    QPixmap               canvas;
    QColor                bgColor;
    bool                  firstTime;

    // Previous mode being background seems to trigger an OpSrc rather than OpOver updating
    bool                  previousWasBG;
public:
    GIFAnimProvider(PixmapPlane* plane, Image* img, QVector<GIFFrameInfo> _frames, QColor bg):
        AnimProvider(plane, img), bgColor(bg), firstTime(true), previousWasBG(false)
    {
        frameInfo = _frames;
        frame     = 0;
        canvas    = QPixmap(img->size());
        canvas.fill(bgColor);
    }

    // Renders a portion of the current frame's image on the painter..
    void renderCurImage(int dx, int dy, QPainter* p, int sx, int sy, int width, int height)
    {
        QRect frameGeom = frameInfo[frame].geom;

        // Take the passed paint rectangle in gif screen coordinates, and
        // clip it to the frame's geometry
        QRect screenPaintRect = QRect(sx, sy, width, height) & frameGeom;

        // Same thing but in the frame's coordinate system
        QRect framePaintRect  = screenPaintRect.translated(-frameGeom.topLeft());

        curFrame->paint(dx + screenPaintRect.x() - sx, dy + screenPaintRect.y() - sy, p,
                framePaintRect.x(), framePaintRect.y(),
                framePaintRect.width(), framePaintRect.height());
    }

    // Renders current gif screen state on the painter
    void renderCurScreen(int dx, int dy, QPainter* p, int sx, int sy, int width, int height)
    {
        // Depending on the previous frame's mode, we may have to cut out a hole when
        // painting the canvas, since if previous frame had BG disposal, we have to do OpSrc.
        if (previousWasBG)
        {
            QRegion canvasDrawRegion(sx, sy, width, height);
            canvasDrawRegion -= frameInfo[frame].geom;
            QVector<QRect> srcRects = canvasDrawRegion.rects();
    
            foreach (const QRect& r, srcRects)
                p->drawPixmap(QPoint(dx + r.x() - sx, dy + r.y() - sy), canvas, r);
        }
        else
        {
            p->drawPixmap(dx, dy, canvas, sx, sy, width, height);
        }

        // Now render the current frame's overlay
        renderCurImage(dx, dy, p, sx, sy, width, height);
    }

    // Update screen, incorporating the dispose operator for current image
    void updateScreenAfterDispose()
    {
        previousWasBG = false;

        // If we're the last frame, just clear the canvas...
        if (frame == frameInfo.size() - 1)
        {
            canvas.fill(bgColor);
            return;
        }

        switch (frameInfo[frame].mode)
        {
            case GCE_DisposalRestore:
            case GCE_UndocumentedMode4: // Not in the spec, mozilla inteprets as above
                // "restore" means the state of the canvas should be the 
                // same as before the current frame.. But we don't touch it 
                // when painting, so it's a no-op.
                return;

            case GCE_DisposalLeave:
            case GCE_DisposalUnspecified: // Qt3 appears to interpret this as leave
            {
                // Update the canvas with current image.
                QPainter p(&canvas);
                if (previousWasBG)
                    p.setCompositionMode(QPainter::CompositionMode_Source);
                renderCurImage(0, 0, &p, 0, 0, canvas.width(), canvas.height());
                return;
            }

            case GCE_DisposalBG:
            {
                previousWasBG = true;
                // Clear with bg color --- the frame rect only
                QPainter p(&canvas);
                p.setCompositionMode(QPainter::CompositionMode_Source);
                p.fillRect(frameInfo[frame].geom, bgColor);
                return;
            }

            default:
                // Including GCE_DisposalUnspecified -- ???
                break;
        }
    }

    virtual void paint(int dx, int dy, QPainter* p, int sx, int sy, int width, int height)
    {
        if (!width || !height)
            return; //Nothing to draw.

        // Move over to next frame if need be, incorporating 
        // the change effect of current one onto the screen.
        if (shouldSwitchFrame)
        {
            updateScreenAfterDispose();

            ++frame;
            if (frame >= frameInfo.size())
            {
                if (animationAdvice == KHTMLSettings::KAnimationLoopOnce)
                    animationAdvice = KHTMLSettings::KAnimationDisabled;
                frame = 0;
            }
            nextFrame();
        }

        // Request next frame to be drawn...
        if (shouldSwitchFrame || firstTime) {
            shouldSwitchFrame = false;
            firstTime         = false;

            // ### FIXME: adjust based on actual interframe timing -- the jitter is 
            // likely to be quite big 
            ImageManager::animTimer()->nextFrameIn(this, frameInfo[frame].delay);
        }

        // Render the currently active frame
        renderCurScreen(dx, dy, p, sx, sy, width, height);

#ifdef DEBUG_GIFLOADER
        p->drawText(QPoint(dx - sx, dy - sy + p->fontMetrics().height()), QString::number(frame));
#endif
    }

    virtual AnimProvider* clone(PixmapPlane *plane)
    {
        if (frame0->height == 0 || frame0->width == 0 ||
            plane->height == 0 || plane->width == 0)
            return 0;

        float heightRatio = frame0->height / plane->height;
        float widthRatio = frame0->width / plane->width;

        QVector<GIFFrameInfo> newFrameInfo;
        Q_FOREACH(const GIFFrameInfo &oldFrame, frameInfo) {
            GIFFrameInfo newFrame(oldFrame);

            newFrame.geom.setWidth(oldFrame.geom.width() * widthRatio);
            newFrame.geom.setHeight(oldFrame.geom.height() * heightRatio);
            newFrame.geom.setX(oldFrame.geom.x() * widthRatio);
            newFrame.geom.setY(oldFrame.geom.y() * heightRatio);
            newFrameInfo.append(newFrame);
        }

        return new GIFAnimProvider(plane, image, newFrameInfo, bgColor);
    }
};


class GIFLoader: public ImageLoader
{
    QByteArray buffer;
    int        bufferReadPos;
public:
    GIFLoader()
    {
        bufferReadPos = 0;
    }

    ~GIFLoader()
    {
    }

    virtual int processData(uchar* data, int length)
    {
        //Collect data in the buffer
        int pos = buffer.size();
        buffer.resize(buffer.size() + length);
        memcpy(buffer.data() + pos, data, length);
        return length;
    }

    static int gifReaderBridge(GifFileType* gifInfo, GifByteType* data, int limit)
    {
        GIFLoader* me = static_cast<GIFLoader*>(gifInfo->UserData);
        
        int remBytes = me->buffer.size() - me->bufferReadPos;
        int toRet    = qMin(remBytes, limit);
        
        memcpy(data, me->buffer.data() + me->bufferReadPos, toRet);
        me->bufferReadPos += toRet;
        
        return toRet;
    }
    
    
#if GIFLIB_MAJOR >= 5
    static unsigned int decode16Bit(unsigned char* signedLoc)
#else
    static unsigned int decode16Bit(char* signedLoc)
#endif
    {
        unsigned char* loc = reinterpret_cast<unsigned char*>(signedLoc);
    
        //GIFs are little-endian
        return loc[0] | (((unsigned int)loc[1]) << 8);
    }
    
    static void palettedToRGB(uchar* out, uchar* in, ImageFormat& format, int w)
    {
        int outPos = 0;
        for (int x = 0; x < w; ++x)
        {
            int colorCode = in[x];
            QRgb color = 0;
            if (colorCode < format.palette.size())
                color = format.palette[colorCode];

            *reinterpret_cast<QRgb*>(&out[outPos]) = color;
            outPos += 4;
        }
    }

    // Read a color from giflib palette, with range checking
    static QColor colorMapColor(ColorMapObject* map, int index)
    {
        QColor col(Qt::black);
        if (!map)
            return col;

        if (index < map->ColorCount)
            col = QColor(map->Colors[index].Red,
                         map->Colors[index].Green,
                         map->Colors[index].Blue);
        return col;
    }

    static void printColorMap(ColorMapObject* map)
    {
        for (int c = 0; c < map->ColorCount; ++c)
            qDebug() << "  " << map << c << map->Colors[c].Red
                          << map->Colors[c].Green
                          << map->Colors[c].Blue;
    }
    
    virtual int processEOF()
    {
        //Feed the buffered data to libUnGif
#if GIFLIB_MAJOR >= 5
        int errorCode;
        GifFileType* file = DGifOpen(this, gifReaderBridge, &errorCode);
#else
        GifFileType* file = DGifOpen(this, gifReaderBridge);
#endif
        
        if (!file)
            return Error;
        
        if (DGifSlurp(file) == GIF_ERROR)
        {
            DGifCloseFile(file);
            return Error;
        }
        

        //We use canvas size only for animations
        if (file->ImageCount > 1) {
            // Verify it..
            if (!ImageManager::isAcceptableSize(file->SWidth, file->SHeight)) {
                DGifCloseFile(file);
                return Error;
            }
            notifyImageInfo(file->SWidth, file->SHeight);
        }
        
        // Check each frame to be within the size limit policy
        for (int frame = 0; frame < file->ImageCount; ++frame)
        {
            //Extract colormap, geometry, so that we can create the frame
            SavedImage* curFrame = &file->SavedImages[frame];
            if (!ImageManager::isAcceptableSize(curFrame->ImageDesc.Width, curFrame->ImageDesc.Height)) {
                DGifCloseFile(file);
                return Error;
            }
        }
        
        QVector<GIFFrameInfo> frameProps;
        
        // First, use the screen color map...
        ColorMapObject* globalColorMap = file->SColorMap;

        // If for some reason there is none, pick one from an image, 
        // and pray it works.
        if (!globalColorMap)
            globalColorMap = file->Image.ColorMap;

        QColor bgColor = colorMapColor(globalColorMap, file->SBackGroundColor);

        //Extract out all the frames
        for (int frame = 0; frame < file->ImageCount; ++frame)
        {
            //Extract colormap, geometry, so that we can create the frame
            SavedImage* curFrame = &file->SavedImages[frame];
            int w = curFrame->ImageDesc.Width;
            int h = curFrame->ImageDesc.Height;

            //For non-animated images, use the frame size for dimension
            if (frame == 0 && file->ImageCount == 1)
                notifyImageInfo(w, h);

            ColorMapObject* colorMap = curFrame->ImageDesc.ColorMap;
            if (!colorMap)  colorMap = globalColorMap;

            GIFFrameInfo frameInf;
            int          trans = -1;
            frameInf.delay = 100;
            frameInf.mode  = GCE_DisposalUnspecified;

            //Go through the extension blocks to see whether there is a color key,
            //and animation info inside the graphics control extension (GCE) block
            for (int ext = 0; ext < curFrame->ExtensionBlockCount; ++ext)
            {
                ExtensionBlock* curExt = &curFrame->ExtensionBlocks[ext];
                if ((curExt->Function  == GCE_Code) && (curExt->ByteCount >= GCE_Size))
                {
                    if (curExt->Bytes[GCE_Flags] & GCE_TransColorMask)
                        trans = ((unsigned char)curExt->Bytes[GCE_TransColor]);

                    frameInf.mode  = curExt->Bytes[GCE_Flags] & GCE_DisposalMask;
                    frameInf.delay = decode16Bit(&curExt->Bytes[GCE_Delay]) * 10;

                    if (frameInf.delay < 100)
                        frameInf.delay = 100;
                }
            }

#ifdef DEBUG_GIFLOADER
            frameInf.delay = 1500;
#endif

            // The only thing I found resembling an explanation suggests that we should 
            // set bgColor to transparent if the first frame's GCE is such...
            // Let's hope this is what actually happens.. (Man, I wish testcasing GIFs was manageable)
            if (frame == 0 && trans != -1)
                bgColor = QColor(Qt::transparent);


            // If we have transparency, we need to go an RGBA mode.
            ImageFormat format;
            if (trans != -1)
                format.type = ImageFormat::Image_ARGB_32; // Premultiply always OK, since we only have 0/1 alpha
            else
                format.type = ImageFormat::Image_Palette_8;

            // Read in colors for the palette... Don't waste memory on 
            // any extra ones beyond 256, though
            int colorCount = colorMap ? colorMap->ColorCount : 0;
            for (int c = 0; c < colorCount && c < 256; ++c) {
                format.palette.append(qRgba(colorMap->Colors[c].Red,
                                            colorMap->Colors[c].Green,
                                            colorMap->Colors[c].Blue, 255));
            }

            // Pad with black as a precaution
            for (int c = colorCount; c < 256; ++c)
                format.palette.append(qRgba(0, 0, 0, 255));

            //Put in the colorkey color 
            if (trans != -1)
                format.palette[trans] = qRgba(0, 0, 0, 0);

            //Now we can declare frame format
            notifyAppendFrame(w, h, format);
            
            frameInf.bg   = bgColor;
            frameInf.geom = QRect(curFrame->ImageDesc.Left,
                                  curFrame->ImageDesc.Top,
                                  w, h);

            frameInf.trans  = format.hasAlpha();
            frameProps.append(frameInf); 

#ifdef DEBUG_GIFLOADER
            qDebug("frame:%d:%d,%d:%dx%d, trans:%d, mode:%d", frame, frameInf.geom.x(), frameInf.geom.y(), w, h, trans, frameInf.mode);
#endif

            //Decode the scanlines
            uchar* buf;
            if (format.hasAlpha())
                buf = new uchar[w*4];
            else
                buf = new uchar[w];
                
            if (curFrame->ImageDesc.Interlace)
            {
                // Interlaced. Considering we don't do progressive loading of gif's, 
                // a useless annoyance... The way it works is that on the first pass 
                // it renders scanlines 8*n, on second 8*n + 4, 
                // third then 4*n + 2, and finally 2*n + 1 (the odd lines)
                // e.g.:
                // 0, 8,  16, ...
                // 4, 12, 20, ...
                // 2, 6, 10, 14, ...
                // 1, 3, 5, 7, ...
                //
                // Anyway, the bottom line is that INTERLACED_OFFSET contains 
                // the initial position, and INTERLACED_JUMP has the increment. 
                // However, imload expects a top-bottom scan of the image... 
                // so what we do it is keep track of which lines are actually 
                // new via nextNewLine variable, and leave others unchanged.

                int interlacedImageScanline = 0; // scanline in interlaced image we are reading from
                for (int pass = 0; pass < 4; ++pass)
                {
                    int nextNewLine = INTERLACED_OFFSET[pass];
                    
                    for (int line = 0; line < h; ++line)
                    {
                        if (line == nextNewLine)
                        {
                            uchar* toFeed = (uchar*) curFrame->RasterBits + w*interlacedImageScanline;
                            if (format.hasAlpha())
                            {
                                palettedToRGB(buf, toFeed, format, w);
                                toFeed = buf;
                            }
                            
                            notifyScanline(pass + 1, toFeed);
                            ++interlacedImageScanline;
                            nextNewLine += INTERLACED_JUMP[pass];
                        }
                        else
                        {
                            // No new information for this scanline, so just 
                            // get it from loader, and feed it right back in 
                            requestScanline(line, buf);
                            notifyScanline (pass + 1, buf);
                        }
                    } // for every scanline
                } // for pass..
            } // if interlaced
            else
            {
                for (int line = 0; line < h; ++line)
                {
                    uchar* toFeed = (uchar*) file->SavedImages[frame].RasterBits + w*line;
                    if (format.hasAlpha())
                    {
                        palettedToRGB(buf, toFeed, format, w);
                        toFeed = buf;
                    }
                    notifyScanline(1, toFeed);
                }
            }
            delete[] buf;
        }


        if (file->ImageCount > 1)
        {
            //need animation provider
            PixmapPlane* frame0  = requestFrame0();
            frame0->animProvider = new GIFAnimProvider(frame0, image, frameProps, bgColor);
        }
        
        DGifCloseFile(file);

        return Done;
    }
};


ImageLoaderProvider::Type GIFLoaderProvider::type()
{
    return Efficient;
}

ImageLoader* GIFLoaderProvider::loaderFor(const QByteArray& prefix)
{
    uchar* data = (uchar*)prefix.data();
    if (prefix.size() < 6) return 0;

    if (data[0] == 'G'  &&
        data[1] == 'I'  &&
        data[2] == 'F'  &&
        data[3] == '8'  &&
      ((data[4] == '7') || (data[4] == '9')) && 
        data[5] == 'a')
         return new GIFLoader;

    return 0;
}

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
