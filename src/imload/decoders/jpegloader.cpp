/*
    This file is part of the KDE libraries

    Copyright (C) 2004 Maks Orlovich (maksim@kde.org)
    Copyright (C) 2000 Dirk Mueller (mueller@kde.org)

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
#include "jpegloader.h"

//### FIXME: I removed all the fancy configury stuff. it needs to be put back

#include <stdio.h>
#include <setjmp.h>
#include <QtCore/QDate>
#include <QImage>

#include "imageloader.h"
#include "imagemanager.h"

extern "C" {
#define XMD_H
#include <jpeglib.h>
#undef const
}

#undef BUFFER_DEBUG
//#define BUFFER_DEBUG

#undef JPEG_DEBUG
//#define JPEG_DEBUG

namespace khtmlImLoad {


class JPEGLoader: public ImageLoader
{
    struct Private;
    friend struct Private;
    Private* d;    
public:
    JPEGLoader();
    ~JPEGLoader();    
    virtual int processData(uchar* data, int length);
};

ImageLoaderProvider::Type JPEGLoaderProvider::type()
{
    return Efficient;
}

ImageLoader* JPEGLoaderProvider::loaderFor(const QByteArray& prefix)
{
    uchar* data = (uchar*)prefix.data();
    if (prefix.size() < 3) return 0;

    if(data[0] == 0377 &&
       data[1] == 0330 &&
       data[2] == 0377)
         return new JPEGLoader;

    return 0;
}

// -----------------------------------------------------------------------------

struct khtml_error_mgr : public jpeg_error_mgr {
    jmp_buf setjmp_buffer;
};

extern "C" {

    static
    void khtml_error_exit (j_common_ptr cinfo)
    {
        khtml_error_mgr* myerr = (khtml_error_mgr*) cinfo->err;
        char buffer[JMSG_LENGTH_MAX];
        (*cinfo->err->format_message)(cinfo, buffer);
        qWarning("%s", buffer);
        longjmp(myerr->setjmp_buffer, 1);
    }
}

static const int max_buf = 8192;
static const int max_consumingtime = 500;

struct khtml_jpeg_source_mgr : public jpeg_source_mgr {
    JOCTET buffer[max_buf];

    int valid_buffer_len;
    size_t skip_input_bytes;
    int ateof;
    QTime decoder_timestamp;
    bool final_pass;
    bool decoding_done;
    bool do_progressive;
public:
    khtml_jpeg_source_mgr();
};


extern "C" {

    static
    void khtml_j_decompress_dummy(j_decompress_ptr)
    {
    }

    static
    boolean khtml_fill_input_buffer(j_decompress_ptr cinfo)
    {
#ifdef BUFFER_DEBUG
        qDebug("khtml_fill_input_buffer called!");
#endif

        khtml_jpeg_source_mgr* src = (khtml_jpeg_source_mgr*)cinfo->src;

        if ( src->ateof )
        {
            /* Insert a fake EOI marker - as per jpeglib recommendation */
            src->buffer[0] = (JOCTET) 0xFF;
            src->buffer[1] = (JOCTET) JPEG_EOI;
            src->bytes_in_buffer = 2;
            src->next_input_byte = (JOCTET *) src->buffer;
#ifdef BUFFER_DEBUG
            qDebug("...returning true!");
#endif
            return TRUE;
        }
        else
            return FALSE;  /* I/O suspension mode */
    }

    static
    void khtml_skip_input_data(j_decompress_ptr cinfo, long num_bytes)
    {
        if(num_bytes <= 0)
            return; /* required noop */

#ifdef BUFFER_DEBUG
        qDebug("khtml_skip_input_data (%d) called!", num_bytes);
#endif

        khtml_jpeg_source_mgr* src = (khtml_jpeg_source_mgr*)cinfo->src;
        src->skip_input_bytes += num_bytes;

        unsigned int skipbytes = qMin(src->bytes_in_buffer, src->skip_input_bytes);

#ifdef BUFFER_DEBUG
        qDebug("skip_input_bytes is now %d", src->skip_input_bytes);
        qDebug("skipbytes is now %d", skipbytes);
        qDebug("valid_buffer_len is before %d", src->valid_buffer_len);
        qDebug("bytes_in_buffer is %d", src->bytes_in_buffer);
#endif

        if(skipbytes < src->bytes_in_buffer)
            memmove(src->buffer, src->next_input_byte+skipbytes, src->bytes_in_buffer - skipbytes);

        src->bytes_in_buffer -= skipbytes;
        src->valid_buffer_len = src->bytes_in_buffer;
        src->skip_input_bytes -= skipbytes;

        /* adjust data for jpeglib */
        cinfo->src->next_input_byte = (JOCTET *) src->buffer;
        cinfo->src->bytes_in_buffer = (size_t) src->valid_buffer_len;
#ifdef BUFFER_DEBUG
        qDebug("valid_buffer_len is afterwards %d", src->valid_buffer_len);
        qDebug("skip_input_bytes is now %d", src->skip_input_bytes);
#endif
    }
}

khtml_jpeg_source_mgr::khtml_jpeg_source_mgr()
{
    jpeg_source_mgr::init_source = khtml_j_decompress_dummy;
    jpeg_source_mgr::fill_input_buffer = khtml_fill_input_buffer;
    jpeg_source_mgr::skip_input_data = khtml_skip_input_data;
    jpeg_source_mgr::resync_to_restart = jpeg_resync_to_restart;
    jpeg_source_mgr::term_source = khtml_j_decompress_dummy;
    bytes_in_buffer = 0;
    valid_buffer_len = 0;
    skip_input_bytes = 0;
    ateof = 0;
    next_input_byte = buffer;
    final_pass = false;
    decoding_done = false;
}

struct JPEGLoader::Private
{
    int processData(uchar* data, int length);
    Private();
    ~Private();
    
    JPEGLoader* owner;
private:
    int    passNum;
    uchar* scanline;

    enum {
        Init,
        readHeader,
        startDecompress,
        decompressStarted,
        consumeInput,
        prepareOutputScan,
        doOutputScan,
        readDone,
        invalid
    } state;

    // structs for the jpeglib
    struct jpeg_decompress_struct cinfo;
    struct khtml_error_mgr jerr;
    struct khtml_jpeg_source_mgr jsrc;    
};

JPEGLoader::Private::Private()
{
    scanline = 0;
    passNum  = 0;
    
    memset(&cinfo, 0, sizeof(cinfo));
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&cinfo);
    cinfo.err = jpeg_std_error(&jerr);
    jerr.error_exit = khtml_error_exit;
    cinfo.src = &jsrc;
    state = Init;
}

JPEGLoader::Private::~Private()
{
    delete[] scanline;
    (void) jpeg_destroy_decompress(&cinfo);
}

int JPEGLoader::Private::processData(uchar* buffer, int length)
{
    if (jsrc.ateof) 
    {
#ifdef JPEG_DEBUG
        qDebug("ateof, eating");
#endif    
        return ImageLoader::Done;
    }       
    
    if(setjmp(jerr.setjmp_buffer))
    {
#ifdef JPEG_DEBUG
        qDebug("jump into state invalid");
#endif

        // this is fatal
        return ImageLoader::Error;
    }

    int consumed = qMin(length, max_buf - jsrc.valid_buffer_len);

#ifdef BUFFER_DEBUG
    qDebug("consuming %d bytes", consumed);
#endif

    // filling buffer with the new data
    memcpy(jsrc.buffer + jsrc.valid_buffer_len, buffer, consumed);
    jsrc.valid_buffer_len += consumed;

    if(jsrc.skip_input_bytes)
    {
#ifdef BUFFER_DEBUG
        qDebug("doing skipping");
        qDebug("valid_buffer_len %d", jsrc.valid_buffer_len);
        qDebug("skip_input_bytes %d", jsrc.skip_input_bytes);
#endif
        int skipbytes = qMin((size_t) jsrc.valid_buffer_len, jsrc.skip_input_bytes);

        if(skipbytes < jsrc.valid_buffer_len)
            memmove(jsrc.buffer, jsrc.buffer+skipbytes, jsrc.valid_buffer_len - skipbytes);

        jsrc.valid_buffer_len -= skipbytes;
        jsrc.skip_input_bytes -= skipbytes;

        // still more bytes to skip
        if(jsrc.skip_input_bytes) {
            if(consumed <= 0) qDebug("ERROR!!!");
            return consumed;
        }
    }        
    
    cinfo.src->next_input_byte = (JOCTET *) jsrc.buffer;
    cinfo.src->bytes_in_buffer = (size_t) jsrc.valid_buffer_len;

#ifdef BUFFER_DEBUG
    qDebug("buffer contains %d bytes", jsrc.valid_buffer_len);
#endif

    if(state == Init)
    {
        if(jpeg_read_header(&cinfo, TRUE) != JPEG_SUSPENDED) {
            state = startDecompress;
            
            // libJPEG can scale down 2x, 4x, and 8x, 
            // so do this for oversize images.
            int scaleDown = 1;
            while (scaleDown <= 8 && !ImageManager::isAcceptableSize(
                    cinfo.image_width / scaleDown, cinfo.image_height / scaleDown)) {
                scaleDown *= 2;
            }
            
            cinfo.scale_denom *= scaleDown;
            
            if (scaleDown > 8) {
                // Still didn't fit... Abort.
                return ImageLoader::Error;
            }
        }
    }
    
    if(state == startDecompress)
    {
        jsrc.do_progressive = jpeg_has_multiple_scans( &cinfo );
        if ( jsrc.do_progressive )
            cinfo.buffered_image = TRUE;
        else
            cinfo.buffered_image = FALSE;
        // setup image sizes
        jpeg_calc_output_dimensions( &cinfo );
        
        if ( cinfo.jpeg_color_space == JCS_YCbCr )
            cinfo.out_color_space = JCS_RGB;
        
        if ( cinfo.jpeg_color_space == JCS_YCCK )
            cinfo.out_color_space = JCS_CMYK;

        cinfo.do_fancy_upsampling = TRUE;
        cinfo.do_block_smoothing = FALSE;
        cinfo.quantize_colors = FALSE;

        // false: IO suspension
        if(jpeg_start_decompress(&cinfo)) {
            ImageFormat f;
            if ( cinfo.output_components == 3 || cinfo.output_components == 4) {
                f.type = ImageFormat::Image_RGB_32;
                scanline = new uchar[cinfo.output_width*4];
                
            } else if ( cinfo.output_components == 1 ) {            
                f.greyscaleSetup();
                scanline = new uchar[cinfo.output_width];
            }
            // ### else return Error?
                       
            owner->notifySingleFrameImage(cinfo.output_width, cinfo.output_height, f);
            
#ifdef JPEG_DEBUG
            qDebug("will create a picture %d/%d in size", cinfo.output_width, cinfo.output_height);
#endif

#ifdef JPEG_DEBUG
            qDebug("ok, going to decompressStarted");
#endif

            jsrc.decoder_timestamp.start();
            state = jsrc.do_progressive ? decompressStarted : doOutputScan;
        }
    }
    
    if(state == decompressStarted) {
        state = (!jsrc.final_pass && jsrc.decoder_timestamp.elapsed() < max_consumingtime)
                ? consumeInput : prepareOutputScan;
    }

    if(state == consumeInput)
    {
        int retval;

        do {
            retval = jpeg_consume_input(&cinfo);
        } while (retval != JPEG_SUSPENDED && retval != JPEG_REACHED_EOI);

        if(jsrc.decoder_timestamp.elapsed() > max_consumingtime || jsrc.final_pass ||
           retval == JPEG_REACHED_EOI || retval == JPEG_REACHED_SOS)
            state = prepareOutputScan;
    }

    if(state == prepareOutputScan)
    {
        jsrc.decoder_timestamp.restart();
        if ( jpeg_start_output(&cinfo, cinfo.input_scan_number) )
            state = doOutputScan;
    }

    if(state == doOutputScan)
    {
        if(!scanline || jsrc.decoding_done)
        {
#ifdef JPEG_DEBUG
            qDebug("complete in doOutputscan, eating..");
#endif
            return consumed;
        }
        uchar* lines[1] = {scanline};
        //int oldoutput_scanline = cinfo.output_scanline;

        //Decode and feed line-by-line
        while(cinfo.output_scanline < cinfo.output_height)
        {
            if (!jpeg_read_scanlines(&cinfo, lines, 1))
                break;
                
            if ( cinfo.output_components == 3 )
            {
                // Expand 24->32 bpp.
                uchar *in = scanline + cinfo.output_width * 3;
                QRgb *out = (QRgb*)scanline;

                for (uint i = cinfo.output_width; i--; )
                {
                    in-=3;
                    out[i] = qRgb(in[0], in[1], in[2]);
                }
            } else if ( cinfo.out_color_space == JCS_CMYK ) {
                uchar *in = scanline + cinfo.output_width * 4;
                QRgb *out = (QRgb *) scanline;

                for (uint i = cinfo.output_width; i--; )
                {
                    in -= 4;
                    int k = in[3];
                    out[i] = qRgb(k * in[0] / 255, k * in[1] / 255, k * in[2] / 255);
                }
            }
            
            owner->notifyScanline(passNum + 1, scanline);
        } //per-line scan    
        
        if(cinfo.output_scanline >= cinfo.output_height)
        {
            passNum++;
            
            if ( jsrc.do_progressive ) {
                jpeg_finish_output(&cinfo);
                jsrc.final_pass = jpeg_input_complete(&cinfo);
                jsrc.decoding_done = jsrc.final_pass && cinfo.input_scan_number == cinfo.output_scan_number;
            }
            else
                jsrc.decoding_done = true;
            
            if (passNum > ImageLoader::FinalVersionID)
            {
                qWarning("JPEG Decoder: Too many interlacing passes needed");
                jsrc.decoding_done = true; //Force exit
            }

#ifdef JPEG_DEBUG
            qDebug("one pass is completed, final_pass = %d, dec_done: %d, complete: %d",
                   jsrc.final_pass, jsrc.decoding_done, jpeg_input_complete(&cinfo));
#endif
            if(!jsrc.decoding_done)
            {
#ifdef JPEG_DEBUG
                qDebug("starting another one, input_scan_number is %d/%d", cinfo.input_scan_number,
                       cinfo.output_scan_number);
#endif
                jsrc.decoder_timestamp.restart();
                state = decompressStarted;
            }
        }
    }
    
    if(state == doOutputScan && jsrc.decoding_done)
    {
#ifdef JPEG_DEBUG
        qDebug("input is complete, cleaning up, returning..");
#endif

        jsrc.ateof = true;
        
        (void) jpeg_finish_decompress(&cinfo);
        (void) jpeg_destroy_decompress(&cinfo);
    
        state = readDone;    
    
        return Done;
    }
 
#ifdef BUFFER_DEBUG
    qDebug("valid_buffer_len is now %d", jsrc.valid_buffer_len);
    qDebug("bytes_in_buffer is now %d", jsrc.bytes_in_buffer);
    qDebug("consumed %d bytes", consumed);
#endif
    if(jsrc.bytes_in_buffer && jsrc.buffer != jsrc.next_input_byte)
        memmove(jsrc.buffer, jsrc.next_input_byte, jsrc.bytes_in_buffer);
    jsrc.valid_buffer_len = jsrc.bytes_in_buffer;
    return consumed;    
}

JPEGLoader::JPEGLoader()
{
    d = new Private;
    d->owner = this;
}

JPEGLoader::~JPEGLoader()
{
    delete d;
}

int JPEGLoader::processData(uchar* data, int length)
{
    return d->processData(data, length);
}

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
