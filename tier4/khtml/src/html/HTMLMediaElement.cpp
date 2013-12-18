/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *           (C) 2009 Michael Howell <mhowell123@gmail.com>.
 *           (C) 2010 Allan Sandfeld Jensen <sandfeld@kde.org>.
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

#include <wtf/Platform.h>

#include "HTMLMediaElement.h"
#include "html_element.h"
#include "HTMLSourceElement.h"
#include "HTMLDocument.h"
#include "MediaError.h"
#include "TimeRanges.h"
#include "css/cssstyleselector.h"
#include "css/cssproperties.h"
#include "css/cssvalues.h"
#include "css/csshelper.h"
#include <phonon/mediaobject.h>
#include <phonon/backendcapabilities.h>
#include <rendering/render_media.h>
#include <rendering/render_style.h>

const double doubleMax = 999999999.8; // ### numeric_limits<double>::max()
const double doubleInf = 999999999.0; // ### numeric_limits<double>::infinity()

using namespace DOM;
namespace khtml {

HTMLMediaElement::HTMLMediaElement(Document* doc)
    : HTMLElement(doc)
    , m_defaultPlaybackRate(1.0f)
    , m_networkState(NETWORK_EMPTY)
    , m_readyState(HAVE_NOTHING)
    , m_begun(false)
    , m_loadedFirstFrame(false)
    , m_autoplaying(true)
    , m_autobuffer(true)
    , m_volume(0.5f)
    , m_muted(false)
    , m_paused(true)
    , m_seeking(false)
    , m_currentTimeDuringSeek(0)
    , m_previousProgress(0)
    , m_previousProgressTime(doubleMax)
    , m_sentStalledEvent(false)
    , m_player(new MediaPlayer())
{
}

void HTMLMediaElement::attach()
{
    assert(!attached());
    assert(!m_render);
    assert(parentNode());

    RenderStyle* _style = document()->styleSelector()->styleForElement(this);
    _style->ref();
    if (parentNode()->renderer() && parentNode()->renderer()->childAllowed() &&
        _style->display() != NONE)
    {
        m_render = new (document()->renderArena()) RenderMedia(this);
        static_cast<RenderMedia*>(m_render)->setPlayer(m_player.data());
        m_render->setStyle(_style);
        parentNode()->renderer()->addChild(m_render, nextRenderer());
    }
    _style->deref();

    NodeBaseImpl::attach();
    if (m_render)
	m_render->updateFromElement();
    setRenderer(m_render);
    updateLoadState();
}

void HTMLMediaElement::close()
{
    HTMLElement::close();
    updateLoadState();
    if (renderer())
        renderer()->updateFromElement();
}

HTMLMediaElement::~HTMLMediaElement()
{
    if (m_player) m_player->deleteLater();
}

void HTMLMediaElement::attributeChanged(NodeImpl::Id attrId)
{
    HTMLElement::attributeChanged(attrId);

    if (attrId == ATTR_SRC) {
        // 3.14.9.2.
        // change to src attribute triggers load()
        if (inDocument() && m_networkState == NETWORK_EMPTY)
            scheduleLoad();
	updateLoadState();
    } if (attrId == ATTR_CONTROLS) {
        /*if (!isVideo() && attached() && (controls() != (renderer() != 0))) {
            detach();
            attach();
        }*/
        if (renderer())
            renderer()->updateFromElement();
    }
}

void HTMLMediaElement::scheduleLoad()
{
    // qDebug() << "not implemented";
}

String serializeTimeOffset(float time)
{
    QString timeString = QString::number(time);
    // FIXME serialize time offset values properly (format not specified yet)
    timeString.append("s");
    return timeString;
}

PassRefPtr<MediaError> HTMLMediaElement::error() const 
{
    return m_error;
}

String HTMLMediaElement::src() const
{
    return document()->completeURL(getAttribute(ATTR_SRC).string());
}

void HTMLMediaElement::setSrc(const String& url)
{
    setAttribute(ATTR_SRC, url);
}

String HTMLMediaElement::currentSrc() const
{
    return m_currentSrc;
}

HTMLMediaElement::NetworkState HTMLMediaElement::networkState() const
{
    return m_networkState;
}

bool HTMLMediaElement::autobuffer() const
{
   return m_autobuffer;
}

void HTMLMediaElement::setAutobuffer(bool b)
{
    m_autobuffer = b;
}

void HTMLMediaElement::load(ExceptionCode&)
{
    loadResource(m_currentSrc);
}

void HTMLMediaElement::loadResource(String &url)
{
    QUrl kurl(url.string());
    if (!m_player)
        return;
    if (autoplay())
        m_player->play(kurl);
    else
        m_player->load(kurl);
}

void HTMLMediaElement::updateLoadState()
{
    String url = pickMedia();
    if (currentSrc() != url) {
	m_currentSrc = url;
	if (m_autobuffer) {
	    loadResource(url);
	}
    }
}

String HTMLMediaElement::canPlayType(String type)
{
    QString theType = type.string().simplified();
    int paramsIdx = theType.indexOf(';');
    bool hasParams = (paramsIdx > 0 );
    // FIXME: Phonon doesn't provide the API to handle codec parameters yet
    if (hasParams)
        theType.truncate(paramsIdx);
    if (theType == QLatin1String("audio/ogg") || theType == QLatin1String("video/ogg"))
        theType = QLatin1String("application/ogg");
    if (Phonon::BackendCapabilities::isMimeTypeAvailable(theType))
        return "probably";
    if (theType == QLatin1String("application/octet-stream") && hasParams)
        return "";
    return "maybe";
}

void HTMLMediaElement::setReadyState(ReadyState state)
{
    // 3.14.9.6. The ready states
    if (m_readyState == state)
        return;
    
    // ###

    updatePlayState();
}

HTMLMediaElement::ReadyState HTMLMediaElement::readyState() const
{
    return m_readyState;
}

bool HTMLMediaElement::seeking() const
{
    return m_seeking;
}

// playback state
float HTMLMediaElement::currentTime() const
{
    if (!m_player)
        return 0;
    if (m_seeking)
        return m_currentTimeDuringSeek;
    return m_player->currentTime();
}

void HTMLMediaElement::setCurrentTime(float time, ExceptionCode& ec)
{
    Q_UNUSED(time);
    Q_UNUSED(ec);
    //    seek(time, ec);
}

float HTMLMediaElement::startTime() const
{
    return 0.0f;
}

float HTMLMediaElement::duration() const
{
    return m_player ? m_player->totalTime() : 0;
}

bool HTMLMediaElement::paused() const
{
    return m_paused;
}

float HTMLMediaElement::defaultPlaybackRate() const
{
    return m_defaultPlaybackRate;
}

void HTMLMediaElement::setDefaultPlaybackRate(float rate, ExceptionCode& ec)
{
    if (rate == 0.0f) {
        ec = DOMException::NOT_SUPPORTED_ERR;
        return;
    }
    if (m_defaultPlaybackRate != rate) {
        m_defaultPlaybackRate = rate;
        // ### dispatchEventAsync(ratechangeEvent);
    }
}

float HTMLMediaElement::playbackRate() const
{
    return 0; // stub...
}

void HTMLMediaElement::setPlaybackRate(float rate, ExceptionCode& ec)
{
    Q_UNUSED(rate);
    Q_UNUSED(ec);
    // stub
    #if 0
    if (rate == 0.0f) {
        ec = DOMException::NOT_SUPPORTED_ERR;
        return;
    }
    if (m_player && m_player->rate() != rate) {
        m_player->setRate(rate);
        // ### dispatchEventAsync(ratechangeEvent);
    }
    #endif
}

bool HTMLMediaElement::ended() const
{
    return endedPlayback();
}

bool HTMLMediaElement::autoplay() const
{
    return hasAttribute(ATTR_AUTOPLAY);
}

void HTMLMediaElement::setAutoplay(bool b)
{
    setBooleanAttribute(ATTR_AUTOPLAY, b);
}

bool HTMLMediaElement::loop() const
{
    return hasAttribute(ATTR_LOOP);
}

void HTMLMediaElement::setLoop(bool b)
{
    setBooleanAttribute(ATTR_LOOP, b);
}

void HTMLMediaElement::play(ExceptionCode& ec)
{
    // 3.14.9.7. Playing the media resource
    if (!m_player || networkState() == NETWORK_EMPTY) {
        ec = 0;
        load(ec);
        if (ec)
            return;
    }
    ExceptionCode unused;
    if (endedPlayback()) {
        // ### seek(effectiveStart(), unused);
    }
    setPlaybackRate(defaultPlaybackRate(), unused);
    
    if (m_paused) {
        m_paused = false;
        // ### dispatchEventAsync(playEvent);
    }

    m_autoplaying = false;
    
    updatePlayState();
}

void HTMLMediaElement::pause(ExceptionCode& ec)
{
    // 3.14.9.7. Playing the media resource
    if (!m_player || networkState() == NETWORK_EMPTY) {
        ec = 0;
        load(ec);
        if (ec)
            return;
    }

    if (!m_paused) {
        m_paused = true;
        // ### dispatchEventAsync(timeupdateEvent);
        // ### dispatchEventAsync(pauseEvent);
    }

    m_autoplaying = false;
    
    updatePlayState();
}

bool HTMLMediaElement::controls() const
{
    return hasAttribute(ATTR_CONTROLS);
}

void HTMLMediaElement::setControls(bool b)
{
    setBooleanAttribute(ATTR_CONTROLS, b);
}

float HTMLMediaElement::volume() const
{
    return m_volume;
}

void HTMLMediaElement::setVolume(float vol, ExceptionCode& ec)
{
    if (vol < 0.0f || vol > 1.0f) {
        ec = DOMException::INDEX_SIZE_ERR;
        return;
    }
    
    if (m_volume != vol) {
        m_volume = vol;
        updateVolume();
        // ### dispatchEventAsync(volumechangeEvent);
    }
}

bool HTMLMediaElement::muted() const
{
    return m_muted;
}

void HTMLMediaElement::setMuted(bool muted)
{
    if (m_muted != muted) {
        m_muted = muted;
        updateVolume();
        // ### dispatchEventAsync(volumechangeEvent);
    }
}

String HTMLMediaElement::pickMedia()
{
    if (!document())
	return String();
    // 3.14.9.2. Location of the media resource
    String mediaSrc = getAttribute(ATTR_SRC);
    String maybeSrc;
    if (mediaSrc.isEmpty()) {
        for (NodeImpl* n = firstChild(); n; n = n->nextSibling()) {
            if (n->id() == ID_SOURCE) {
                String match = "maybe";
                HTMLSourceElement* source = static_cast<HTMLSourceElement*>(n);
                if (!source->hasAttribute(ATTR_SRC))
                    continue;
                if (source->hasAttribute(ATTR_TYPE)) {
                    String type = source->type();
                    match = canPlayType(type);
                }
                if (match == "maybe" && maybeSrc.isEmpty())
                    maybeSrc = source->src().string();
                else
                if (match == "probably") {
                    mediaSrc = source->src().string();
                    break;
                }
            }
        }
    }
    if (mediaSrc.isEmpty())
        mediaSrc = maybeSrc;
    if (mediaSrc.isEmpty())
	return mediaSrc;
    DocLoader* loader = document()->docLoader();
    if (!loader || !loader->willLoadMediaElement(mediaSrc))
	return String();
    mediaSrc = document()->completeURL(mediaSrc.string());
    return mediaSrc;
}

void HTMLMediaElement::checkIfSeekNeeded()
{
    // ###
}

PassRefPtr<TimeRanges> HTMLMediaElement::buffered() const
{
    // FIXME real ranges support
    #if 0
    if (!m_player || !m_player->maxTimeBuffered())
        return new TimeRanges;
    return new TimeRanges(0, m_player->maxTimeBuffered());
    #endif
    return new TimeRanges(0, 0.0f); // stub
}

PassRefPtr<TimeRanges> HTMLMediaElement::played() const
{
    // FIXME track played
    return new TimeRanges;
}

PassRefPtr<TimeRanges> HTMLMediaElement::seekable() const
{
    #if 0
    // FIXME real ranges support
    if (!m_player || !m_player->maxTimeSeekable())
        return new TimeRanges;
    return new TimeRanges(0, m_player->maxTimeSeekable());
    #endif
    return new TimeRanges(0, 0.0f); // stub
}

bool HTMLMediaElement::endedPlayback() const
{
#if 0
    return networkState() >= LOADED_METADATA && currentTime() >= effectiveEnd() && currentLoop() == playCount() - 1;
#endif
    return m_player && m_player->mediaObject()->remainingTime() == 0;
}

void HTMLMediaElement::updateVolume()
{
    if (!m_player)
        return;

    m_player->setVolume(m_muted ? 0 : m_volume);
    
    if (renderer())
        renderer()->updateFromElement();
}

void HTMLMediaElement::updatePlayState()
{
    if (!m_player)
        return;
    if (m_autoplaying)
        return;
    if (m_paused && !m_player->isPaused())
        m_player->pause();
    if (!m_paused && !m_player->isPlaying())
        m_player->play();
}

}
// kate: indent-width 4; replace-tabs on; tab-width 8; space-indent on;
