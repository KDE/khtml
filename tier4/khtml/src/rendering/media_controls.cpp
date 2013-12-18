/*
 * Copyright (C) 2009 Michael Howell <mhowell123@gmail.com>.
 * Copyright (C) 2009 Germain Garand <germain@ebooksfrance.org>
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

#include "media_controls.h"
#include <QHBoxLayout>
#include <phonon/seekslider.h>
#include <phonon/mediaobject.h>
#include <rendering/render_media.h>
#include <phonon/videowidget.h>
#include <ktogglefullscreenaction.h>
#include <kglobalaccel.h>
#include <klocalizedstring.h>

namespace khtml {

MediaControls::MediaControls(MediaPlayer* mediaPlayer, QWidget* parent) : QWidget(parent)
{
    m_mediaPlayer = mediaPlayer;
    Phonon::MediaObject* mediaObject = m_mediaPlayer->mediaObject();
    setLayout(new QHBoxLayout(this));
    m_play = new QPushButton(QIcon::fromTheme("media-playback-start"), i18n("Play"), this);
    connect(m_play, SIGNAL(clicked()), mediaObject, SLOT(play()));
    layout()->addWidget(m_play);
    m_pause = new QPushButton(QIcon::fromTheme("media-playback-pause"), i18n("Pause"), this);
    connect(m_pause, SIGNAL(clicked()), mediaObject, SLOT(pause()));
    layout()->addWidget(m_pause);
    layout()->addWidget(new Phonon::SeekSlider(mediaObject, this));
    QAction* fsac = new KToggleFullScreenAction(this);
    fsac->setObjectName("KHTMLMediaPlayerFullScreenAction"); // needed for global shortcut activation.
    m_fullscreen = new QToolButton(this);
    m_fullscreen->setDefaultAction(fsac);
    m_fullscreen->setCheckable(true);
    connect(fsac, SIGNAL(toggled(bool)), this, SLOT(slotToggled(bool)));
    layout()->addWidget(m_fullscreen); 

    slotStateChanged(mediaObject->state());
    connect(mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)), SLOT(slotStateChanged(Phonon::State)));
}

void MediaControls::slotToggled(bool t)
{
    if (t) {
        m_mediaPlayer->videoWidget()->enterFullScreen();
        KGlobalAccel::self()->setShortcut(m_fullscreen->defaultAction(), QList<QKeySequence>() << Qt::Key_Escape);
    } else {
        m_mediaPlayer->videoWidget()->exitFullScreen();
        KGlobalAccel::self()->removeAllShortcuts(m_fullscreen->defaultAction());
    }
}

void MediaControls::slotStateChanged(Phonon::State state)
{
    if (state == Phonon::PlayingState) {
        m_play->hide();
	m_pause->show();
    } else {
        m_pause->hide();
	m_play->show();
    }
}

}

