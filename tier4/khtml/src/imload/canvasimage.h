/*
    KHTML image displaying library.

    Copyright (C) 2007 Maks Orlovich (maksim@kde.org)

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
#ifndef IMLOAD_CANVAS_IMAGE_H
#define IMLOAD_CANVAS_IMAGE_H

class QImage;

#include "image.h"

namespace khtmlImLoad {

/**
  A CanvasImage encapsulates a QImage that will be painted on externally,
  in order to permit scaling of it.  When drawing happens, however, the client must call the
  contentUpdated() method to avoid out-of-date data being painted. 
 */
class CanvasImage : public Image
{
public:
    CanvasImage(int width, int height);
    void contentUpdated();
    void resizeImage(int width, int height);
private:
    void setupOriginalPlane(int width, int height);
    void flushAllCaches();
    static ImageOwner* trivialOwner();
    static ImageOwner* s_trivialOwner;
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
