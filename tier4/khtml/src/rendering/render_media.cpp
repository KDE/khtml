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

#include "render_media.h"
#include "media_controls.h"
#include <phonon/mediaobject.h>
#include <phonon/videowidget.h>
#include <QVBoxLayout>

const double doubleMax = 999999999.8; // ### numeric_limits<double>::max()
const double doubleInf = 999999999.0; // ### numeric_limits<double>::infinity()

using namespace DOM;

namespace khtml {

RenderMedia::RenderMedia(HTMLMediaElement* element) : RenderWidget(element), m_player(0)
{
    setInline(true); // <video> is an inline element.
    QWidget* container = new QWidget();
    container->setLayout(new QVBoxLayout(container));
    setQWidget(container);
}

void RenderMedia::setPlayer(MediaPlayer* player)
{
    if (m_player == player) return;
    if (m_player) m_player->deleteLater();
    m_player = player;
    connect(player->mediaObject(), SIGNAL(metaDataChanged()), SLOT(slotMetaDataChanged()));
    player->setParent(widget());
    widget()->layout()->addWidget(player);
}

void RenderMedia::layout()
{
    calcWidth();
    calcHeight();

    RenderWidget::layout();

    if (mediaElement()->controls() && widget()->layout()->count() == 1) {
        MediaControls* toolbox = new MediaControls(player(), widget());
	widget()->layout()->addWidget(toolbox);
	if ((!widget()->underMouse()) && mediaElement()->isVideo())
	    toolbox->hide();
	else
	    toolbox->show();
    }
}

bool RenderMedia::eventFilter(QObject* o, QEvent* e)
{
    if (widget()->layout()->count() > 1 && mediaElement()->isVideo()) {
        switch(e->type()) {
        case QEvent::Enter:
	case QEvent::FocusIn:
	    widget()->layout()->itemAt(1)->widget()->show();
	    break;
	case QEvent::Leave:
	case QEvent::FocusOut:
	    widget()->layout()->itemAt(1)->widget()->hide();
	    break;
	default: ;
        }
    }

    return RenderWidget::eventFilter(o, e);
}

void RenderMedia::updateFromElement()
{
    RenderWidget::updateFromElement();
}

void RenderMedia::slotMetaDataChanged()
{
    if (mediaElement()->isVideo()) {
        if (player()->videoWidget()->sizeHint().isValid()) {
	    setIntrinsicWidth(player()->videoWidget()->sizeHint().width());
	    setIntrinsicHeight(player()->videoWidget()->sizeHint().height());
        }
    } else {
        if (widget()->sizeHint().isValid()) {
	    setIntrinsicWidth(widget()->sizeHint().width());
	    setIntrinsicHeight(widget()->sizeHint().height());
        }
	player()->hide();
    }

    setNeedsLayoutAndMinMaxRecalc();
}

}
