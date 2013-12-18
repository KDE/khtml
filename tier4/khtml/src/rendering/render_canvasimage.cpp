/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2005 Zack Rusin <zack@kde.org>
 * Copyright (C) 2007 Maksim Orlovich <maksim@kde.org>
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

//#define DEBUG_LAYOUT

#include "render_canvasimage.h"
#include "render_canvas.h"

#include <QPainter>
#include <QtCore/QDebug>

#include <css/csshelper.h>
#include <misc/helper.h>
#include <html/html_formimpl.h>
#include <html/html_canvasimpl.h>
#include <xml/dom2_eventsimpl.h>
#include <html/html_documentimpl.h>
#include <imload/canvasimage.h>

#include <math.h>

using namespace DOM;
using namespace khtml;

// -------------------------------------------------------------------------

RenderCanvasImage::RenderCanvasImage(DOM::HTMLCanvasElementImpl* canvasEl)
    : RenderReplaced(canvasEl), imagePainter(canvasEl->getCanvasImage())
{
    setIntrinsicWidth (element()->width());
    setIntrinsicHeight(element()->height());
}

void RenderCanvasImage::paint(PaintInfo& i, int _tx, int _ty)
{
    int x = _tx + m_x;
    int y = _ty + m_y;

    if (shouldPaintBackgroundOrBorder() && i.phase != PaintActionOutline)
        paintBoxDecorations(i, x, y);

    QPainter* p = i.p;

    if (i.phase == PaintActionOutline && style()->outlineWidth() && style()->visibility() == VISIBLE)
        paintOutline(p, x, y, width(), height(), style());

    if (i.phase != PaintActionForeground && i.phase != PaintActionSelection)
        return;

    //bool isPrinting = (i.p->device()->devType() == QInternal::Printer);
    //bool drawSelectionTint = (selectionState() != SelectionNone) && !isPrinting;
    if (i.phase == PaintActionSelection) {
        if (selectionState() == SelectionNone) {
            return;
        }
        //drawSelectionTint = false;
    }

    int cWidth = contentWidth();
    int cHeight = contentHeight();
    if ( !cWidth )  cWidth = 300;
    if ( !cHeight ) cHeight = 150;
    int leftBorder = borderLeft();
    int topBorder = borderTop();
    int leftPad = paddingLeft();
    int topPad = paddingTop();

    x += leftBorder + leftPad;
    y += topBorder + topPad;

    element()->getContext2D()->commit(); // Make sure everything is up-to-date
    imagePainter.setSize(QSize(cWidth, cHeight));
    imagePainter.paint(x, y, p, 0, 0);

    // if (drawSelectionTint) {
//         QBrush brush(selectionColor(p));
//         QRect selRect(selectionRect());
//         p->fillRect(selRect.x(), selRect.y(), selRect.width(), selRect.height(), brush);
//     }
}

void RenderCanvasImage::layout()
{
    KHTMLAssert( needsLayout());
    KHTMLAssert( minMaxKnown() );

    calcWidth();
    calcHeight();

    setNeedsLayout(false);
}

void RenderCanvasImage::updateFromElement()
{
    int newWidth  = element()->width();
    int newHeight = element()->height();
    if (intrinsicHeight() != newHeight || intrinsicWidth()  != newWidth) {
        setIntrinsicWidth (newWidth);
        setIntrinsicHeight(newHeight);
        setNeedsLayoutAndMinMaxRecalc();
    }
    
    if (!needsLayout())
        repaint();
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
