/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2002-2007 Apple Computer, Inc.
 *           (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 *           (C) 2007 Germain Garand (germain@ebooksfrance.org)
 *           (C) 2008 Fredrik Höglund (fredrik@kde.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
// -------------------------------------------------------------------------
//#define DEBUG_LAYOUT
//#define CLIP_DEBUG


#include <QPainter>

#include "misc/loader.h"
#include "rendering/render_form.h"
#include "rendering/render_replaced.h"
#include "rendering/render_canvas.h"
#include "rendering/render_table.h"
#include "rendering/render_inline.h"
#include "rendering/render_block.h"
#include "rendering/render_line.h"
#include "rendering/render_layer.h"
#include "xml/dom_nodeimpl.h"
#include "xml/dom_docimpl.h"
#include "xml/dom2_eventsimpl.h"
#include "html/html_elementimpl.h"

#include <QWheelEvent>
#include <khtmlview.h>
#include <QDebug>
#include <assert.h>


using namespace DOM;
using namespace khtml;

#define TABLECELLMARGIN -0x4000

RenderBox::RenderBox(DOM::NodeImpl* node)
    : RenderContainer(node)
{
    m_minWidth = -1;
    m_maxWidth = -1;
    m_width = m_height = 0;
    m_x = 0;
    m_y = 0;
    m_marginTop = 0;
    m_marginBottom = 0;
    m_marginLeft = 0;
    m_marginRight = 0;
    m_staticX = 0;
    m_staticY = 0;

    m_placeHolderBox = 0;
    m_layer = 0;
}

RenderBlock* RenderBox::createAnonymousBlock()
{
    RenderStyle *newStyle = new RenderStyle();
    newStyle->inheritFrom(style());
    newStyle->setDisplay(BLOCK);

    RenderBlock *newBox = new (renderArena()) RenderBlock(document() /* anonymous*/);
    newBox->setStyle(newStyle);
    return newBox;
}

void RenderBox::restructureParentFlow() {
    if (!parent() || parent()->childrenInline() == isInline())
        return;
    // We have gone from not affecting the inline status of the parent flow to suddenly
    // having an impact.  See if there is a mismatch between the parent flow's
    // childrenInline() state and our state.
    if (!isInline()) {
        if (parent()->isRenderInline()) {
            // We have to split the parent flow.
            RenderInline* parentInline = static_cast<RenderInline*>(parent());
            RenderBlock* newBox = parentInline->createAnonymousBlock();

            RenderFlow* oldContinuation = parent()->continuation();
            parentInline->setContinuation(newBox);

            RenderObject* beforeChild = nextSibling();
            parent()->removeChildNode(this);
            parentInline->splitFlow(beforeChild, newBox, this, oldContinuation);
        }
        else if (parent()->isRenderBlock()) {
            RenderBlock* p = static_cast<RenderBlock*>(parent());
            p->makeChildrenNonInline();
            if (p->isAnonymousBlock() && p->parent())
                p->parent()->removeSuperfluousAnonymousBlockChild( p );
            // we might be deleted now
        }
    }
    else {
        // An anonymous block must be made to wrap this inline.
        RenderBlock* box = createAnonymousBlock();
        parent()->insertChildNode(box, this);
        box->appendChildNode(parent()->removeChildNode(this));
    }
}

static inline bool overflowAppliesTo(RenderObject* o)
{
     // css 2.1-11.1.1
     // 1) overflow only applies to non-replaced block-level elements, table cells, and inline-block elements
     if (o->isRenderBlock() || o->isTableRow() || o->isTableSection())
         // 2) overflow on root applies to the viewport (cf. KHTMLView::layout)
         if (!o->isRoot())
             // 3) overflow on body may apply to the viewport...
             if (!o->isBody()
                   // ...but only for HTML documents...
                   || !o->document()->isHTMLDocument()
                   // ...and only when the root has a visible overflow
                   || !o->document()->documentElement()->renderer()
                   || !o->document()->documentElement()->renderer()->style()
                   ||  o->document()->documentElement()->renderer()->style()->hidesOverflow())
                 return true;

     return false;
}

void RenderBox::setStyle(RenderStyle *_style)
{
    bool affectsParent = style() && isFloatingOrPositioned() &&
         (!_style->isFloating() && _style->position() != PABSOLUTE && _style->position() != PFIXED) &&
         parent() && (parent()->isBlockFlow() || parent()->isInlineFlow());

    RenderContainer::setStyle(_style);

    // The root always paints its background/border.
    if (isRoot())
        setShouldPaintBackgroundOrBorder(true);

    switch(_style->display())
    {
    case INLINE:
    case INLINE_BLOCK:
    case INLINE_TABLE:
        setInline(true);
        break;
    case RUN_IN:
        if (isInline() && parent() && parent()->childrenInline())
            break;
    default:
        setInline(false);
    }

    switch(_style->position())
    {
    case PABSOLUTE:
    case PFIXED:
        setPositioned(true);
        break;
    default:
        setPositioned(false);
        if( !isTableCell() && _style->isFloating() )
            setFloating(true);

        if( _style->position() == PRELATIVE )
            setRelPositioned(true);
    }

    if (overflowAppliesTo(this) && _style->hidesOverflow())
        setHasOverflowClip();

    if (requiresLayer()) {
        if (!m_layer) {
            m_layer = new (renderArena()) RenderLayer(this);
            m_layer->insertOnlyThisLayer();
            if (parent() && containingBlock())
                m_layer->updateLayerPosition();
        }
    }
    else if (m_layer && !isCanvas()) {
        m_layer->removeOnlyThisLayer();
        m_layer = 0;
    }

    if (m_layer)
        m_layer->styleChanged();

    if (style()->outlineWidth() > 0 && style()->outlineSize() > maximalOutlineSize(PaintActionOutline))
        static_cast<RenderCanvas*>(document()->renderer())->setMaximalOutlineSize(style()->outlineSize());
    if (affectsParent)
        restructureParentFlow();
}

RenderBox::~RenderBox()
{
    //qDebug() << "Element destructor: this=" << nodeName().string();
}

void RenderBox::detach()
{
    RenderLayer* layer = m_layer;
    RenderArena* arena = renderArena();

    detachRemainingChildren();
    RenderContainer::detach();

    if (layer)
        layer->detach(arena);
}

void RenderBox::detachRemainingChildren()
{
    while (firstChild()) {
        if (firstChild()->style()->styleType() == RenderStyle::FIRST_LETTER && !firstChild()->isText()) {
             // First letters are destroyed by their remaining text fragment.
             // We have to remove their references to parent here, however, 
             // since it may be destroyed once we get to them
            firstChild()->remove(); 
        } else {
	    // Destroy any (most likely anonymous) children remaining in the render tree
            if (firstChild()->element())
                firstChild()->element()->setRenderer(0);
            firstChild()->detach();
        }
    }
}

void RenderBox::removeChild(RenderObject* oldChild)
{
    // We do this here instead of in removeChildNode, since the only extremely low-level uses of remove/appendChildNode
    // cannot affect the positioned object list, and the floating object list is irrelevant (since the list gets cleared on
    // layout anyway).
    oldChild->removeFromObjectLists();

    removeChildNode(oldChild);
}

InlineBox* RenderBox::createInlineBox(bool /*makePlaceHolderBox*/, bool /*isRootLineBox*/)
{
    if (m_placeHolderBox)
        m_placeHolderBox->detach(renderArena(), true/*noRemove*/);
    return (m_placeHolderBox = new (renderArena()) PlaceHolderBox(this));
}

void RenderBox::deleteInlineBoxes(RenderArena* /*arena*/)
{
    if (m_placeHolderBox) {
        m_placeHolderBox->detach( renderArena(), true /*noRemove*/ );
        m_placeHolderBox = 0;
    }
}

void RenderBox::dirtyInlineBoxes(bool fullLayout, bool /*isRootLineBox*/)
{
    if (m_placeHolderBox) {
        if (fullLayout) {
            m_placeHolderBox->detach(renderArena(), true /*noRemove*/ );
            m_placeHolderBox = 0;
        } else
            m_placeHolderBox->dirtyInlineBoxes();
    }
}

short RenderBox::contentWidth() const
{
    short w = m_width - style()->borderLeftWidth() - style()->borderRightWidth();
    w -= paddingLeft() + paddingRight();

    if (m_layer && scrollsOverflowY())
        w -= m_layer->verticalScrollbarWidth();

    //qDebug() << "RenderBox::contentWidth(2) = " << w;
    return w;
}

int RenderBox::contentHeight() const
{
    int h = m_height - style()->borderTopWidth() - style()->borderBottomWidth();
    h -= paddingTop() + paddingBottom();

    if (m_layer && scrollsOverflowX())
        h -= m_layer->horizontalScrollbarHeight();

    return h;
}

void RenderBox::setPos( int xPos, int yPos )
{
    m_x = xPos; m_y = yPos;
}

short RenderBox::width() const
{
    return m_width;
}

int RenderBox::height() const
{
    return m_height;
}

void RenderBox::setWidth( int width )
{
    m_width = width;
}

void RenderBox::setHeight( int height )
{
    m_height = height;
}

int RenderBox::calcBoxHeight(int h) const
{
    if (style()->boxSizing() == CONTENT_BOX)
        h += borderTop() + borderBottom() + paddingTop() + paddingBottom();

    return h;
}

int RenderBox::calcBoxWidth(int w) const
{
    if (style()->boxSizing() == CONTENT_BOX)
        w += borderLeft() + borderRight() + paddingLeft() + paddingRight();

    return w;
}

int RenderBox::calcContentHeight(int h) const
{
    if (style()->boxSizing() == BORDER_BOX)
        h -= borderTop() + borderBottom() + paddingTop() + paddingBottom();

    return qMax(0, h);
}

int RenderBox::calcContentWidth(int w) const
{
    if (style()->boxSizing() == BORDER_BOX)
        w -= borderLeft() + borderRight() + paddingLeft() + paddingRight();

    return qMax(0, w);
}

// --------------------- painting stuff -------------------------------

void RenderBox::paint(PaintInfo& i, int _tx, int _ty)
{
    _tx += m_x;
    _ty += m_y;

    if (hasOverflowClip() && m_layer)
        m_layer->subtractScrollOffset(_tx, _ty);

    // default implementation. Just pass things through to the children
    for(RenderObject* child = firstChild(); child; child = child->nextSibling())
        child->paint(i, _tx, _ty);
}

void RenderBox::paintRootBoxDecorations(PaintInfo& paintInfo, int _tx, int _ty)
{
    //qDebug() << renderName() << "::paintRootBoxDecorations()" << _tx << "/" << _ty;
    const BackgroundLayer* bgLayer = style()->backgroundLayers();
    QColor bgColor = style()->backgroundColor();
    if (document()->isHTMLDocument() && !style()->hasBackground()) {
        // Locate the <body> element using the DOM.  This is easier than trying
        // to crawl around a render tree with potential :before/:after content and
        // anonymous blocks created by inline <body> tags etc.  We can locate the <body>
        // render object very easily via the DOM.
        HTMLElementImpl* body = document()->body();
        RenderObject* bodyObject = (body && body->id() == ID_BODY) ? body->renderer() : 0;

        if (bodyObject) {
            bgLayer = bodyObject->style()->backgroundLayers();
            bgColor = bodyObject->style()->backgroundColor();
        }
    }

    if( !bgColor.isValid() && canvas()->view())
        bgColor = canvas()->view()->palette().color(QPalette::Active, QPalette::Base);

    int w = width();
    int h = height();

    //    qDebug() << "width = " << w;

    int rw, rh;
    if (canvas()->view()) {
        rw = canvas()->view()->contentsWidth();
        rh = canvas()->view()->contentsHeight();
    } else {
        rw = canvas()->docWidth();
        rh = canvas()->docHeight();
    }

    //    qDebug() << "rw = " << rw;

    int bx = _tx - marginLeft();
    int by = _ty - marginTop();
    int bw = qMax(w + marginLeft() + marginRight() + borderLeft() + borderRight(), rw);
    int bh = qMax(h + marginTop() + marginBottom() + borderTop() + borderBottom(), rh);

    // CSS2 14.2:
    // " The background of the box generated by the root element covers the entire canvas."
    // hence, paint the background even in the margin areas (unlike for every other element!)
    // I just love these little inconsistencies .. :-( (Dirk)
    QRect cr = paintInfo.r.intersected(QRect(bx, by, bw, bh));
    paintAllBackgrounds(paintInfo.p, bgColor, bgLayer, cr, bx, by, bw, bh);

    if(style()->hasBorder())
        paintBorder( paintInfo.p, _tx, _ty, w, h, style() );
}

void RenderBox::paintBoxDecorations(PaintInfo& paintInfo, int _tx, int _ty)
{
    //qDebug() << renderName() << "::paintDecorations()";

    if(isRoot())
        return paintRootBoxDecorations(paintInfo, _tx, _ty);

    int w = width();
    int h = height() + borderTopExtra() + borderBottomExtra();
    _ty -= borderTopExtra();
    QRect cr = QRect(_tx, _ty, w, h).intersected(paintInfo.r);

   // The <body> only paints its background if the root element has defined a background
   // independent of the body.  Go through the DOM to get to the root element's render object,
   // since the root could be inline and wrapped in an anonymous block.

   if (!isBody() || !document()->isHTMLDocument() || document()->documentElement()->renderer()->style()->hasBackground())
        paintAllBackgrounds(paintInfo.p, style()->backgroundColor(), style()->backgroundLayers(), cr, _tx, _ty, w, h);

    if(style()->hasBorder()) {
        paintBorder(paintInfo.p, _tx, _ty, w, h, style());
    }
}

void RenderBox::paintAllBackgrounds(QPainter *p, const QColor& c, const BackgroundLayer* bgLayer, QRect clipr, int _tx, int _ty, int w, int height)
{
    if (!bgLayer) return;
    paintAllBackgrounds(p, c, bgLayer->next(), clipr, _tx, _ty, w, height);
    paintOneBackground(p, c, bgLayer, clipr, _tx, _ty, w, height);
}

void RenderBox::paintOneBackground(QPainter *p, const QColor& c, const BackgroundLayer* bgLayer, QRect clipr, int _tx, int _ty, int w, int height)
{
    paintBackgroundExtended(p, c, bgLayer, clipr, _tx, _ty, w, height,
                            borderLeft(), borderRight(), paddingLeft(), paddingRight(),
                            borderTop(), borderBottom(), paddingTop(), paddingBottom());
}

static void calculateBackgroundSize(const BackgroundLayer* bgLayer, int& scaledWidth, int& scaledHeight)
{
    CachedImage* bg = bgLayer->backgroundImage();

    if (bgLayer->isBackgroundSizeSet()) {
        if (bgLayer->backgroundSizeType() == BGSLENGTH) {
            int w = scaledWidth;
            int h = scaledHeight;

            Length bgWidth = bgLayer->backgroundSize().width;
            Length bgHeight = bgLayer->backgroundSize().height;

            if (bgWidth.isFixed())
                w = bgWidth.value();
            else if (bgWidth.isPercent())
                w = bgWidth.width(scaledWidth);

            if (bgHeight.isFixed())
                h = bgHeight.value();
            else if (bgHeight.isPercent())
                h = bgHeight.width(scaledHeight);

            // If one of the values is auto we have to use the appropriate
            // scale to maintain our aspect ratio.
            if (bgWidth.isAuto() && !bgHeight.isAuto())
                w = bg->pixmap_size().width() * h / bg->pixmap_size().height();
            else if (!bgWidth.isAuto() && bgHeight.isAuto())
                h = bg->pixmap_size().height() * w / bg->pixmap_size().width();
            else if (bgWidth.isAuto() && bgHeight.isAuto()) {
                // If both width and height are auto, we just want to use the image's
                // intrinsic size.
                w = bg->pixmap_size().width();
                h = bg->pixmap_size().height();
            }
            scaledWidth = qMax(1, w);
            scaledHeight = qMax(1, h);
        } else {
            // 'cover' and 'contain' scaling ratio
            assert( bgLayer->backgroundSizeType() == BGSCONTAIN ||
                    bgLayer->backgroundSizeType() == BGSCOVER);
            float iw = bg->pixmap_size().width();
            float ih = bg->pixmap_size().height();
            float w = scaledWidth/iw;        
            float h = scaledHeight/ih;
            float r = (bgLayer->backgroundSizeType() == BGSCONTAIN) ? qMin(w,h) : qMax(w,h);
            scaledWidth = qMax(1, static_cast<int>(iw*r));
            scaledHeight = qMax(1, static_cast<int>(ih*r));
        }
    } else {
        scaledWidth = bg->pixmap_size().width();
        scaledHeight = bg->pixmap_size().height();
    }
}

void RenderBox::paintBackgroundExtended(QPainter *p, const QColor &c, const BackgroundLayer* bgLayer, QRect clipr,
                                        int _tx, int _ty, int w, int h,
                                        int bleft, int bright, int pleft, int pright, int btop, int bbottom, int ptop, int pbottom)
{
    if (!clipr.isValid())
	return;

    bool needRestore = false;

    if (bgLayer->backgroundClip() != BGBORDER) {
        // Clip to the padding or content boxes as necessary.
        bool includePadding = bgLayer->backgroundClip() == BGCONTENT;
        int x = _tx + bleft + (includePadding ? pleft : 0);
        int y = _ty + btop + (includePadding ? ptop : 0);
        int width = w - bleft - bright - (includePadding ? pleft + pright : 0);
        int height = h - btop - bbottom - (includePadding ? ptop + pbottom : 0);
        p->save();
        p->setClipRect(QRect(x, y, width, height));
        needRestore = true;
    }

    // Create the border-radius clip path
    QPainterPath path;
    if (style()->hasBorderRadius()) {
        path = borderRadiusClipPath(bgLayer, _tx, _ty, w, h, bleft, bright, btop, bbottom);

        // Avoid saving the painter state twice
        if (!needRestore) {
            p->save();
            needRestore = true;
        }
        p->setRenderHint(QPainter::Antialiasing, true);
    }

    CachedImage* bg = bgLayer->backgroundImage();
    bool shouldPaintBackgroundImage = bg && bg->isComplete() && !bg->isTransparent() && !bg->isErrorImage() && canvas()->printImages();
    QColor bgColor = c;

    // the "bottom" of the page can't be transparent
    // unless this is a subframe
    if (!bgLayer->next() && isRoot())
    {
        KHTMLView* v = canvas()->view();
        if (bgColor.alpha() != 255) {
            if (v && v->m_kwp->isRedirected()) {
                RenderStyle* ws = v->m_kwp->renderWidget()->style();
                // only set static background on transparent subframes if the underlying RenderWidget background
                // has something to show through.
                if (!ws || !ws->backgroundColor().isValid() || ws->backgroundColor().alpha() != 255
                        || ws->hasBackgroundImage())
                    v->setHasStaticBackground();
            } else {
                if (bgColor.alpha() == 0)
                    bgColor = p->background().color();
                bgColor.setAlpha(255);
            }
        }
    }

    // Paint the color first underneath all images.
    if (!bgLayer->next() && bgColor.isValid() && qAlpha(bgColor.rgba()) > 0) {
        if (!path.isEmpty())
            p->fillPath(path, bgColor);
        else
            p->fillRect(clipr.x(), clipr.y(), clipr.width(), clipr.height(), bgColor);
    }

    // If we're one of the higher-up layers, make sure the color we
    // pass to CachedImage::tiled_pixmap below is invalid, so it
    // doesn't preblend upper-layers with the color.
    if (bgLayer->next())
        bgColor = QColor();

    // no progressive loading of the background image
    if (shouldPaintBackgroundImage) {
        int sx = 0;
        int sy = 0;
        int rw = 0;
        int rh = 0;
        int cw,ch;
        int cx,cy;
        int scaledImageWidth, scaledImageHeight;

        // CSS2 chapter 14.2.1

        if (bgLayer->backgroundAttachment() != BGAFIXED) {
            //scroll
            int hpab = 0, vpab = 0, left = 0, top = 0; // Init to 0 for background-origin of 'border'
            if (bgLayer->backgroundOrigin() != BGBORDER) {
                hpab += bleft + bright;
                vpab += btop + bbottom;
                left += bleft;
                top += btop;
                if (bgLayer->backgroundOrigin() == BGCONTENT) {
                    hpab += pleft + pright;
                    vpab += ptop + pbottom;
                    left += pleft;
                    top += ptop;
                }
            }

            int pw, ph;
            pw = w - hpab;
            ph = h - vpab;
            if (isRoot()) {
                // the root's background box 'spills out' to cover the whole canvas, so we have to
                // go back to its true edge for the purpose of computing background-size
                // and honouring background-origin
                rw = width() - hpab;
                rh = height() - vpab;
                left += marginLeft();
                hpab += marginLeft() + marginRight();
                vpab += marginTop() + marginBottom();
                top += marginTop();
                scaledImageWidth = rw;
                scaledImageHeight = rh;
            } else {
                scaledImageWidth = pw;
                scaledImageHeight = ph;
            }
            calculateBackgroundSize(bgLayer, scaledImageWidth, scaledImageHeight);

            EBackgroundRepeat bgr = bgLayer->backgroundRepeat();
            if (bgr == NO_REPEAT || bgr == REPEAT_Y) {
                cw = scaledImageWidth;
                int xPosition;
                if (isRoot())
                    xPosition = bgLayer->backgroundXPosition().minWidthRounded(rw-scaledImageWidth);
                else
                    xPosition = bgLayer->backgroundXPosition().minWidthRounded(pw-scaledImageWidth);
                if ( xPosition >= 0 ) {
                    cx = _tx + xPosition;
                    cw = qMin(scaledImageWidth, pw - xPosition);
                }
                else {
                    cx = _tx;
                    if (scaledImageWidth > 0) {
                        sx = -xPosition;
                        cw = qMin(scaledImageWidth+xPosition, pw);
                    }
                }
                cx += left;
            } else {
                // repeat over x
                cw = w;
                cx = _tx;
                if (scaledImageWidth > 0) {
                    int xPosition;
                    if (isRoot())
                        xPosition = bgLayer->backgroundXPosition().minWidthRounded(rw-scaledImageWidth);
                    else
                        xPosition = bgLayer->backgroundXPosition().minWidthRounded(pw-scaledImageWidth);
                    sx = scaledImageWidth - (xPosition % scaledImageWidth);
                    sx -= left % scaledImageWidth;
                }
            }
            if (bgr == NO_REPEAT || bgr == REPEAT_X) {
                ch = scaledImageHeight;
                int yPosition;
                if (isRoot())
                    yPosition = bgLayer->backgroundYPosition().minWidthRounded(rh - scaledImageHeight);
                else
                    yPosition = bgLayer->backgroundYPosition().minWidthRounded(ph - scaledImageHeight);
                if ( yPosition >= 0 ) {
                    cy = _ty + yPosition;
                    ch = qMin(ch, ph - yPosition);
                }
                else {
                    cy = _ty;
                    if (scaledImageHeight > 0) {
                        sy = -yPosition;
                        ch = qMin(scaledImageHeight+yPosition, ph);
                    }
                }

                cy += top;
            } else {
                // repeat over y
                ch = h;
                cy = _ty;
                if (scaledImageHeight > 0) {
                    int yPosition;
                    if (isRoot())
                        yPosition = bgLayer->backgroundYPosition().minWidthRounded(rh - scaledImageHeight);
                    else
                        yPosition = bgLayer->backgroundYPosition().minWidthRounded(ph - scaledImageHeight);
                    sy = scaledImageHeight - (yPosition % scaledImageHeight);
                    sy -= top % scaledImageHeight;
                }
            }
	    if (layer() && bgLayer->backgroundAttachment() == BGALOCAL)
		layer()->scrollOffset(sx, sy);
        }
        else
        {
            //fixed
            QRect fix = getFixedBackgroundImageRect(bgLayer, sx, sy, scaledImageWidth, scaledImageHeight);
            QRect ele(_tx, _ty, w, h);
            QRect b = fix.intersect(ele);

            //qDebug() <<" ele is " << ele << " b is " << b << " fix is " << fix;
            sx+=b.x()-fix.x();
            sy+=b.y()-fix.y();
            cx=b.x();cy=b.y();cw=b.width();ch=b.height();

	    if (canvas()->pagedMode() && scaledImageHeight > 0)
		sy = (sy - viewRect().y()) % scaledImageHeight;
        }
        // restrict painting to repaint-clip
        if (cy < clipr.y()) {
            ch -= (clipr.y() - cy);
            sy += (clipr.y() - cy);
            cy = clipr.y();
        }
        if (cx < clipr.x()) {
            cw -= (clipr.x() - cx);
            sx += (clipr.x() - cx);
            cx = clipr.x();
        }
        ch = qMin(ch, clipr.height());
        cw = qMin(cw, clipr.width());

//         qDebug() << " drawTiledPixmap(" << cx << ", " << cy << ", " << cw << ", " << ch << ", " << sx << ", " << sy << ")";
        if (cw>0 && ch>0) {
            // Note that the reason we don't simply set the path as the clip path here before calling
            // p->drawTiledPixmap() is that QX11PaintEngine doesn't support anti-aliased clipping.
            if (!path.isEmpty()) {
                QBrush brush(bg->tiled_pixmap(bgColor, scaledImageWidth, scaledImageHeight));
                brush.setTransform(QTransform(1, 0, 0, 1, cx - sx, cy - sy));
                QPainterPath cpath;
                cpath.addRect(cx, cy, cw, ch);
                p->fillPath(path.intersected(cpath), brush);
            } else
                p->drawTiledPixmap(cx, cy, cw, ch, bg->tiled_pixmap(bgColor, scaledImageWidth, scaledImageHeight), sx, sy);
        }
    }

    if (needRestore)
        p->restore(); // Undo the background clip and/or the anti-aliasing hint

}

QRect RenderBox::getFixedBackgroundImageRect( const BackgroundLayer* bgLayer, int& sx, int& sy, int& scaledImageWidth, int& scaledImageHeight )
{
    int cx, cy, cw, ch;
    QRect vr = viewRect();
    int pw = vr.width();
    int ph = vr.height();
    scaledImageWidth = pw;
    scaledImageHeight = ph;
    calculateBackgroundSize(bgLayer, scaledImageWidth, scaledImageHeight);
    EBackgroundRepeat bgr = bgLayer->backgroundRepeat();

    int xPosition = bgLayer->backgroundXPosition().minWidthRounded(pw-scaledImageWidth);
    if (bgr == NO_REPEAT || bgr == REPEAT_Y) {
        cw = qMin(scaledImageWidth, pw - xPosition);
        cx = vr.x() + xPosition;
    } else {
        cw = pw;
        cx = vr.x();
        if (scaledImageWidth > 0)
            sx = scaledImageWidth - xPosition % scaledImageWidth;
    }

    int yPosition = bgLayer->backgroundYPosition().minWidthRounded(ph-scaledImageHeight);
    if (bgr == NO_REPEAT || bgr == REPEAT_X) {
        ch = qMin(scaledImageHeight, ph - yPosition);
        cy = vr.y() + yPosition;
    } else {
        ch = ph;
        cy = vr.y();
        if (scaledImageHeight > 0)
            sy = scaledImageHeight - yPosition % scaledImageHeight;
    }
    return QRect(cx, cy, cw, ch);
}

void RenderBox::outlineBox(QPainter *p, int _tx, int _ty, const char *color)
{
    p->setPen(QPen(QColor(color), 1, Qt::DotLine));
    p->setBrush( Qt::NoBrush );
    p->drawRect(_tx, _ty, m_width, m_height);
}

QPainterPath RenderBox::borderRadiusClipPath(const BackgroundLayer *bgLayer, int _tx, int _ty, int w, int h,
                                             int bleft, int bright, int btop, int bbottom) const
{
    QPainterPath path;

    if (style()->hasBorderRadius()) {
        BorderRadii tl = style()->borderTopLeftRadius();
        BorderRadii tr = style()->borderTopRightRadius();
        BorderRadii bl = style()->borderBottomLeftRadius();
        BorderRadii br = style()->borderBottomRightRadius();

        // Adjust the border radii so they don't overlap when taking the size of the box
        // into account.
        adjustBorderRadii(tl, tr, bl, br, w, h);

        // CSS Backgrounds and Borders Module Level 3 (WD-css3-background-20080910), chapter 4.5:
        // "  Backgrounds, but not the border-image, are clipped to the inner, resp., outer
        // curve of the border if ‘background-clip’ is ‘padding-box’ resp., ‘border-box’."
        bool clipInner = bgLayer->backgroundClip() == BGPADDING;

        // The clip bounding rect
        QRect rect(_tx, _ty, w, h);
        if (clipInner) {
            rect.adjust(bleft, btop, -bright, -bbottom);

            tl.horizontal = qMax(0, tl.horizontal - bleft);
            bl.horizontal = qMax(0, bl.horizontal - bleft);
            tr.horizontal = qMax(0, tr.horizontal - bright);
            br.horizontal = qMax(0, br.horizontal - bright);
            tl.vertical = qMax(0, tl.vertical - btop);
            tr.vertical = qMax(0, tr.vertical - btop);
            bl.vertical = qMax(0, bl.vertical - bbottom);
            br.vertical = qMax(0, br.vertical - bbottom);
        }

        // Top right corner
        if (tr.hasBorderRadius()) {
            const QRect r(rect.x() + rect.width() - tr.horizontal * 2, rect.y(), tr.horizontal * 2, tr.vertical * 2);
            path.arcMoveTo(r, 0);
            path.arcTo(r, 0, 90);
        } else
            path.moveTo(rect.x() + rect.width(), rect.y());

        // Top left corner
        if (tl.hasBorderRadius()) {
            const QRect r(rect.x(), rect.y(), tl.horizontal * 2, tl.vertical * 2);
            path.arcTo(r, 90, 90);
        } else
            path.lineTo(rect.x(), rect.y());

        // Bottom left corner
        if (bl.hasBorderRadius()) {
            const QRect r(rect.x(), rect.y() + rect.height() - bl.vertical * 2, bl.horizontal * 2, bl.vertical * 2);
            path.arcTo(r, 180, 90);
        } else
            path.lineTo(rect.x(), rect.y() + rect.height());

        // Bottom right corner
        if (br.hasBorderRadius()) {
            const QRect r(rect.x() + rect.width() - br.horizontal * 2, rect.y() + rect.height() - br.vertical * 2,
                          br.horizontal * 2, br.vertical * 2);
            path.arcTo(r, 270, 90);
        } else
            path.lineTo(rect.x() + rect.width(), rect.y() + rect.height());

        path.closeSubpath();
    }

    return path;
}

QRect RenderBox::overflowClipRect(int tx, int ty)
{
    // XXX When overflow-clip (CSS3) is implemented, we'll obtain the property
    // here.
    int bl=borderLeft(),bt=borderTop(),bb=borderBottom(),br=borderRight();
    int clipx = tx+bl;
    int clipy = ty+bt;
    int clipw = m_width-bl-br;
    int cliph = m_height-bt-bb+borderTopExtra()+borderBottomExtra();

    // Subtract out scrollbars if we have them.
    if (m_layer) {
        if (m_layer->hasReversedScrollbar())
            clipx += m_layer->verticalScrollbarWidth();
        clipw -= m_layer->verticalScrollbarWidth();
        cliph -= m_layer->horizontalScrollbarHeight();
    }

    return QRect(clipx,clipy,clipw,cliph);
}

QRect RenderBox::clipRect(int tx, int ty)
{
    // Clipping applies to the entire box, including the borders, so we
    // don't have to do anything about them or margins
    int clipw = m_width;
    int cliph = m_height;

    bool rtl = (style()->direction() == RTL);

    int clipleft = 0;
    int clipright = clipw;
    int cliptop = 0;
    int clipbottom = cliph;

    if ( style()->hasClip() && style()->position() == PABSOLUTE ) {
	// the only case we use the clip property according to CSS 2.1
	if (!style()->clipLeft().isAuto()) {
	    int c = style()->clipLeft().width(clipw);
	    if ( rtl )
		clipleft = clipw - c;
	    else
		clipleft = c;
	}
	if (!style()->clipRight().isAuto()) {
	    int w = style()->clipRight().width(clipw);
	    if ( rtl ) {
		clipright = clipw - w;
	    } else {
		clipright = w;
	    }
	}
	if (!style()->clipTop().isAuto())
	    cliptop = style()->clipTop().width(cliph);
	if (!style()->clipBottom().isAuto())
	    clipbottom = style()->clipBottom().width(cliph);
    }
    int clipx = tx + clipleft;
    int clipy = ty + cliptop;
    clipw = clipright-clipleft;
    cliph = clipbottom-cliptop;

    //qDebug() << "setting clip("<<clipx<<","<<clipy<<","<<clipw<<","<<cliph<<")";

    return QRect(clipx,clipy,clipw,cliph);
}

void RenderBox::close()
{
    setNeedsLayoutAndMinMaxRecalc();
}

short RenderBox::containingBlockWidth(RenderObject* providedCB) const
{
    if (isCanvas() && canvas()->view())
    {
        if (canvas()->pagedMode())
            return canvas()->width();
        else
            return canvas()->view()->visibleWidth();
    }

    RenderObject* cb = providedCB ? providedCB : containingBlock();
    if (isRenderBlock() && cb->isTable() && static_cast<RenderTable*>(cb)->caption() == this) {
        //captions are not affected by table border or padding
        return cb->width();
    }
    if (isPositioned()) {
        // cf. 10.1.4 - use padding edge
        if (cb->isInlineFlow()) {
            // 10.1.4.1
            int l, r;
            InlineFlowBox* firstLineBox = static_cast<const RenderFlow*>(cb)->firstLineBox();
            InlineFlowBox* lastLineBox = static_cast<const RenderFlow*>(cb)->lastLineBox();
            if (!lastLineBox)
                return 0;
            if (cb->style()->direction() == RTL) {
                l = lastLineBox->xPos() + lastLineBox->borderLeft();
                r = firstLineBox->xPos() + firstLineBox->width() - firstLineBox->borderRight();
            } else {
                l = firstLineBox->xPos() + firstLineBox->borderLeft();
                r = lastLineBox->xPos() + lastLineBox->width() - lastLineBox->borderRight();
            }
            return qMax(0, r-l);
        }
        // 10.1.4.2
        return cb->contentWidth() + cb->paddingLeft() + cb->paddingRight();
    }
    else if (usesLineWidth()) {
        assert( cb->isRenderBlock() );
        return static_cast<RenderBlock*>(cb)->lineWidth(m_y);
    } else
        return cb->contentWidth();
}

bool RenderBox::absolutePosition(int &_xPos, int &_yPos, bool f) const
{
    if ( style()->position() == PFIXED )
	f = true;
    RenderObject *o = container();
    if( o && o->absolutePosition(_xPos, _yPos, f))
    {
        if ( o->layer() ) {
            if (o->hasOverflowClip())
                o->layer()->subtractScrollOffset( _xPos, _yPos );
            if (isPositioned())
                o->layer()->checkInlineRelOffset(this, _xPos, _yPos);
        }

        if(!isInline() || isReplaced()) {
            _xPos += xPos(),
            _yPos += yPos();
        }

        if(isRelPositioned())
            relativePositionOffset(_xPos, _yPos);
        return true;
    }
    else
    {
        _xPos = 0;
        _yPos = 0;
        return false;
    }
}

void RenderBox::position(InlineBox* box, int /*from*/, int /*len*/, bool /*reverse*/)
{
    if (isPositioned()) {
        // Cache the x position only if we were an INLINE type originally.
        bool wasInline = style()->isOriginalDisplayInlineType();

        if (wasInline && hasStaticX()) {
            // The value is cached in the xPos of the box.  We only need this value if
            // our object was inline originally, since otherwise it would have ended up underneath
            // the inlines.
            m_staticX = box->xPos();
        }
        else if (!wasInline && hasStaticY()) {
            // Our object was a block originally, so we make our normal flow position be
            // just below the line box (as though all the inlines that came before us got
            // wrapped in an anonymous block, which is what would have happened had we been
            // in flow).  This value was cached in the yPos() of the box.
            m_staticY = box->yPos();
        }
    }
    else if (isReplaced())
        setPos( box->xPos(), box->yPos() );
}

void RenderBox::repaint(Priority prior)
{
    int ow = style() ? style()->outlineSize() : 0;
    if( isInline() && !isReplaced() )
    {
        RenderObject* p = parent();
        Q_ASSERT(p);
        while( p->isInline() && !p->isReplaced() )
            p = p->parent();
        int xoff = p->hasOverflowClip() ? 0 : p->overflowLeft();
        int yoff = p->hasOverflowClip() ? 0 : p->overflowTop();
        p->repaintRectangle( -ow + xoff, -ow + yoff, p->effectiveWidth()+ow*2, p->effectiveHeight()+ow*2, prior);
    }
    else
    {
        int xoff = hasOverflowClip() ? 0 : overflowLeft();
        int yoff = hasOverflowClip() ? 0 : overflowTop();
        repaintRectangle( -ow + xoff, -ow + yoff, effectiveWidth()+ow*2, effectiveHeight()+ow*2, prior);
    }
}

void RenderBox::repaintRectangle(int x, int y, int w, int h, Priority p, bool f)
{
    x += m_x;
    y += m_y;

    // Apply the relative position offset when invalidating a rectangle.  The layer
    // is translated, but the render box isn't, so we need to do this to get the
    // right dirty rect.  Since this is called from RenderObject::setStyle, the relative position
    // flag on the RenderObject has been cleared, so use the one on the style().
    if (style()->position() == PRELATIVE && m_layer)
        relativePositionOffset(x,y);

    if (style()->position() == PFIXED) f=true;

    // qDebug() << "RenderBox(" <<this << ", " << renderName() << ")::repaintRectangle (" << x << "/" << y << ") (" << w << "/" << h << ")";
    RenderObject *o = container();
    if( o ) {
         if (o->layer()) {
             if (o->style()->hidesOverflow() && o->layer() && !o->isInlineFlow())
                 o->layer()->subtractScrollOffset(x,y); // For overflow:auto/scroll/hidden.
             if (style()->position() == PABSOLUTE)
                 o->layer()->checkInlineRelOffset(this,x,y);
        }
        o->repaintRectangle(x, y, w, h, p, f);
    }
}

void RenderBox::relativePositionOffset(int &tx, int &ty) const
{
    if (!style()->left().isAuto()) {
        if (!style()->right().isAuto() && containingBlock()->style()->direction() == RTL)
            tx -= style()->right().width(containingBlockWidth());
        else
            tx +=  style()->left().width(containingBlockWidth());
    } else if (!style()->right().isAuto())
        tx -= style()->right().width(containingBlockWidth());
    if(!style()->top().isAuto()) {
        if (style()->top().isPercent()) {
            float p = style()->top().percent();
            bool neg = p < 0.0;
            int ph = calcPercentageHeight(Length((neg?-p:p), Percent));
            if  (ph != -1)
                ty += neg?-ph:ph;
        } else {
            ty += style()->top().width(containingBlockHeight());
        }
    }
    else if(!style()->bottom().isAuto()) {
        if (style()->bottom().isPercent()) {
            float p = style()->bottom().percent();
            bool neg = p < 0.0;
            int ph = calcPercentageHeight(Length((neg?-p:p), Percent));
            if (ph != -1)
                ty -= neg?-ph:ph;
        } else {
            ty -= style()->bottom().width(containingBlockHeight());
        }
    }
}

void RenderBox::calcWidth()
{
#ifdef DEBUG_LAYOUT
    qDebug() << "RenderBox("<<renderName()<<")::calcWidth()";
#endif
    if (isPositioned())
    {
        calcAbsoluteHorizontal();
    }
    else
    {
        bool treatAsReplaced = isReplaced() && !isInlineBlockOrInlineTable();
        Length w;
        if (treatAsReplaced)
            w = Length( calcReplacedWidth(), Fixed );
        else
            w = style()->width();

        Length ml = style()->marginLeft();
        Length mr = style()->marginRight();

	int cw = containingBlockWidth();
        if (cw<0) cw = 0;

        m_marginLeft = 0;
        m_marginRight = 0;

        if (isInline() && !isInlineBlockOrInlineTable())
        {
            // just calculate margins
            m_marginLeft = ml.minWidth(cw);
            m_marginRight = mr.minWidth(cw);
            if (treatAsReplaced)
            {
                m_width = w.width(cw) + borderLeft() + borderRight() + paddingLeft() + paddingRight();
                m_width = qMax(m_width, m_minWidth);
            }

            return;
        }
        else
        {
            LengthType widthType, minWidthType, maxWidthType;
            if (treatAsReplaced) {
                m_width = w.width(cw) + borderLeft() + borderRight() + paddingLeft() + paddingRight();
                widthType = w.type();
            } else {
                m_width = calcWidthUsing(Width, cw, widthType);
                int minW = calcWidthUsing(MinWidth, cw, minWidthType);
                int maxW = style()->maxWidth().isUndefined() ?
                             m_width : calcWidthUsing(MaxWidth, cw, maxWidthType);

                if (m_width > maxW) {
                    m_width = maxW;
                    widthType = maxWidthType;
                }
                if (m_width < minW) {
                    m_width = minW;
                    widthType = minWidthType;
                }
                if (short iw = intrinsicWidth()) {
                    // some elements (e.g. Fieldset) have pseudo-replaced behaviour in quirk mode
                    if (m_width < iw) {
                        m_width = iw;
                        widthType = Fixed;
                    }
                }
            }

            if (widthType == Auto) {
    //          qDebug() << "variable";
                m_marginLeft = ml.minWidth(cw);
                m_marginRight = mr.minWidth(cw);
            }
            else
            {
//              qDebug() << "non-variable " << w.type << ","<< w.value;
                calcHorizontalMargins(ml,mr,cw);
            }
        }

        if (cw && cw != m_width + m_marginLeft + m_marginRight && !isFloating() && !isInline())
        {
            if (containingBlock()->style()->direction()==LTR)
                m_marginRight = cw - m_width - m_marginLeft;
            else
                m_marginLeft = cw - m_width - m_marginRight;
        }
    }

#ifdef DEBUG_LAYOUT
    qDebug() << "RenderBox::calcWidth(): m_width=" << m_width << " containingBlockWidth()=" << containingBlockWidth();
    qDebug() << "m_marginLeft=" << m_marginLeft << " m_marginRight=" << m_marginRight;
#endif
}

int RenderBox::calcWidthUsing(WidthType widthType, int cw, LengthType& lengthType)
{
    int width = m_width;
    Length w;
    if (widthType == Width)
        w = style()->width();
    else if (widthType == MinWidth)
        w = style()->minWidth();
    else
        w = style()->maxWidth();

    lengthType = w.type();

    if (lengthType == Auto) {
        int marginLeft = style()->marginLeft().minWidth(cw);
        int marginRight = style()->marginRight().minWidth(cw);
        if (cw) width = cw - marginLeft - marginRight;

        // size to max width?
        if (sizesToMaxWidth()) {
            width = qMax(width, (int)m_minWidth);
            width = qMin(width, (int)m_maxWidth);
        }
    }
    else
    {
        width = calcBoxWidth(w.width(cw));
    }

    return width;
}

void RenderBox::calcHorizontalMargins(const Length& ml, const Length& mr, int cw)
{
    if (isFloating() || isInline()) // Inline blocks/tables and floats don't have their margins increased.
    {
        m_marginLeft = ml.minWidth(cw);
        m_marginRight = mr.minWidth(cw);
    }
    else
    {
        if ( (ml.isAuto() && mr.isAuto() && m_width<cw) ||
             (!ml.isAuto() && !mr.isAuto() &&
                containingBlock()->style()->textAlign() == KHTML_CENTER) )
        {
            m_marginLeft = (cw - m_width)/2;
            if (m_marginLeft<0) m_marginLeft=0;
            m_marginRight = cw - m_width - m_marginLeft;
        }
        else if ( (mr.isAuto() && m_width<cw) ||
                 (!ml.isAuto() && containingBlock()->style()->direction() == RTL &&
                  containingBlock()->style()->textAlign() == KHTML_LEFT))
        {
            m_marginLeft = ml.width(cw);
            m_marginRight = cw - m_width - m_marginLeft;
        }
        else if ( (ml.isAuto() && m_width<cw) ||
                 (!mr.isAuto() && containingBlock()->style()->direction() == LTR &&
                  containingBlock()->style()->textAlign() == KHTML_RIGHT))
        {
            m_marginRight = mr.width(cw);
            m_marginLeft = cw - m_width - m_marginRight;
        }
        else
        {
           // this makes auto margins 0 if we failed a m_width<cw test above (css2.1, 10.3.3)
            m_marginLeft = ml.minWidth(cw);
            m_marginRight = mr.minWidth(cw);
        }
    }
}

void RenderBox::calcHeight()
{

#ifdef DEBUG_LAYOUT
    qDebug() << "RenderBox::calcHeight()";
#endif

    //cell height is managed by table, inline elements do not have a height property.
    if ( isTableCell() || (isInline() && !isReplaced()) )
        return;

    if (isPositioned())
        calcAbsoluteVertical();
    else
    {
        calcVerticalMargins();

        // For tables, calculate margins only
        if (isTable())
            return;

        Length h;
        bool treatAsReplaced = isReplaced() && !isInlineBlockOrInlineTable();
        bool checkMinMaxHeight = false;

        if ( treatAsReplaced )
            h = Length( calcReplacedHeight(), Fixed );
        else {
            h = style()->height();
            checkMinMaxHeight = true;
        }

        int height;
        if (checkMinMaxHeight) {
            height = calcHeightUsing(style()->height());
            if (height == -1)
                height = m_height;
            int minH = calcHeightUsing(style()->minHeight()); // Leave as -1 if unset.
            int maxH = style()->maxHeight().isUndefined() ? height : calcHeightUsing(style()->maxHeight());
            if (maxH == -1)
                maxH = height;
            height = qMin(maxH, height);
            height = qMax(minH, height);
        }
        else {
            // The only times we don't check min/max height are when a fixed length has
            // been given as an override.  Just use that.
            height = h.value() + borderTop() + borderBottom() + paddingTop() + paddingBottom();
        }

        m_height = height;
    }

    // Unfurling marquees override with the furled height.
    if (style()->overflowX() == OMARQUEE && m_layer && m_layer->marquee() &&
        m_layer->marquee()->isUnfurlMarquee() && !m_layer->marquee()->isHorizontal()) {
        m_layer->marquee()->setEnd(m_height);
        m_height = qMin(m_height, m_layer->marquee()->unfurlPos());
    }

}

int RenderBox::calcHeightUsing(const Length& h)
{
    int height = -1;
    if (!h.isAuto()) {
        if (h.isFixed())
            height = h.value();
        else if (h.isPercent())
            height = calcPercentageHeight(h);
        if (height != -1) {
            height = calcBoxHeight(height);
            return height;
        }
    }
    return height;
}

int RenderBox::calcImplicitContentHeight() const {
    assert(hasImplicitHeight());

    RenderBlock* cb = containingBlock();
    // padding-box height
    int ch = cb->height() - cb->borderTop() - cb->borderBottom();
    int top = style()->top().width(ch);
    int bottom = style()->bottom().width(ch);

    return ch - top - bottom - borderTop() - borderBottom() - paddingTop() - paddingBottom();
}

int RenderBox::calcPercentageHeight(const Length& height) const
{
    int result = -1;
    RenderBlock* cb = containingBlock();
    // In quirk mode, table cells violate what the CSS spec says to do with heights.
    if (cb->isTableCell() && style()->htmlHacks()) {
        result = static_cast<RenderTableCell*>(cb)->cellPercentageHeight();
    }

    // Otherwise we only use our percentage height if our containing block had a specified
    // height.
    else if (cb->style()->height().isFixed())
        result = cb->calcContentHeight(cb->style()->height().value());
    else if (cb->style()->height().isPercent()) {
        // We need to recur and compute the percentage height for our containing block.
        result = cb->calcPercentageHeight(cb->style()->height());
        if (result != -1)
            result = cb->calcContentHeight(result);
    }
    else if (cb->isCanvas()) {
        if (!canvas()->pagedMode())
            result = static_cast<RenderCanvas*>(cb)->viewportHeight();
        else
            result = static_cast<RenderCanvas*>(cb)->height();
        result -= cb->style()->borderTopWidth() - cb->style()->borderBottomWidth();
        result -= cb->paddingTop() + cb->paddingBottom();
    }
    else if (cb->isBody() && style()->htmlHacks() &&
                             cb->style()->height().isAuto() && !cb->isFloatingOrPositioned()) {
        int margins = cb->collapsedMarginTop() + cb->collapsedMarginBottom();
        int visHeight = canvas()->viewportHeight();
        RenderObject* p = cb->parent();
        result = visHeight - (margins + p->marginTop() + p->marginBottom() +
                              p->borderTop() + p->borderBottom() +
                              p->paddingTop() + p->paddingBottom());
    }
    else if (cb->isRoot() && style()->htmlHacks() && cb->style()->height().isAuto()) {
        int visHeight = canvas()->viewportHeight();
        result = visHeight - (marginTop() + marginBottom() +
                              borderTop() + borderBottom() +
                              paddingTop() + paddingBottom());
    }
    else if (isPositioned()) {
        // "10.5 - Note that the height of the containing block of an absolutely positioned element is independent
        //  of the size of the element itself, and thus a percentage height on such an element can always be resolved."
        //
        // take the used height - at the padding edge since we are positioned (10.1)
        result = cb->height() - cb->borderTop() - cb->borderBottom();
    }
    else if (cb->hasImplicitHeight()) {
        result = cb->calcImplicitContentHeight();
    }
    else if (cb->isAnonymousBlock() || style()->htmlHacks()) {
        // IE quirk.
        assert( cb->style()->height().isAuto() );
        result = cb->calcPercentageHeight(cb->style()->height());
        if (result != -1)
            result = cb->calcContentHeight(result);
    }

    if (result != -1) {
        result = height.width(result);
        if (cb->isTableCell() && style()->boxSizing() != BORDER_BOX) {
            result -= (borderTop() + paddingTop() + borderBottom() + paddingBottom());
            result = qMax(0, result);
        }
    }
    return result;
}

short RenderBox::calcReplacedWidth() const
{
    int width = calcReplacedWidthUsing(Width);
    int minW = calcReplacedWidthUsing(MinWidth);
    int maxW = style()->maxWidth().isUndefined() ? width : calcReplacedWidthUsing(MaxWidth);

    if (width > maxW)
        width = maxW;

    if (width < minW)
        width = minW;

    return width;
}

int RenderBox::calcReplacedWidthUsing(WidthType widthType) const
{
    Length w;
    if (widthType == Width)
        w = style()->width();
    else if (widthType == MinWidth)
        w = style()->minWidth();
    else
        w = style()->maxWidth();

    switch (w.type()) {
    case Fixed:
        return calcContentWidth(w.value());
    case Percent:
    {
        const int cw = containingBlockWidth();
        if (cw > 0) {
            int result = calcContentWidth(w.minWidth(cw));
            return result;
        }
    }
    // fall through
    default:
        return intrinsicWidth();
    }
}

int RenderBox::calcReplacedHeight() const
{
    int height = calcReplacedHeightUsing(Height);
    int minH = calcReplacedHeightUsing(MinHeight);
    int maxH = style()->maxHeight().isUndefined() ? height : calcReplacedHeightUsing(MaxHeight);

    if (height > maxH)
        height = maxH;

    if (height < minH)
        height = minH;

    return height;
}

int RenderBox::calcReplacedHeightUsing(HeightType heightType) const
{
    Length h;
    if (heightType == Height)
        h = style()->height();
    else if (heightType == MinHeight)
        h = style()->minHeight();
    else
        h = style()->maxHeight();
    switch( h.type() ) {
    case Fixed:
        return calcContentHeight(h.value());
    case Percent:
      {
        int th = calcPercentageHeight(h);
        if (th != -1)
            return calcContentHeight(th);
        // fall through
      }
    default:
        return intrinsicHeight();
    };
}

int RenderBox::availableHeight() const
{
    return availableHeightUsing(style()->height());
}

int RenderBox::availableHeightUsing(const Length& h) const
{
    if (h.isFixed())
        return calcContentHeight(h.value());

    if (isCanvas()) {
        if (static_cast<const RenderCanvas*>(this)->pagedMode()) {
            return static_cast<const RenderCanvas*>(this)->pageHeight();
        } else {
            return static_cast<const RenderCanvas*>(this)->viewportHeight();
        }
    }

    // We need to stop here, since we don't want to increase the height of the table
    // artificially.  We're going to rely on this cell getting expanded to some new
    // height, and then when we lay out again we'll use the calculation below.
    if (isTableCell() && (h.isAuto() || h.isPercent())) {
        const RenderTableCell* tableCell = static_cast<const RenderTableCell*>(this);
        return tableCell->cellPercentageHeight() -
	    (borderTop()+borderBottom()+paddingTop()+paddingBottom());
    }

    if (h.isPercent())
       return calcContentHeight(h.width(containingBlock()->availableHeight()));

    // Check for implicit height
    if (hasImplicitHeight())
        return calcImplicitContentHeight();

    return containingBlock()->availableHeight();
}

int RenderBox::availableWidth() const
{
    return availableWidthUsing(style()->width());
}

int RenderBox::availableWidthUsing(const Length& w) const
{
    if (w.isFixed())
        return calcContentWidth(w.value());

    if (isCanvas())
        return static_cast<const RenderCanvas*>(this)->viewportWidth();

    if (w.isPercent())
       return calcContentWidth(w.width(containingBlock()->availableWidth()));

    return containingBlock()->availableWidth();
}

void RenderBox::calcVerticalMargins()
{
    if( isTableCell() ) {
	// table margins are basically infinite
	m_marginTop = TABLECELLMARGIN;
	m_marginBottom = TABLECELLMARGIN;
	return;
    }

    Length tm = style()->marginTop();
    Length bm = style()->marginBottom();

    // margins are calculated with respect to the _width_ of
    // the containing block (8.3)
    int cw = containingBlock()->contentWidth();

    m_marginTop = tm.minWidth(cw);
    m_marginBottom = bm.minWidth(cw);
}

void RenderBox::setStaticX(short staticX)
{
    m_staticX = staticX;
}

void RenderBox::setStaticY(int staticY)
{
    m_staticY = staticY;
}


void RenderBox::calcAbsoluteHorizontal()
{
   if (isReplaced()) {
        calcAbsoluteHorizontalReplaced();
        return;
    }

    // QUESTIONS
    // FIXME 1: Which RenderObject's 'direction' property should used: the
    // containing block (cb) as the spec seems to imply, the parent (parent()) as
    // was previously done in calculating the static distances, or ourself, which
    // was also previously done for deciding what to override when you had
    // over-constrained margins?  Also note that the container block is used
    // in similar situations in other parts of the RenderBox class (see calcWidth()
    // and calcHorizontalMargins()). For now we are using the parent for quirks
    // mode and the containing block for strict mode.

    // FIXME 2: Can perhaps optimize out cases when max-width/min-width are greater
    // than or less than the computed m_width.  Be careful of box-sizing and
    // percentage issues.

    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.3.7 "Absolutely positioned, non-replaced elements"
    // <http://www.w3.org/TR/CSS21/visudet.html#abs-non-replaced-width>
    // (block-style-comments in this function and in calcAbsoluteHorizontalValues()
    // correspond to text from the spec)


    // We don't use containingBlock(), since we may be positioned by an enclosing
    // relative positioned inline.
    RenderObject* containerBlock = container();

    const int containerWidth = containingBlockWidth(containerBlock);

    // To match WinIE, in quirks mode use the parent's 'direction' property
    // instead of the container block's.
    EDirection containerDirection = (style()->htmlHacks()) ? parent()->style()->direction() : containerBlock->style()->direction();

    const int bordersPlusPadding = borderLeft() + borderRight() + paddingLeft() + paddingRight();
    const Length marginLeft = style()->marginLeft();
    const Length marginRight = style()->marginRight();
    Length left = style()->left();
    Length right = style()->right();

    /*---------------------------------------------------------------------------*\
     * For the purposes of this section and the next, the term "static position"
     * (of an element) refers, roughly, to the position an element would have had
     * in the normal flow. More precisely:
     *
     * * The static position for 'left' is the distance from the left edge of the
     *   containing block to the left margin edge of a hypothetical box that would
     *   have been the first box of the element if its 'position' property had
     *   been 'static' and 'float' had been 'none'. The value is negative if the
     *   hypothetical box is to the left of the containing block.
     * * The static position for 'right' is the distance from the right edge of the
     *   containing block to the right margin edge of the same hypothetical box as
     *   above. The value is positive if the hypothetical box is to the left of the
     *   containing block's edge.
     *
     * But rather than actually calculating the dimensions of that hypothetical box,
     * user agents are free to make a guess at its probable position.
     *
     * For the purposes of calculating the static position, the containing block of
     * fixed positioned elements is the initial containing block instead of the
     * viewport, and all scrollable boxes should be assumed to be scrolled to their
     * origin.
    \*---------------------------------------------------------------------------*/

    // Calculate the static distance if needed.
    if (left.isAuto() && right.isAuto()) {
        if (containerDirection == LTR) {
            // 'm_staticX' should already have been set through layout of the parent.
            int staticPosition = m_staticX - containerBlock->borderLeft();
            for (RenderObject* po = parent(); po && po != containerBlock; po = po->parent())
                staticPosition += po->xPos();
            left = Length(staticPosition, Fixed);
        } else {
            RenderObject* po = parent();
            // 'm_staticX' should already have been set through layout of the parent.
            int staticPosition = m_staticX + containerWidth + containerBlock->borderRight() - po->width();
            for (; po && po != containerBlock; po = po->parent())
                staticPosition -= po->xPos();
            right = Length(staticPosition, Fixed);
        }
    }

    // Calculate constraint equation values for 'width' case.
    calcAbsoluteHorizontalValues(style()->width(), containerBlock, containerDirection,
                                 containerWidth, bordersPlusPadding,
                                 left, right, marginLeft, marginRight,
                                 m_width, m_marginLeft, m_marginRight, m_x);
    // Calculate constraint equation values for 'max-width' case.calcContentWidth(width.width(containerWidth));
    if (!style()->maxWidth().isUndefined()) {
        short maxWidth;
        short maxMarginLeft;
        short maxMarginRight;
        short maxXPos;

        calcAbsoluteHorizontalValues(style()->maxWidth(), containerBlock, containerDirection,
                                     containerWidth, bordersPlusPadding,
                                     left, right, marginLeft, marginRight,
                                     maxWidth, maxMarginLeft, maxMarginRight, maxXPos);

        if (m_width > maxWidth) {
            m_width = maxWidth;
            m_marginLeft = maxMarginLeft;
            m_marginRight = maxMarginRight;
            m_x = maxXPos;
        }
    }

    // Calculate constraint equation values for 'min-width' case.
    if (!style()->minWidth().isZero()) {
        short minWidth;
        short minMarginLeft;
        short minMarginRight;
        short minXPos;

        calcAbsoluteHorizontalValues(style()->minWidth(), containerBlock, containerDirection,
                                     containerWidth, bordersPlusPadding,
                                     left, right, marginLeft, marginRight,
                                     minWidth, minMarginLeft, minMarginRight, minXPos);

        if (m_width < minWidth) {
            m_width = minWidth;
            m_marginLeft = minMarginLeft;
            m_marginRight = minMarginRight;
            m_x = minXPos;
        }
    }

    if (short iw = intrinsicWidth()) {
        // some elements (e.g. Fieldset) have pseudo-replaced behaviour in quirk mode
        if (m_width < iw - bordersPlusPadding)
            calcAbsoluteHorizontalValues(Length(iw - bordersPlusPadding, Fixed), containerBlock, containerDirection,
                                     containerWidth, bordersPlusPadding,
                                     left, right, marginLeft, marginRight,
                                     m_width, m_marginLeft, m_marginRight, m_x);
    }

    // Put m_width into correct form.
    m_width += bordersPlusPadding;
}

void RenderBox::calcAbsoluteHorizontalValues(Length width, const RenderObject* containerBlock, EDirection containerDirection,
                                             const int containerWidth, const int bordersPlusPadding,
                                             const Length left, const Length right, const Length marginLeft, const Length marginRight,
                                             short& widthValue, short& marginLeftValue, short& marginRightValue, short& xPos)
{
    // 'left' and 'right' cannot both be 'auto' because one would of been
    // converted to the static postion already
    assert(!(left.isAuto() && right.isAuto()));

    int leftValue = 0;

    const bool widthIsAuto = width.isAuto();
    const bool leftIsAuto = left.isAuto();
    const bool rightIsAuto = right.isAuto();

    if (!leftIsAuto && !widthIsAuto && !rightIsAuto) {
        /*-----------------------------------------------------------------------*\
         * If none of the three is 'auto': If both 'margin-left' and 'margin-
         * right' are 'auto', solve the equation under the extra constraint that
         * the two margins get equal values, unless this would make them negative,
         * in which case when direction of the containing block is 'ltr' ('rtl'),
         * set 'margin-left' ('margin-right') to zero and solve for 'margin-right'
         * ('margin-left'). If one of 'margin-left' or 'margin-right' is 'auto',
         * solve the equation for that value. If the values are over-constrained,
         * ignore the value for 'left' (in case the 'direction' property of the
         * containing block is 'rtl') or 'right' (in case 'direction' is 'ltr')
         * and solve for that value.
        \*-----------------------------------------------------------------------*/
        // NOTE:  It is not necessary to solve for 'right' in the over constrained
        // case because the value is not used for any further calculations.

        leftValue = left.width(containerWidth);
        widthValue = calcContentWidth(width.width(containerWidth));

        const int availableSpace = containerWidth - (leftValue + widthValue + right.width(containerWidth) + bordersPlusPadding);

        // Margins are now the only unknown
        if (marginLeft.isAuto() && marginRight.isAuto()) {
            // Both margins auto, solve for equality
            if (availableSpace >= 0) {
                marginLeftValue = availableSpace / 2; // split the diference
                marginRightValue = availableSpace - marginLeftValue;  // account for odd valued differences
            } else {
                // see FIXME 1
                if (containerDirection == LTR) {
                    marginLeftValue = 0;
                    marginRightValue = availableSpace; // will be negative
                } else {
                    marginLeftValue = availableSpace; // will be negative
                    marginRightValue = 0;
                }
            }
        } else if (marginLeft.isAuto()) {
            // Solve for left margin
            marginRightValue = marginRight.width(containerWidth);
            marginLeftValue = availableSpace - marginRightValue;
        } else if (marginRight.isAuto()) {
            // Solve for right margin
            marginLeftValue = marginLeft.width(containerWidth);
            marginRightValue = availableSpace - marginLeftValue;
        } else {
            // Over-constrained, solve for left if direction is RTL
            marginLeftValue = marginLeft.width(containerWidth);
            marginRightValue = marginRight.width(containerWidth);

            // see FIXME 1 -- used to be "this->style()->direction()"
            if (containerDirection == RTL)
                leftValue = (availableSpace + leftValue) - marginLeftValue - marginRightValue;
        }
    } else {
        /*--------------------------------------------------------------------*\
         * Otherwise, set 'auto' values for 'margin-left' and 'margin-right'
         * to 0, and pick the one of the following six rules that applies.
         *
         * 1. 'left' and 'width' are 'auto' and 'right' is not 'auto', then the
         *    width is shrink-to-fit. Then solve for 'left'
         *
         *              OMIT RULE 2 AS IT SHOULD NEVER BE HIT
         * ------------------------------------------------------------------
         * 2. 'left' and 'right' are 'auto' and 'width' is not 'auto', then if
         *    the 'direction' property of the containing block is 'ltr' set
         *    'left' to the static position, otherwise set 'right' to the
         *    static position. Then solve for 'left' (if 'direction is 'rtl')
         *    or 'right' (if 'direction' is 'ltr').
         * ------------------------------------------------------------------
         *
         * 3. 'width' and 'right' are 'auto' and 'left' is not 'auto', then the
         *    width is shrink-to-fit . Then solve for 'right'
         * 4. 'left' is 'auto', 'width' and 'right' are not 'auto', then solve
         *    for 'left'
         * 5. 'width' is 'auto', 'left' and 'right' are not 'auto', then solve
         *    for 'width'
         * 6. 'right' is 'auto', 'left' and 'width' are not 'auto', then solve
         *    for 'right'
         *
         * Calculation of the shrink-to-fit width is similar to calculating the
         * width of a table cell using the automatic table layout algorithm.
         * Roughly: calculate the preferred width by formatting the content
         * without breaking lines other than where explicit line breaks occur,
         * and also calculate the preferred minimum width, e.g., by trying all
         * possible line breaks. CSS 2.1 does not define the exact algorithm.
         * Thirdly, calculate the available width: this is found by solving
         * for 'width' after setting 'left' (in case 1) or 'right' (in case 3)
         * to 0.
         *
         * Then the shrink-to-fit width is:
         * min(max(preferred minimum width, available width), preferred width).
        \*--------------------------------------------------------------------*/
        // NOTE: For rules 3 and 6 it is not necessary to solve for 'right'
        // because the value is not used for any further calculations.

        // Calculate margins, 'auto' margins are ignored.
        marginLeftValue = marginLeft.minWidth(containerWidth);
        marginRightValue = marginRight.minWidth(containerWidth);

        const int availableSpace = containerWidth - (marginLeftValue + marginRightValue + bordersPlusPadding);

        // FIXME: Is there a faster way to find the correct case?
        // Use rule/case that applies.
        if (leftIsAuto && widthIsAuto && !rightIsAuto) {
            // RULE 1: (use shrink-to-fit for width, and solve of left)
            int rightValue = right.width(containerWidth);

            // FIXME: would it be better to have shrink-to-fit in one step?
            int preferredWidth = m_maxWidth - bordersPlusPadding;
            int preferredMinWidth = m_minWidth - bordersPlusPadding;
            int availableWidth = availableSpace - rightValue;
            widthValue = qMin(qMax(preferredMinWidth, availableWidth), preferredWidth);
            leftValue = availableSpace - (widthValue + rightValue);
        } else if (!leftIsAuto && widthIsAuto && rightIsAuto) {
            // RULE 3: (use shrink-to-fit for width, and no need solve of right)
            leftValue = left.width(containerWidth);

            // FIXME: would it be better to have shrink-to-fit in one step?
            int preferredWidth = m_maxWidth - bordersPlusPadding;
            int preferredMinWidth = m_minWidth - bordersPlusPadding;
            int availableWidth = availableSpace - leftValue;
            widthValue = qMin(qMax(preferredMinWidth, availableWidth), preferredWidth);
        } else if (leftIsAuto && !width.isAuto() && !rightIsAuto) {
            // RULE 4: (solve for left)
            widthValue = calcContentWidth(width.width(containerWidth));
            leftValue = availableSpace - (widthValue + right.width(containerWidth));
        } else if (!leftIsAuto && widthIsAuto && !rightIsAuto) {
            // RULE 5: (solve for width)
            leftValue = left.width(containerWidth);
            widthValue = availableSpace - (leftValue + right.width(containerWidth));
        } else if (!leftIsAuto&& !widthIsAuto && rightIsAuto) {
            // RULE 6: (no need solve for right)
            leftValue = left.width(containerWidth);
            widthValue = calcContentWidth(width.width(containerWidth));
        }
    }

    // Use computed values to calculate the horizontal position.
    int calculatedHorizontalPosition = leftValue + marginLeftValue + containerBlock->borderLeft();
    xPos = qBound((int)SHRT_MIN, calculatedHorizontalPosition, (int)SHRT_MAX);
}

void RenderBox::calcAbsoluteVertical()
{
    if (isReplaced()) {
        calcAbsoluteVerticalReplaced();
        return;
    }

    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.6.4 "Absolutely positioned, non-replaced elements"
    // <http://www.w3.org/TR/2005/WD-CSS21-20050613/visudet.html#abs-non-replaced-height>
    // (block-style-comments in this function and in calcAbsoluteVerticalValues()
    // correspond to text from the spec)


    // We don't use containingBlock(), since we may be positioned by an enclosing relpositioned inline.
    const RenderObject* containerBlock = container();
    const int containerHeight = containerBlock->height() - containerBlock->borderTop() - containerBlock->borderBottom();

    const int bordersPlusPadding = borderTop() + borderBottom() + paddingTop() + paddingBottom();
    const Length marginTop = style()->marginTop();
    const Length marginBottom = style()->marginBottom();
    Length top = style()->top();
    Length bottom = style()->bottom();

    /*---------------------------------------------------------------------------*\
     * For the purposes of this section and the next, the term "static position"
     * (of an element) refers, roughly, to the position an element would have had
     * in the normal flow. More precisely, the static position for 'top' is the
     * distance from the top edge of the containing block to the top margin edge
     * of a hypothetical box that would have been the first box of the element if
     * its 'position' property had been 'static' and 'float' had been 'none'. The
     * value is negative if the hypothetical box is above the containing block.
     *
     * But rather than actually calculating the dimensions of that hypothetical
     * box, user agents are free to make a guess at its probable position.
     *
     * For the purposes of calculating the static position, the containing block
     * of fixed positioned elements is the initial containing block instead of
     * the viewport.
    \*---------------------------------------------------------------------------*/

    // Calculate the static distance if needed.
    if (top.isAuto() && bottom.isAuto()) {
        // m_staticY should already have been set through layout of the parent()
        int staticTop = m_staticY - containerBlock->borderTop();
        for (RenderObject* po = parent(); po && po != containerBlock; po = po->parent()) {
            staticTop += po->yPos();
        }
        top.setValue(Fixed, staticTop);
    }


    int height; // Needed to compute overflow.

    // Calculate constraint equation values for 'height' case.
    calcAbsoluteVerticalValues(style()->height(), containerBlock, containerHeight, bordersPlusPadding,
                               top, bottom, marginTop, marginBottom,
                               height, m_marginTop, m_marginBottom, m_y);

    // Avoid doing any work in the common case (where the values of min-height and max-height are their defaults).
    // see FIXME 2

    // Calculate constraint equation values for 'max-height' case.
    if (!style()->maxHeight().isUndefined()) {
        int maxHeight;
        short maxMarginTop;
        short maxMarginBottom;
        int maxYPos;

        calcAbsoluteVerticalValues(style()->maxHeight(), containerBlock, containerHeight, bordersPlusPadding,
                                   top, bottom, marginTop, marginBottom,
                                   maxHeight, maxMarginTop, maxMarginBottom, maxYPos);

        if (height > maxHeight) {
            height = maxHeight;
            m_marginTop = maxMarginTop;
            m_marginBottom = maxMarginBottom;
            m_y = maxYPos;
        }
    }

    // Calculate constraint equation values for 'min-height' case.
    if (!style()->minHeight().isZero()) {
        int minHeight;
        short minMarginTop;
        short minMarginBottom;
        int minYPos;

        calcAbsoluteVerticalValues(style()->minHeight(), containerBlock, containerHeight, bordersPlusPadding,
                                   top, bottom, marginTop, marginBottom,
                                   minHeight, minMarginTop, minMarginBottom, minYPos);

        if (height < minHeight) {
            height = minHeight;
            m_marginTop = minMarginTop;
            m_marginBottom = minMarginBottom;
            m_y = minYPos;
        }
    }

    height += bordersPlusPadding;

    // Set final height value.
    m_height = height;
}

void RenderBox::calcAbsoluteVerticalValues(Length height, const RenderObject* containerBlock,
                                           const int containerHeight, const int bordersPlusPadding,
                                           const Length top, const Length bottom, const Length marginTop, const Length marginBottom,
                                           int& heightValue, short& marginTopValue, short& marginBottomValue, int& yPos)
{
    // 'top' and 'bottom' cannot both be 'auto' because 'top would of been
    // converted to the static position in calcAbsoluteVertical()
    assert(!(top.isAuto() && bottom.isAuto()));

    int contentHeight = m_height - bordersPlusPadding;

    int topValue = 0;

    bool heightIsAuto = height.isAuto();
    bool topIsAuto = top.isAuto();
    bool bottomIsAuto = bottom.isAuto();

    if (isTable() && heightIsAuto) {
         // Height is never unsolved for tables. "auto" means shrink to fit.
         // Use our height instead.
        heightValue = contentHeight;
        heightIsAuto = false;
    } else if (!heightIsAuto) {
        heightValue = calcContentHeight(height.width(containerHeight));
        if (contentHeight > heightValue) {
            if (!isTable())
                contentHeight = heightValue;
            else
                heightValue = contentHeight;
        }
    }


    if (!topIsAuto && !heightIsAuto && !bottomIsAuto) {
        /*-----------------------------------------------------------------------*\
         * If none of the three are 'auto': If both 'margin-top' and 'margin-
         * bottom' are 'auto', solve the equation under the extra constraint that
         * the two margins get equal values. If one of 'margin-top' or 'margin-
         * bottom' is 'auto', solve the equation for that value. If the values
         * are over-constrained, ignore the value for 'bottom' and solve for that
         * value.
        \*-----------------------------------------------------------------------*/
        // NOTE:  It is not necessary to solve for 'bottom' in the over constrained
        // case because the value is not used for any further calculations.

        topValue = top.width(containerHeight);

        const int availableSpace = containerHeight - (topValue + heightValue + bottom.width(containerHeight) + bordersPlusPadding);

        // Margins are now the only unknown
        if (marginTop.isAuto() && marginBottom.isAuto()) {
            // Both margins auto, solve for equality
            // NOTE: This may result in negative values.
            marginTopValue = availableSpace / 2; // split the diference
            marginBottomValue = availableSpace - marginTopValue; // account for odd valued differences
        } else if (marginTop.isAuto()) {
            // Solve for top margin
            marginBottomValue = marginBottom.width(containerHeight);
            marginTopValue = availableSpace - marginBottomValue;
        } else if (marginBottom.isAuto()) {
            // Solve for bottom margin
            marginTopValue = marginTop.width(containerHeight);
            marginBottomValue = availableSpace - marginTopValue;
        } else {
            // Over-constrained, (no need solve for bottom)
            marginTopValue = marginTop.width(containerHeight);
            marginBottomValue = marginBottom.width(containerHeight);
        }
    } else {
        /*--------------------------------------------------------------------*\
         * Otherwise, set 'auto' values for 'margin-top' and 'margin-bottom'
         * to 0, and pick the one of the following six rules that applies.
         *
         * 1. 'top' and 'height' are 'auto' and 'bottom' is not 'auto', then
         *    the height is based on the content, and solve for 'top'.
         *
         *              OMIT RULE 2 AS IT SHOULD NEVER BE HIT
         * ------------------------------------------------------------------
         * 2. 'top' and 'bottom' are 'auto' and 'height' is not 'auto', then
         *    set 'top' to the static position, and solve for 'bottom'.
         * ------------------------------------------------------------------
         *
         * 3. 'height' and 'bottom' are 'auto' and 'top' is not 'auto', then
         *    the height is based on the content, and solve for 'bottom'.
         * 4. 'top' is 'auto', 'height' and 'bottom' are not 'auto', and
         *    solve for 'top'.
         * 5. 'height' is 'auto', 'top' and 'bottom' are not 'auto', and
         *    solve for 'height'.
         * 6. 'bottom' is 'auto', 'top' and 'height' are not 'auto', and
         *    solve for 'bottom'.
        \*--------------------------------------------------------------------*/
        // NOTE: For rules 3 and 6 it is not necessary to solve for 'bottom'
        // because the value is not used for any further calculations.

        // Calculate margins, 'auto' margins are ignored.
        marginTopValue = marginTop.minWidth(containerHeight);
        marginBottomValue = marginBottom.minWidth(containerHeight);

        const int availableSpace = containerHeight - (marginTopValue + marginBottomValue + bordersPlusPadding);

        // Use rule/case that applies.
        if (topIsAuto && heightIsAuto && !bottomIsAuto) {
            // RULE 1: (height is content based, solve of top)
            heightValue = contentHeight;
            topValue = availableSpace - (heightValue + bottom.width(containerHeight));
        }
        else if (topIsAuto && !heightIsAuto && bottomIsAuto) {
            // RULE 2: (shouldn't happen)
        }
        else if (!topIsAuto && heightIsAuto && bottomIsAuto) {
            // RULE 3: (height is content based, no need solve of bottom)
            heightValue = contentHeight;
            topValue = top.width(containerHeight);
        } else if (topIsAuto && !heightIsAuto && !bottomIsAuto) {
            // RULE 4: (solve of top)
            topValue = availableSpace - (heightValue + bottom.width(containerHeight));
        } else if (!topIsAuto && heightIsAuto && !bottomIsAuto) {
            // RULE 5: (solve of height)
            topValue = top.width(containerHeight);
            heightValue = qMax(0, availableSpace - (topValue + bottom.width(containerHeight)));
        } else if (!topIsAuto && !heightIsAuto && bottomIsAuto) {
            // RULE 6: (no need solve of bottom)
            topValue = top.width(containerHeight);
        }
    }

    // Use computed values to calculate the vertical position.
    yPos = topValue + marginTopValue + containerBlock->borderTop();
}

void RenderBox::calcAbsoluteHorizontalReplaced()
{
    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.3.8 "Absolutly positioned, replaced elements"
    // <http://www.w3.org/TR/2005/WD-CSS21-20050613/visudet.html#abs-replaced-width>
    // (block-style-comments in this function correspond to text from the spec and
    // the numbers correspond to numbers in spec)

    // We don't use containingBlock(), since we may be positioned by an enclosing relpositioned inline.
    RenderObject* containerBlock = container();

    const int containerWidth = containingBlockWidth(containerBlock);

    // To match WinIE, in quirks mode use the parent's 'direction' property
    // instead of the container block's.
    EDirection containerDirection = (style()->htmlHacks()) ? parent()->style()->direction() : containerBlock->style()->direction();

    // Variables to solve.
    Length left = style()->left();
    Length right = style()->right();
    Length marginLeft = style()->marginLeft();
    Length marginRight = style()->marginRight();


    /*-----------------------------------------------------------------------*\
     * 1. The used value of 'width' is determined as for inline replaced
     *    elements.
    \*-----------------------------------------------------------------------*/
    // NOTE: This value of width is FINAL in that the min/max width calculations
    // are dealt with in calcReplacedWidth().  This means that the steps to produce
    // correct max/min in the non-replaced version, are not necessary.
    m_width = calcReplacedWidth() + borderLeft() + borderRight() + paddingLeft() + paddingRight();
    const int availableSpace = containerWidth - m_width;

    /*-----------------------------------------------------------------------*\
     * 2. If both 'left' and 'right' have the value 'auto', then if 'direction'
     *    of the containing block is 'ltr', set 'left' to the static position;
     *    else if 'direction' is 'rtl', set 'right' to the static position.
    \*-----------------------------------------------------------------------*/
    if (left.isAuto() && right.isAuto()) {
        // see FIXME 1
        if (containerDirection == LTR) {
            // 'm_staticX' should already have been set through layout of the parent.
            int staticPosition = m_staticX - containerBlock->borderLeft();
            for (RenderObject* po = parent(); po && po != containerBlock; po = po->parent())
                staticPosition += po->xPos();
            left.setValue(Fixed, staticPosition);
        } else {
            RenderObject* po = parent();
            // 'm_staticX' should already have been set through layout of the parent.
            int staticPosition = m_staticX + containerWidth + containerBlock->borderRight() - po->width();
            for (; po && po != containerBlock; po = po->parent())
                staticPosition -= po->xPos();
            right.setValue(Fixed, staticPosition);
        }
    }

    /*-----------------------------------------------------------------------*\
     * 3. If 'left' or 'right' are 'auto', replace any 'auto' on 'margin-left'
     *    or 'margin-right' with '0'.
    \*-----------------------------------------------------------------------*/
    if (left.isAuto() || right.isAuto()) {
        if (marginLeft.isAuto())
            marginLeft.setValue(Fixed, 0);
        if (marginRight.isAuto())
            marginRight.setValue(Fixed, 0);
    }

    /*-----------------------------------------------------------------------*\
     * 4. If at this point both 'margin-left' and 'margin-right' are still
     *    'auto', solve the equation under the extra constraint that the two
     *    margins must get equal values, unless this would make them negative,
     *    in which case when the direction of the containing block is 'ltr'
     *    ('rtl'), set 'margin-left' ('margin-right') to zero and solve for
     *    'margin-right' ('margin-left').
    \*-----------------------------------------------------------------------*/
    int leftValue = 0;
    int rightValue = 0;

    if (marginLeft.isAuto() && marginRight.isAuto()) {
        // 'left' and 'right' cannot be 'auto' due to step 3
        assert(!(left.isAuto() && right.isAuto()));

        leftValue = left.width(containerWidth);
        rightValue = right.width(containerWidth);

        int difference = availableSpace - (leftValue + rightValue);
        if (difference > 0) {
            m_marginLeft = difference / 2; // split the diference
            m_marginRight = difference - m_marginLeft; // account for odd valued differences
        } else {
            // see FIXME 1
            if (containerDirection == LTR) {
                m_marginLeft = 0;
                m_marginRight = difference;  // will be negative
            } else {
                m_marginLeft = difference;  // will be negative
                m_marginRight = 0;
            }
        }

    /*-----------------------------------------------------------------------*\
     * 5. If at this point there is an 'auto' left, solve the equation for
     *    that value.
    \*-----------------------------------------------------------------------*/
    } else if (left.isAuto()) {
        m_marginLeft = marginLeft.width(containerWidth);
        m_marginRight = marginRight.width(containerWidth);
        rightValue = right.width(containerWidth);

        // Solve for 'left'
        leftValue = availableSpace - (rightValue + m_marginLeft + m_marginRight);
    } else if (right.isAuto()) {
        m_marginLeft = marginLeft.width(containerWidth);
        m_marginRight = marginRight.width(containerWidth);
        leftValue = left.width(containerWidth);

        // Solve for 'right'
        rightValue = availableSpace - (leftValue + m_marginLeft + m_marginRight);
    } else if (marginLeft.isAuto()) {
        m_marginRight = marginRight.width(containerWidth);
        leftValue = left.width(containerWidth);
        rightValue = right.width(containerWidth);

        // Solve for 'margin-left'
        m_marginLeft = availableSpace - (leftValue + rightValue + m_marginRight);
    } else if (marginRight.isAuto()) {
        m_marginLeft = marginLeft.width(containerWidth);
        leftValue = left.width(containerWidth);
        rightValue = right.width(containerWidth);

        // Solve for 'margin-right'
        m_marginRight = availableSpace - (leftValue + rightValue + m_marginLeft);
    }

    /*-----------------------------------------------------------------------*\
     * 6. If at this point the values are over-constrained, ignore the value
     *    for either 'left' (in case the 'direction' property of the
     *    containing block is 'rtl') or 'right' (in case 'direction' is
     *    'ltr') and solve for that value.
    \*-----------------------------------------------------------------------*/
    else  {
        m_marginLeft = marginLeft.width(containerWidth);
        m_marginRight = marginRight.width(containerWidth);
        if (containerDirection  == LTR) {
            leftValue = left.width(containerWidth);
            rightValue = availableSpace - (leftValue + m_marginLeft + m_marginRight);
        }
        else {
            rightValue = right.width(containerWidth);
            leftValue = availableSpace - (rightValue + m_marginLeft + m_marginRight);
        }
    }

    int totalWidth = m_width + leftValue + rightValue +  m_marginLeft + m_marginRight;
    if (totalWidth > containerWidth && (containerDirection == RTL))
        leftValue = containerWidth - (totalWidth - leftValue);

    // Use computed values to calculate the horizontal position.
    int calculatedHorizontalPosition = leftValue + m_marginLeft + containerBlock->borderLeft();
    m_x = qBound((int)SHRT_MIN, calculatedHorizontalPosition, (int)SHRT_MAX);
}

void RenderBox::calcAbsoluteVerticalReplaced()
{
    // The following is based off of the W3C Working Draft from April 11, 2006 of
    // CSS 2.1: Section 10.6.5 "Absolutly positioned, replaced elements"
    // <http://www.w3.org/TR/2005/WD-CSS21-20050613/visudet.html#abs-replaced-height>
    // (block-style-comments in this function correspond to text from the spec and
    // the numbers correspond to numbers in spec)

    // We don't use containingBlock(), since we may be positioned by an enclosing relpositioned inline.
    const RenderObject* containerBlock = container();
    const int containerHeight = containerBlock->height() - containerBlock->borderTop() - containerBlock->borderBottom();

    // Variables to solve.
    Length top = style()->top();
    Length bottom = style()->bottom();
    Length marginTop = style()->marginTop();
    Length marginBottom = style()->marginBottom();


    /*-----------------------------------------------------------------------*\
     * 1. The used value of 'height' is determined as for inline replaced
     *    elements.
    \*-----------------------------------------------------------------------*/
    // NOTE: This value of height is FINAL in that the min/max height calculations
    // are dealt with in calcReplacedHeight().  This means that the steps to produce
    // correct max/min in the non-replaced version, are not necessary.
    m_height = calcReplacedHeight() + borderTop() + borderBottom() + paddingTop() + paddingBottom();
    const int availableSpace = containerHeight - m_height;

    /*-----------------------------------------------------------------------*\
     * 2. If both 'top' and 'bottom' have the value 'auto', replace 'top'
     *    with the element's static position.
    \*-----------------------------------------------------------------------*/
    if (top.isAuto() && bottom.isAuto()) {
        // m_staticY should already have been set through layout of the parent().
        int staticTop = m_staticY - containerBlock->borderTop();
        for (RenderObject* po = parent(); po && po != containerBlock; po = po->parent()) {
            staticTop += po->yPos();
        }
        top.setValue(Fixed, staticTop);
    }

    /*-----------------------------------------------------------------------*\
     * 3. If 'bottom' is 'auto', replace any 'auto' on 'margin-top' or
     *    'margin-bottom' with '0'.
    \*-----------------------------------------------------------------------*/
    // FIXME: The spec. says that this step should only be taken when bottom is
    // auto, but if only top is auto, this makes step 4 impossible.
    if (top.isAuto() || bottom.isAuto()) {
        if (marginTop.isAuto())
            marginTop.setValue(Fixed, 0);
        if (marginBottom.isAuto())
            marginBottom.setValue(Fixed, 0);
    }

    /*-----------------------------------------------------------------------*\
     * 4. If at this point both 'margin-top' and 'margin-bottom' are still
     *    'auto', solve the equation under the extra constraint that the two
     *    margins must get equal values.
    \*-----------------------------------------------------------------------*/
    int topValue = 0;
    int bottomValue = 0;

    if (marginTop.isAuto() && marginBottom.isAuto()) {
        // 'top' and 'bottom' cannot be 'auto' due to step 2 and 3 combinded.
        assert(!(top.isAuto() || bottom.isAuto()));

        topValue = top.width(containerHeight);
        bottomValue = bottom.width(containerHeight);

        int difference = availableSpace - (topValue + bottomValue);
        // NOTE: This may result in negative values.
        m_marginTop =  difference / 2; // split the difference
        m_marginBottom = difference - m_marginTop; // account for odd valued differences

    /*-----------------------------------------------------------------------*\
     * 5. If at this point there is only one 'auto' left, solve the equation
     *    for that value.
    \*-----------------------------------------------------------------------*/
    } else if (top.isAuto()) {
        m_marginTop = marginTop.width(containerHeight);
        m_marginBottom = marginBottom.width(containerHeight);
        bottomValue = bottom.width(containerHeight);

        // Solve for 'top'
        topValue = availableSpace - (bottomValue + m_marginTop + m_marginBottom);
    } else if (bottom.isAuto()) {
        m_marginTop = marginTop.width(containerHeight);
        m_marginBottom = marginBottom.width(containerHeight);
        topValue = top.width(containerHeight);

        // Solve for 'bottom'
        // NOTE: It is not necessary to solve for 'bottom' because we don't ever
        // use the value.
    } else if (marginTop.isAuto()) {
        m_marginBottom = marginBottom.width(containerHeight);
        topValue = top.width(containerHeight);
        bottomValue = bottom.width(containerHeight);

        // Solve for 'margin-top'
        m_marginTop = availableSpace - (topValue + bottomValue + m_marginBottom);
    } else if (marginBottom.isAuto()) {
        m_marginTop = marginTop.width(containerHeight);
        topValue = top.width(containerHeight);
        bottomValue = bottom.width(containerHeight);

        // Solve for 'margin-bottom'
        m_marginBottom = availableSpace - (topValue + bottomValue + m_marginTop);
    }

    /*-----------------------------------------------------------------------*\
     * 6. If at this point the values are over-constrained, ignore the value
     *    for 'bottom' and solve for that value.
    \*-----------------------------------------------------------------------*/
    else {
        m_marginTop = marginTop.width(containerHeight);
        m_marginBottom = marginBottom.width(containerHeight);
        topValue = top.width(containerHeight);

        // Solve for 'bottom'
        // NOTE: It is not necessary to solve for 'bottom' because we don't ever
        // use the value.
    }

    // Use computed values to calculate the vertical position.
    m_y = topValue + m_marginTop + containerBlock->borderTop();
}

int RenderBox::highestPosition(bool /*includeOverflowInterior*/, bool includeSelf) const
{
    return includeSelf ? 0 : m_height;
}

int RenderBox::lowestPosition(bool /*includeOverflowInterior*/, bool includeSelf) const
{
    return includeSelf ? m_height : 0;
    if (!includeSelf || !m_width)
        return 0;
    int bottom = m_height;
    if (isRelPositioned()) {
        int x = 0;
        relativePositionOffset(x, bottom);
    }
    return bottom;
}

int RenderBox::rightmostPosition(bool /*includeOverflowInterior*/, bool includeSelf) const
{
    if (!includeSelf || !m_height)
        return 0;
    int right = m_width;
    if (isRelPositioned()) {
        int y = 0;
        relativePositionOffset(right, y);
    }
    return right;
}

int RenderBox::leftmostPosition(bool /*includeOverflowInterior*/, bool includeSelf) const
{
    if (!includeSelf || !m_height)
        return m_width;
    int left = 0;
    if (isRelPositioned()) {
        int y = 0;
        relativePositionOffset(left, y);
    }
    return left;
}

int RenderBox::pageTopAfter(int y) const
{
    RenderObject* cb = container();
    if (cb)
        return cb->pageTopAfter(y+yPos()) - yPos();
    else
        return 0;
}

int RenderBox::crossesPageBreak(int t, int b) const
{
    RenderObject* cb = container();
    if (cb)
        return cb->crossesPageBreak(yPos()+t, yPos()+b);
    else
        return false;
}

bool RenderBox::handleEvent(const DOM::EventImpl& e)
{
    KHTMLAssert( scrollsOverflow() );
    bool accepted = false;

    switch ( e.id() ) {
      case EventImpl::KHTML_MOUSEWHEEL_EVENT: {

        const MouseEventImpl& me = static_cast<const MouseEventImpl&>(e);
        Qt::MouseButtons buttons = Qt::NoButton;
        Qt::KeyboardModifiers state = 0;
        Qt::Orientation orient = Qt::Vertical;

        switch (me.button()) {
        case 0:
            buttons = Qt::LeftButton;
            break;
        case 1:
            buttons = Qt::MidButton;
            break;
        case 2:
            buttons = Qt::RightButton;
            break;
        default:
            break;
        }

        if (me.orientation() == MouseEventImpl::OHorizontal)
            orient = Qt::Horizontal;

        int absx = 0;
        int absy = 0;
        absolutePosition(absx, absy);
        absx += borderLeft()+paddingLeft();
        absy += borderTop()+paddingTop();

        QPoint p(me.clientX() - absx + canvas()->view()->contentsX(),
                 me.clientY() - absy + canvas()->view()->contentsY());

        QWheelEvent we(p, -me.detail()*40, buttons, state, orient);
        KHTMLAssert(layer());
        KHTMLView* v = document()->view();
        if ( ((orient == Qt::Vertical && (v->contentsHeight() > v->visibleHeight()))  ||
              (orient == Qt::Horizontal && (v->contentsWidth() > v->visibleWidth()))) &&
               v->isScrollingFromMouseWheel() ) {
            // don't propagate wheel events to overflows if heuristics say the view is being scrolled by mouse wheel
            accepted = false;
            break;
        }

        bool d = (we.delta() < 0);
        if (orient == Qt::Vertical) {
            if (QScrollBar* vsb = layer()->verticalScrollbar()) {
                if ((d && vsb->value() != vsb->maximum()) || (!d && vsb->value() != vsb->minimum()))
                    QApplication::sendEvent( vsb, &we);
                accepted = true;
            }
        } else {
            if (QScrollBar* hsb = layer()->horizontalScrollbar()) {
                if ((d && hsb->value() != hsb->maximum()) || (!d && hsb->value() != hsb->minimum()))
                    QApplication::sendEvent( hsb, &we);
                accepted = true;
            }
        }
        break;
      }
      case EventImpl::KEYDOWN_EVENT:
      case EventImpl::KEYUP_EVENT:
        break;
      case EventImpl::KEYPRESS_EVENT:
      {
        if (!e.isKeyRelatedEvent()) break;
        const KeyEventBaseImpl& domKeyEv = static_cast<const KeyEventBaseImpl &>(e);

        QKeyEvent* const ke = domKeyEv.qKeyEvent();
        QScrollBar* vbar = layer()->verticalScrollbar();
        QScrollBar* hbar = layer()->horizontalScrollbar();
        switch (ke->key()) {
          case Qt::Key_PageUp:
            if(vbar) vbar->triggerAction(QScrollBar::SliderPageStepSub);
            break;
          case Qt::Key_PageDown:
            if(vbar) vbar->triggerAction(QScrollBar::SliderPageStepAdd);
            break;
          case Qt::Key_Up:
            if(vbar) vbar->triggerAction(QScrollBar::SliderSingleStepSub);
            break;
          case Qt::Key_Down:
            if(vbar) vbar->triggerAction(QScrollBar::SliderSingleStepAdd);
            break;
          case Qt::Key_Left:
            if (hbar) hbar->triggerAction(QScrollBar::SliderSingleStepSub);
            break;
          case Qt::Key_Right:
            if (hbar) hbar->triggerAction(QScrollBar::SliderSingleStepAdd);
            break;
          default:
            break;
        }
        break;
      }
      default:
        break;
    }
    if (accepted)
        return true;
    return RenderContainer::handleEvent(e);
}

void RenderBox::caretPos(int /*offset*/, int flags, int &_x, int &_y, int &width, int &height) const
{
#if 0
    _x = -1;

    // propagate it downwards to its children, someone will feel responsible
    RenderObject *child = firstChild();
//    if (child) qDebug() << "delegating caretPos to " << child->renderName();
    if (child) child->caretPos(offset, override, _x, _y, width, height);

    // if not, use the extents of this box. offset 0 means left, offset 1 means
    // right
    if (_x == -1) {
        //qDebug() << "no delegation";
        _x = xPos() + (offset == 0 ? 0 : m_width);
	_y = yPos();
	height = m_height;
	width = override && offset == 0 ? m_width : 1;

	// If height of box is smaller than font height, use the latter one,
	// otherwise the caret might become invisible.
	// FIXME: ignoring :first-line, missing good reason to take care of
	int fontHeight = style()->fontMetrics().height();
	if (fontHeight > height)
	  height = fontHeight;

        int absx, absy;

        RenderObject *cb = containingBlock();

        if (cb && cb != this && cb->absolutePosition(absx,absy)) {
            //qDebug() << "absx=" << absx << " absy=" << absy;
            _x += absx;
            _y += absy;
        } else {
            // we don't know our absolute position, and there is no point returning
            // just a relative one
            _x = _y = -1;
        }
    }
#endif

    _x = xPos();
    _y = yPos();
//     qDebug() << "_x " << _x << " _y " << _y;
    width = 1;		// no override is indicated in boxes

    RenderBlock *cb = containingBlock();

    // Place caret outside the border
    if (flags & CFOutside) {

        RenderStyle *s = element() && element()->parent()
			&& element()->parent()->renderer()
			? element()->parent()->renderer()->style()
			: cb->style();

        const QFontMetrics &fm = s->fontMetrics();
        height = fm.height();

	bool rtl = s->direction() == RTL;
	bool outsideEnd = flags & CFOutsideEnd;

	if (outsideEnd) {
	    _x += this->width();
	} else {
	    _x--;
	}

	int hl = fm.leading() / 2;
	if (!isReplaced() || style()->display() == BLOCK) {
	    if (!outsideEnd ^ rtl)
	        _y -= hl;
	    else
	        _y += qMax(this->height() - fm.ascent() - hl, 0);
	} else {
	    _y += baselinePosition(false) - fm.ascent() - hl;
	}

    // Place caret inside the element
    } else {
        const QFontMetrics &fm = style()->fontMetrics();
        height = fm.height();

        RenderStyle *s = style();

        _x += borderLeft() + paddingLeft();
        _y += borderTop() + paddingTop();

        // ### regard direction
	switch (s->textAlign()) {
	case LEFT:
	case KHTML_LEFT:
	case TAAUTO:	// ### find out what this does
	case JUSTIFY:
	    break;
	case CENTER:
	case KHTML_CENTER:
	    _x += contentWidth() / 2;
	    break;
	case KHTML_RIGHT:
	case RIGHT:
	    _x += contentWidth();
	    break;
	}
    }

    int absx, absy;
    if (cb && cb != this && cb->absolutePosition(absx,absy)) {
//         qDebug() << "absx=" << absx << " absy=" << absy;
        _x += absx;
        _y += absy;
    } else {
        // we don't know our absolute position, and there is no point returning
        // just a relative one
        _x = _y = -1;
    }
}

#undef DEBUG_LAYOUT
