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

#include "animprovider.h"
#include "pixmapplane.h"
#include "imagemanager.h"
#include "image.h"

namespace khtmlImLoad {

void AnimProvider::nextFrame()
{
    curFrame = curFrame->nextFrame;
    if (!curFrame)
        curFrame = frame0;
}

void AnimProvider::switchFrame()
{
    if (animationAdvice == KHTMLSettings::KAnimationDisabled)
        return;
    shouldSwitchFrame = true; 
    image->notifyFrameChange();
}

AnimProvider::~AnimProvider()
{
    ImageManager::animTimer()->destroyed(this);
}

void AnimProvider::setShowAnimations(KHTMLSettings::KAnimationAdvice newAdvice)
{
    animationAdvice = newAdvice;
}

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
