/*
    Progressive image displaying library.

    Copyright (C) 2004,2005 Maks Orlovich (maksim@kde.org)

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

#ifndef IMAGE_LOADER_H
#define IMAGE_LOADER_H

#include "imageformat.h"
#include "image.h"

namespace khtmlImLoad {

class Image;

/**
 A base class for decoders. The decoders should inherit off this, and use the protected functions to  
 feed their images data
*/
class ImageLoader
{
protected:
    Image* image;

    ImageLoader()
    {
        image = 0;
    }

    /**
     Call to declare canvas geometry and format. May only be called once
     */
    void notifyImageInfo(int width, int height)
    {
        image->notifyImageInfo(width, height);
    }

    /**
     Call to declare frame geometry, should be called for each frame.
    */
    void notifyAppendFrame(int fwidth, int fheight, const ImageFormat& format)
    {
        image->notifyAppendFrame(fwidth, fheight, format);
    }

    /**
     wrapper for the above method for single-frame images
    */
    void notifySingleFrameImage(int width, int height, const ImageFormat& format)
    {
        notifyImageInfo  (width, height);
        notifyAppendFrame(width, height, format);
    }

    /**
     Call to provide a scanline, for active frame, and given "version".
     The decoder must feed multiple versions of the image inside here, top-to-bottom,
     and incrementing versions. When it's done, it should either feed the last
     version with FinalVersionID, or call notifyFinished() separately.
    */
    void notifyScanline(unsigned char version, unsigned char* line)
    {
        image->notifyScanline(version, line);
    }

    void notifyQImage(unsigned char version, const QImage* qimage)
    {
        image->notifyQImage(version, qimage);
    }

    /**
     Call this to exract the current state of a scanline to the provided bufer
    */
    void requestScanline(unsigned int lineNum, unsigned char* lineBuf)
    {
        image->requestScanline(lineNum, lineBuf);
    }

    /**
     Call this to get the first frame
    */
    PixmapPlane* requestFrame0()
    {
        return image->getSize(image->size());
    }
    
public:
    //Some constants. Not consts since some compilers are broken
    enum 
    {
        DefaultFrame   = 0,
        FinalVersionID = 0xff 
    };

    void setImage(Image* _image)
    {
        image = _image;
    }
    
    virtual ~ImageLoader()
    {}
    
    enum Status
    {
        Done  = -2,
        Error = -1
    };
    
    /**
     Decodes a portion of the image, and returns the appropriate 
     status, or the number of bytes read
    */
    virtual int processData(uchar* data, int length) = 0;

    /**
     This method is called to notify the decoder that the input is done, if the decoder 
     has not already indicated some sort of completion on the stream; this is intended 
     mostly for non-incremental decoders. It should return Done if all is OK. Note that 
     the "if" above should mean that the non-incremental decoders should just return 
     the bytes read in processData
    */
    virtual int processEOF()
    {
        return Done; //### Probably should be Error if notifyImageInfo has not been called
    }
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
