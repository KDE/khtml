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

#ifndef ANIM_TIMER_H
#define ANIM_TIMER_H

#include <QtCore/QDate>
#include <QtCore/QObject>
#include <QtCore/QMap>

class QTimer;

namespace khtmlImLoad {

class AnimProvider;

/**
This class is used to manage animation frame change requests, to avoid creating multiple 
QTimers for this purpose.
*/
class AnimTimer: public QObject
{
    Q_OBJECT
public:
    AnimTimer();

    /**
     This requests that a new frame notification be called at the given delay on the given AnimProvider.
     Note that only the first request is honored, until the notification is given
     */
    void nextFrameIn(AnimProvider* provider, int ms);
    
    void destroyed(AnimProvider* provider);
private Q_SLOTS:
    void tick();
private:
    QTimer* animTicks;
    QMap<AnimProvider*, int> pending;
    QTime lastTime;
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
