/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *           (C) 2009 Michael Howell <mhowell123@gmail.com>.
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

#ifndef HTMLMediaElement_h
#define HTMLMediaElement_h

#include "html_elementimpl.h"
#include "HTMLElement.h"
#include "ExceptionCode.h"
#include <wtf/PassRefPtr.h>
#include <QPointer>

namespace khtml {

class MediaError;
class TimeRanges;
class RenderMedia;
class MediaPlayer;

class HTMLMediaElement : public HTMLElement {
public:
    HTMLMediaElement(Document*);
    virtual ~HTMLMediaElement();

    virtual void attach();
    virtual void close();

//    virtual void parseAttribute(AttributeImpl *token);
    using DOM::ElementImpl::attributeChanged;
    virtual void attributeChanged(NodeImpl::Id attrId);

    virtual bool isVideo() const { return false; }

    void scheduleLoad();

// DOM API
// error state
    PassRefPtr<MediaError> error() const;

// network state
    String src() const;
    void setSrc(const String&);
    String currentSrc() const;
    
    enum NetworkState { NETWORK_EMPTY, NETWORK_IDLE, NETWORK_LOADING, NETWORK_NO_SOURCE };
    NetworkState networkState() const;
    bool autobuffer() const;
    void setAutobuffer(bool b);
    PassRefPtr<TimeRanges> buffered() const;
    void load(ExceptionCode&);
    String canPlayType(String type);
    

// ready state
    enum ReadyState { HAVE_NOTHING, HAVE_METADATA, HAVE_CURRENT_DATA, HAVE_FUTURE_DATA, HAVE_ENOUGH_DATA };
    ReadyState readyState() const;
    bool seeking() const;

// playback state
    float currentTime() const;
    void setCurrentTime(float, ExceptionCode&);
    float startTime() const;
    float duration() const;
    bool paused() const;
    float defaultPlaybackRate() const;
    void setDefaultPlaybackRate(float, ExceptionCode&);
    float playbackRate() const;
    void setPlaybackRate(float, ExceptionCode&);
    PassRefPtr<TimeRanges> played() const;
    PassRefPtr<TimeRanges> seekable() const;
    bool ended() const;
    bool autoplay() const;    
    void setAutoplay(bool b);
    bool loop() const;
    void setLoop(bool b);
    void play(ExceptionCode&);
    void pause(ExceptionCode&);
    
// controls
    bool controls() const;
    void setControls(bool);
    float volume() const;
    void setVolume(float, ExceptionCode&);
    bool muted() const;
    void setMuted(bool);

    String pickMedia();

protected:
    void checkIfSeekNeeded();

    void setReadyState(ReadyState);

private:
    void loadResource(String& url);
    void updateLoadState();
    
    void updateVolume();
    void updatePlayState();
    bool endedPlayback() const;

protected:
    float m_defaultPlaybackRate;
    NetworkState m_networkState;
    ReadyState m_readyState;
    String m_currentSrc;
    
    RefPtr<MediaError> m_error;
    
    bool m_begun;
    bool m_loadedFirstFrame;
    bool m_autoplaying;
    bool m_autobuffer;
    
    float m_volume;
    bool m_muted;
    
    bool m_paused;
    bool m_seeking;
    
    float m_currentTimeDuringSeek;
    
    unsigned m_previousProgress;
    double m_previousProgressTime;
    bool m_sentStalledEvent;
    
    QPointer<MediaPlayer> m_player;
};

} //namespace

#endif
