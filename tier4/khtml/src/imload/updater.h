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

#ifndef UPDATER_H
#define UPDATER_H

#include <QObject>
#include <QVector>

class QTimer;

namespace khtmlImLoad {

class Image;

/**
 The updater class helps manage timers, to permit update messages to be coalesced (so we don't 
 bug KHTML, or whatever use every 5 pico-seconds.
*/
class Updater: public QObject
{
    Q_OBJECT
public:
    Updater();

    /**
     The frames should call this function as they start having data to emit. 
     The updater will call back their notifyPerformUpdate() function when a sufficient
     amount of time has passed.
     */
    void haveUpdates(Image* frame);

    /**
     Called by image when it's destroyed, and hence should be purged 
     from the updates list
    */
    void destroyed(Image* frame);

private Q_SLOTS:
    void pushUpdates();
private:
    QTimer* updatePusher;
    bool updatesPending();
    QVector<Image*> frames[10];
    int             timePortion;
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
