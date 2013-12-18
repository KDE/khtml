/*
 * Copyright (C) 2009 Michael Howell <mhowell123@gmail.com>.
 * Parts copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef render_media_h
#define render_media_h

#include <phonon/videoplayer.h>
#include <rendering/render_replaced.h>
#include <html/HTMLMediaElement.h>

namespace khtml {

class MediaPlayer : public Phonon::VideoPlayer {
Q_OBJECT
public:
    inline MediaPlayer(Phonon::Category category, QWidget *parent = 0) : Phonon::VideoPlayer(category, parent)
    {
    }
    inline explicit MediaPlayer(QWidget* parent = 0) : Phonon::VideoPlayer(parent) {};
};

class RenderMedia : public RenderWidget {
Q_OBJECT
public:
    virtual const char *renderName() const { return "RenderMedia"; }
    virtual bool isMedia() const { return true; }

    void setPlayer(MediaPlayer* player);
    MediaPlayer* player() { return m_player; }
    const MediaPlayer* player() const { return m_player; }
    HTMLMediaElement* mediaElement() { return static_cast<HTMLMediaElement*>(RenderWidget::element()); }
    const HTMLMediaElement* mediaElement() const { return static_cast<const HTMLMediaElement*>(RenderWidget::element()); }

protected:
    bool eventFilter(QObject*, QEvent*);

private Q_SLOTS:
    void slotMetaDataChanged();

private:
    RenderMedia(HTMLMediaElement* element);
    void layout();
    void updateFromElement();
    MediaPlayer* m_player;
    friend class HTMLMediaElement;
};

} //namespace

#endif
