/*
    Large image load library -- PNG decoder

    Copyright (C) 2004 Maksim Orlovich <maksim@kde.org>

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

#include "pngloader.h"
#include "imageloader.h"
#include "imagemanager.h"

#include <png.h>

namespace khtmlImLoad {


class PNGLoader;
/**
Paranoia: since g++ may very well store "this" in a register, we
use and external volatile to refer to our instance in case libPNG longjumps 
on us. We also use this variable to find where to map libPNG's callbacks.
*/
static volatile PNGLoader* curLoader; 

class PNGLoader: public ImageLoader
{
private:
    png_structp pngReadStruct;
    png_infop   pngInfoStruct;
    
    bool        libPngError;
    bool        done;
    
    bool        interlaced;
    
    png_uint_32  width;
    png_uint_32  height;
    unsigned int depth;
    
    unsigned char* scanlineBuf; //Used only for interlaced images
    
    /**
     Dispatch hooks for libPNG callbacks -- call instance methods, 
     and make parameters slightly more our-style
    */
    static void dispHaveInfo(png_structp, png_infop)
    {    
        const_cast<PNGLoader*>(curLoader)->haveInfo(); 
    }
    
    static void dispHaveRow(png_structp, png_bytep row, png_uint_32 rowNum, int pass)
    {
        const_cast<PNGLoader*>(curLoader)->haveRow(rowNum, pass, row);
    }
    
    static void dispHaveEnd(png_structp, png_infop)
    {
        const_cast<PNGLoader*>(curLoader)->haveEnd();
    }
    
    /**
     Our implementation of libPNG callbacks
    */
    void haveInfo()
    {
        int bitDepth, colorType, interlaceType;
    
        png_get_IHDR(pngReadStruct, pngInfoStruct, &width, &height, &bitDepth,
                     &colorType, &interlaceType, 0, 0);
                     
        if (!ImageManager::isAcceptableSize(width, height)) {
            libPngError = true;
            return;
        }
        
        //Ask libPNG to change bit depths we don't support
        if (bitDepth < 8)
#if PNG_LIBPNG_VER < 10400
            png_set_gray_1_2_4_to_8(pngReadStruct);
#else
            png_set_expand_gray_1_2_4_to_8(pngReadStruct);
#endif
        
        if (bitDepth > 8)
            png_set_strip_16       (pngReadStruct);
            
        //Some images (basically, only paletted ones) may have alpha
        //included as part of a tRNS chunk. We want to convert that to regular alpha
        //channel..
        bool haveTRNS = false; 
        if (png_get_valid(pngReadStruct, pngInfoStruct, PNG_INFO_tRNS))
        {
            png_set_tRNS_to_alpha(pngReadStruct);
            haveTRNS = true;
            
            if (colorType == PNG_COLOR_TYPE_RGB)
                colorType =  PNG_COLOR_TYPE_RGB_ALPHA; //Paranoia..
            else if (colorType == PNG_COLOR_TYPE_GRAY)
                colorType = PNG_COLOR_TYPE_GRAY_ALPHA;
        }    
            
        ImageFormat imFrm;    
            
        //Prepare for mapping from colorType to our format descriptors.
        switch (colorType)
        {
            case PNG_COLOR_TYPE_GRAY:
                imFrm.greyscaleSetup();
                break;
            case PNG_COLOR_TYPE_GRAY_ALPHA:
                //We don't natively support 8-bit plus alpha, so ask libPNG to expand it out to RGB
                png_set_gray_to_rgb(pngReadStruct);
                imFrm.type = ImageFormat::Image_ARGB_32;
                break;
            case PNG_COLOR_TYPE_PALETTE:                
                //For now, we handle paletted images as RGB or ARGB
                //### TODO: handle non-alpha paletted images with a sufficiently small palette as 
                //paletted
                imFrm.type = haveTRNS ? ImageFormat::Image_ARGB_32 : ImageFormat::Image_RGB_32;
                png_set_palette_to_rgb(pngReadStruct);
                break;
            case PNG_COLOR_TYPE_RGB:
                imFrm.type = ImageFormat::Image_RGB_32;
                break;
            case PNG_COLOR_TYPE_RGB_ALPHA:
                imFrm.type = ImageFormat::Image_ARGB_32;
                break;
            default:
                //Huh?
                libPngError = true;
                return;
        }
        
        //Configure padding/byte swapping if need be (32-bit images)
        //We want a 32-bit value with ARGB.
        //This means that for little-endian, in memory we should have BGRA,
        //and for big-endian, well, ARGB        
        if (imFrm.type == ImageFormat::Image_RGB_32)
        {
            //Need fillers, plus perhaps BGR swapping for non-alpha
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
            png_set_filler(pngReadStruct, 0xff, PNG_FILLER_BEFORE);
#else
            png_set_filler(pngReadStruct, 0xff, PNG_FILLER_AFTER);
            png_set_bgr   (pngReadStruct);
#endif                
        }
        else if (imFrm.type == ImageFormat::Image_ARGB_32)
        {
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
            png_set_swap_alpha(pngReadStruct); //ARGB, not RGBA
#else
            png_set_bgr   (pngReadStruct);     //BGRA
#endif
        }
        
        //Remember depth, for our own use
        depth = imFrm.depth();
        
        //handle interlacing        
        if (interlaceType != PNG_INTERLACE_NONE)
        {
            interlaced  = true;
            scanlineBuf = new unsigned char[depth * width];
            png_set_interlace_handling(pngReadStruct);
            
            // Give up on premultiply in this case..
            if (imFrm.type == ImageFormat::Image_ARGB_32)
                imFrm.type = ImageFormat::Image_ARGB_32_DontPremult;
        }
        
        notifySingleFrameImage(width, height, imFrm);
        
        //OK, time to start input
        png_read_update_info(pngReadStruct, pngInfoStruct);
    }
    
    void haveRow(unsigned int rowNum, int pass, unsigned char* data)
    {
        if (interlaced)
        {
            Q_ASSERT(png_get_bit_depth(pngReadStruct, pngInfoStruct) <= depth * 8);
            requestScanline(rowNum, scanlineBuf);
            png_progressive_combine_row(pngReadStruct, scanlineBuf, data);
            notifyScanline(pass + 1, scanlineBuf);
        }
        else
            notifyScanline(pass + 1, data);
    }
    
    void haveEnd()
    {
        done = true;
    }
    
public:
    PNGLoader()
    {
        done        = false;
        libPngError = false;
        
        interlaced  = false;
        scanlineBuf = 0;
    
        //Setup the basic structures.
        //### I think I want to pass the handlers to shut it up?
        pngReadStruct = png_create_read_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
                       
        pngInfoStruct = png_create_info_struct(pngReadStruct);
        
        //Prepare for progressive loading
        png_set_progressive_read_fn(pngReadStruct, 0, dispHaveInfo, dispHaveRow, dispHaveEnd);
    }
    
    ~PNGLoader()
    {
        delete[] scanlineBuf;
    
        png_destroy_read_struct(&pngReadStruct, &pngInfoStruct, 0);
        //### CHECKME!
    }
    
    virtual int processData(uchar* data, int length)
    {
        if (done)
            return Done;
    
        //This bit takes care of dealing with the libPNG error handling
        if (libPngError)
            return Error;
            
        curLoader = this;                
        if (setjmp(png_jmpbuf(pngReadStruct)))
        {
            curLoader->libPngError = true;
            return Error;
        }

        //OK, now we can actually do work.... Push data to libPNG,
        //it will use callbacks
        //### time limiting?
        png_process_data(pngReadStruct, pngInfoStruct, data, length);
        
        return length;
    }
};


ImageLoaderProvider::Type PNGLoaderProvider::type()
{    
    return Efficient;
}

    
ImageLoader* PNGLoaderProvider::loaderFor(const QByteArray& prefix)
{
    uchar* data = (uchar*)prefix.data();
    if (prefix.size() < 8) return 0;

    if(data[0] == 0x89 &&
       data[1] == 'P'  &&
       data[2] == 'N'  &&
       data[3] == 'G'  &&
       data[4] == 0x0D && 
       data[5] == 0x0A &&
       data[6] == 0x1A && 
       data[7] == 0x0A)
         return new PNGLoader;

    return 0;
}

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
