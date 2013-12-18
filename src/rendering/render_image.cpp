/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000-2003 Dirk Mueller (mueller@kde.org)
 *           (C) 2003 Apple Computer, Inc.
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
//#define DEBUG_LAYOUT

#include "render_image.h"
#include "render_canvas.h"

#include <qdrawutil.h>
#include <QPainter>
#include <QApplication>

#include <QDebug>

#include "css/csshelper.h"
#include "misc/helper.h"
#include "misc/loader.h"
#include "html/html_formimpl.h"
#include "html/html_imageimpl.h"
#include "html/dtd.h"
#include "xml/dom2_eventsimpl.h"
#include "html/html_documentimpl.h"
#include "html/html_objectimpl.h"
#include "khtmlview.h"
#include "khtml_part.h"
#include <math.h>
#include "imload/imagepainter.h"

#include "loading_icon.cpp"

using namespace DOM;
using namespace khtml;
using namespace khtmlImLoad;

// -------------------------------------------------------------------------

RenderImage::RenderImage(NodeImpl *_element)
    : RenderReplaced(_element)
{
    m_cachedImage  = 0;
    m_imagePainter = 0;

    m_selectionState = SelectionNone;
    berrorPic = false;

    const KHTMLSettings *settings = _element->document()->view()->part()->settings();
    bUnfinishedImageFrame = settings->unfinishedImageFrame();

    setIntrinsicWidth( 0 );
    setIntrinsicHeight( 0 );
}

RenderImage::~RenderImage()
{
    delete m_imagePainter;
    if(m_cachedImage) m_cachedImage->deref( this );
}

// QPixmap RenderImage::pixmap() const
// {
//     return image ? image->pixmap() : QPixmap();
// }

void RenderImage::setStyle(RenderStyle* _style)
{
    RenderReplaced::setStyle(_style);
    // init RenderObject attributes
    setShouldPaintBackgroundOrBorder(true);
}

void RenderImage::setContentObject(CachedObject* co )
{
    if (co && m_cachedImage != co)
        updateImage( static_cast<CachedImage*>( co ) );
}

void RenderImage::updatePixmap( const QRect& r, CachedImage *o)
{
    if(o != m_cachedImage) {
        RenderReplaced::updatePixmap(r, o);
        return;
    }

    bool iwchanged = false;

    if(o->isErrorImage()) {
        int iw = o->pixmap_size().width() + 8;
        int ih = o->pixmap_size().height() + 8;

        // we have an alt and the user meant it (its not a text we invented)
        if ( element() && !alt.isEmpty() && !element()->getAttribute( ATTR_ALT ).isNull()) {
            const QFontMetrics &fm = style()->fontMetrics();
            QRect br = fm.boundingRect (  0, 0, 1024, 256, Qt::AlignLeft|Qt::TextWordWrap, alt.string() );
            if ( br.width() > iw )
                iw = br.width();
//#ifdef __GNUC__
//  #warning "KDE4: hack for testregression, remove (use above instead) when main branch"
//#endif
//              iw = br.width() + qMax(-fm.minLeftBearing(), 0) + qMax(-fm.minRightBearing(), 0);

            if ( br.height() > ih )
                ih = br.height();
        }

        if ( iw != intrinsicWidth() ) {
            setIntrinsicWidth( iw );
            iwchanged = true;
        }
        if ( ih != intrinsicHeight() ) {
            setIntrinsicHeight( ih );
            iwchanged = true;
        }
        if ( element() && element()->id() == ID_OBJECT ) {
            static_cast<HTMLObjectElementImpl*>(  element() )->renderAlternative();
            return;
        }
    }
    berrorPic = o->isErrorImage();

    bool needlayout = false;

    // Image dimensions have been changed, see what needs to be done
    if( o->pixmap_size().width() != intrinsicWidth() ||
       o->pixmap_size().height() != intrinsicHeight() || iwchanged )
    {
//           qDebug("image dimensions have been changed, old: %d/%d  new: %d/%d",
//                  intrinsicWidth(), intrinsicHeight(),
//               o->pixmap_size().width(), o->pixmap_size().height());

        if(!o->isErrorImage()) {
            setIntrinsicWidth( o->pixmap_size().width() );
            setIntrinsicHeight( o->pixmap_size().height() );
        }

         // lets see if we need to relayout at all..
         int oldwidth = m_width;
         int oldheight = m_height;
         int oldminwidth = m_minWidth;
         m_minWidth = 0;

         if ( parent() ) {
             calcWidth();
             calcHeight();
         }

         if(iwchanged || m_width != oldwidth || m_height != oldheight)
             needlayout = true;

         m_minWidth = oldminwidth;
         m_width = oldwidth;
         m_height = oldheight;
    }

    // we're not fully integrated in the tree yet.. we'll come back.
    if ( !parent() )
        return;

    if(needlayout)
    {
        if (!selfNeedsLayout())
            setNeedsLayout(true);
        if (minMaxKnown())
            setMinMaxKnown(false);
    }
    else
    {
        if (intrinsicHeight() == 0 || intrinsicWidth() == 0)
            return;
        int scaledHeight = intrinsicHeight() ? ((r.height()*contentHeight())/intrinsicHeight()) : 0;
        int scaledWidth= intrinsicWidth() ? ((r.width()*contentWidth())/intrinsicWidth()) : 0;
        int scaledX = intrinsicWidth() ? ((r.x()*contentWidth())/intrinsicWidth()) : 0;
        int scaledY = intrinsicHeight() ? ((r.y()*contentHeight())/intrinsicHeight()) : 0;

        repaintRectangle(scaledX + borderLeft() + paddingLeft(), scaledY + borderTop() + paddingTop(),
                            scaledWidth, scaledHeight);
    }
}

void RenderImage::paint(PaintInfo& paintInfo, int _tx, int _ty)
{
    if (paintInfo.phase == PaintActionOutline && style()->outlineWidth() && style()->visibility() == VISIBLE)
        paintOutline(paintInfo.p, _tx + m_x, _ty + m_y, width(), height(), style());

    if (paintInfo.phase != PaintActionForeground && paintInfo.phase != PaintActionSelection)
        return;

    // not visible or not even once layouted?
    if (style()->visibility() != VISIBLE || m_y <=  -500000)  return;

    _tx += m_x;
    _ty += m_y;

    if((_ty > paintInfo.r.bottom()) || (_ty + m_height <= paintInfo.r.top())) return;

    if(shouldPaintBackgroundOrBorder())
        paintBoxDecorations(paintInfo, _tx, _ty);

    if (!canvas()->printImages())
        return;

    int cWidth = contentWidth();
    int cHeight = contentHeight();
    int leftBorder = borderLeft();
    int topBorder = borderTop();
    int leftPad = paddingLeft();
    int topPad = paddingTop();

    // paint frame around image and loading icon as long as it is not completely loaded from web.
    if (bUnfinishedImageFrame && paintInfo.phase == PaintActionForeground && cWidth > 2 && cHeight > 2 && !complete()) {
        static QPixmap *loadingIcon;
        QColor bg = khtml::retrieveBackgroundColor(this);
        QColor fg = khtml::hasSufficientContrast(Qt::gray, bg) ? Qt::gray :
                    (hasSufficientContrast(Qt::white, bg) ? Qt::white : Qt::black);
        paintInfo.p->setPen(QPen(fg, 1));
        paintInfo.p->setBrush( Qt::NoBrush );
        const int offsetX = _tx + leftBorder + leftPad;
        const int offsetY = _ty + topBorder + topPad;
        paintInfo.p->drawRect(offsetX, offsetY, cWidth - 1, cHeight - 1);
        if (!(m_width <= 5 || m_height <= 5)) {
            if (!loadingIcon) {
                loadingIcon = new QPixmap();
                loadingIcon->loadFromData(loading_icon_data, loading_icon_len);
            }
            paintInfo.p->drawPixmap(offsetX + 4, offsetY + 4, *loadingIcon, 0, 0, cWidth - 5, cHeight - 5);
        }

    }

    CachedImage* i = m_cachedImage;

    //qDebug() << "    contents (" << contentWidth << "/" << contentHeight << ") border=" << borderLeft() << " padding=" << paddingLeft();
    if ( !i || berrorPic)
    {
        if(cWidth > 2 && cHeight > 2)
        {
            if ( !berrorPic ) {
                //qDebug("qDrawShadePanel %d/%d/%d/%d", _tx + leftBorder, _ty + topBorder, cWidth, cHeight);
                qDrawShadePanel( paintInfo.p, _tx + leftBorder + leftPad, _ty + topBorder + topPad, cWidth, cHeight,
                                 QApplication::palette(), true, 1 );
            }

            QPixmap pix = *Cache::brokenPixmap;
            if(berrorPic && (cWidth >= pix.width()+4) && (cHeight >= pix.height()+4) )
            {
                QRect r(pix.rect());
                r = r.intersect(QRect(0, 0, cWidth-4, cHeight-4));
                paintInfo.p->drawPixmap( QPoint( _tx + leftBorder + leftPad+2, _ty + topBorder + topPad+2), pix, r );
            }

            if(!alt.isEmpty()) {
                QString text = alt.string();
                paintInfo.p->setFont(style()->font());
                paintInfo.p->setPen( style()->color() );
                int ax = _tx + leftBorder + leftPad + 2;
                int ay = _ty + topBorder + topPad + 2;
                const QFontMetrics &fm = style()->fontMetrics();

//BEGIN HACK
#if 0
#ifdef __GNUC__
  #warning "KDE4: hack for testregression, remove when main branch"
#endif
                ax     += qMax(-fm.minLeftBearing(), 0);
                cWidth -= qMax(-fm.minLeftBearing(), 0);

#endif
//END HACK
                if (cWidth>5 && cHeight>=fm.height())
                    paintInfo.p->drawText(ax, ay+1, cWidth - 4, cHeight - 4, Qt::TextWordWrap, text );
            }
        }
    }
    else if (i && !i->isTransparent() &&
             i->image()->size().width() && i->image()->size().height())
    {
        paintInfo.p->setPen( Qt::black ); // used for bitmaps
        //const QPixmap& pix = i->pixmap();
        if (!m_imagePainter)
            m_imagePainter = new ImagePainter(i->image());

       // If we have a scaled  painter we want to handle the resizing ourselves, so figure out the scaled size,
       QTransform painterTransform = paintInfo.p->transform();

       bool scaled = painterTransform.isScaling() && !painterTransform.isRotating();

       QRect scaledRect; // bounding box of the whole thing, transformed, so we also know where the origin goes.
       if (scaled) {
           scaledRect = painterTransform.mapRect(QRect(0, 0, contentWidth(), contentHeight()));
           m_imagePainter->setSize(QSize(scaledRect.width(), scaledRect.height()));
       } else {
           m_imagePainter->setSize(QSize(contentWidth(), contentHeight()));
       }

        // Now, figure out the rectangle to paint (in painter coordinates), by interesting us with the painting clip rectangle.
        int x = _tx + leftBorder + leftPad;
        int y = _ty + topBorder + topPad;
        QRect imageGeom   = QRect(0, 0, contentWidth(), contentHeight());

        QRect clipPortion = paintInfo.r.translated(-x, -y);
        imageGeom &= clipPortion;

       QPoint destPos = QPoint(x + imageGeom.x(), y + imageGeom.y());

       // If we're scaling, reset the painters transform, and apply it ourselves; though
       // being careful not apply the translation to the source rect.
       if (scaled) {
           paintInfo.p->resetTransform();
           destPos   = painterTransform.map(destPos);
           imageGeom = painterTransform.mapRect(imageGeom).translated(-scaledRect.topLeft());
       }

        m_imagePainter->paint(destPos.x(), destPos.y(), paintInfo.p,
                                    imageGeom.x(),     imageGeom.y(),
                                    imageGeom.width(), imageGeom.height());

       if (scaled)
           paintInfo.p->setTransform(painterTransform);

    }
    if (m_selectionState != SelectionNone) {
//    qDebug() << "_tx " << _tx << " _ty " << _ty << " _x " << _x << " _y " << _y;
        // Draw in any case if inside selection. For selection borders, the
	// offset will decide whether to draw selection or not
	bool draw = true;
	if (m_selectionState != SelectionInside) {
	    int startPos, endPos;
            selectionStartEnd(startPos, endPos);
            if(selectionState() == SelectionStart)
                endPos = 1;
            else if(selectionState() == SelectionEnd)
                startPos = 0;
	    draw = endPos - startPos > 0;
	}
	if (draw) {
    	    // setting the brush origin is important for compatibility,
	    // don't touch it unless you know what you're doing
    	    paintInfo.p->setBrushOrigin(_tx, _ty - paintInfo.r.y());
            paintInfo.p->fillRect(_tx, _ty, width(), height(),
		    QBrush(style()->palette().color( QPalette::Active, QPalette::Highlight ),
		    Qt::Dense4Pattern));
	}
    }
}

void RenderImage::layout()
{
    KHTMLAssert( needsLayout());
    KHTMLAssert( minMaxKnown() );

    //short m_width = 0;

    // minimum height
    m_height = m_cachedImage && m_cachedImage->isErrorImage() ? intrinsicHeight() : 0;

    calcWidth();
    calcHeight();

    setNeedsLayout(false);
}

bool RenderImage::nodeAtPoint(NodeInfo& info, int _x, int _y, int _tx, int _ty, HitTestAction hitTestAction, bool inside)
{
    inside |= RenderReplaced::nodeAtPoint(info, _x, _y, _tx, _ty, hitTestAction, inside);

    if (inside && element()) {
        int tx = _tx + m_x;
        int ty = _ty + m_y;

        HTMLImageElementImpl* i = element()->id() == ID_IMG ? static_cast<HTMLImageElementImpl*>(element()) : 0;
        HTMLMapElementImpl* map;
        if (i && i->document()->isHTMLDocument() &&
            (map = static_cast<HTMLDocumentImpl*>(i->document())->getMap(i->imageMap()))) {
            // we're a client side image map
            inside = map->mapMouseEvent(_x - tx, _y - ty, contentWidth(), contentHeight(), info);
            info.setInnerNonSharedNode(element());
        }
    }

    return inside;
}

void RenderImage::updateImage(CachedImage* newImage)
{
    if (newImage == m_cachedImage)
        return;

    delete m_imagePainter; m_imagePainter = 0;

    if ( m_cachedImage )
        m_cachedImage->deref(this);

    // Note: this must be up-to-date before we ref, since
    // ref can cause us to be notified of it being loaded
    m_cachedImage  = newImage;
    if ( m_cachedImage )
        m_cachedImage->ref(this);

    // if the loading finishes we might get an error and then the image is deleted
    if ( m_cachedImage )
        berrorPic = m_cachedImage->isErrorImage();
    else
        berrorPic = true;
}

void RenderImage::updateFromElement()
{
    if (element()->id() == ID_INPUT)
        alt = static_cast<HTMLInputElementImpl*>(element())->altText();
    else if (element()->id() == ID_IMG)
        alt = static_cast<HTMLImageElementImpl*>(element())->altText();

    DOMString u = element()->id() == ID_OBJECT ?
                  element()->getAttribute(ATTR_DATA) : element()->getAttribute(ATTR_SRC);

    if (!u.isEmpty() &&
        ( !m_cachedImage || m_cachedImage->url() != u ) ) {
        CachedImage *new_image = element()->document()->docLoader()->
                                 requestImage(khtml::parseURL(u));

        if(new_image && new_image != m_cachedImage)
            updateImage( new_image );
    }
}

bool RenderImage::complete() const
{
     // "complete" means that the image has been loaded
     // but also that its width/height (contentWidth(),contentHeight()) have been calculated.
     return m_cachedImage && m_cachedImage->isComplete() && !needsLayout();
}

bool RenderImage::isWidthSpecified() const
{
    switch (style()->width().type()) {
        case Fixed:
        case Percent:
            return true;
        default:
            return false;
    }
    assert(false);
    return false;
}

bool RenderImage::isHeightSpecified() const
{
    switch (style()->height().type()) {
        case Fixed:
        case Percent:
            return true;
        default:
            return false;
    }
    assert(false);
    return false;
}

short RenderImage::calcAspectRatioWidth() const
{
    if (intrinsicHeight() == 0)
        return 0;
    if (!m_cachedImage || m_cachedImage->isErrorImage())
        return intrinsicWidth(); // Don't bother scaling.
    return RenderReplaced::calcReplacedHeight() * intrinsicWidth() / intrinsicHeight();
}

int RenderImage::calcAspectRatioHeight() const
{
    if (intrinsicWidth() == 0)
        return 0;
    if (!m_cachedImage || m_cachedImage->isErrorImage())
        return intrinsicHeight(); // Don't bother scaling.
    return RenderReplaced::calcReplacedWidth() * intrinsicHeight() / intrinsicWidth();
}

short RenderImage::calcReplacedWidth() const
{
    int width;
    if (isWidthSpecified())
        width = calcReplacedWidthUsing(Width);
    else
        width = calcAspectRatioWidth();
    int minW = calcReplacedWidthUsing(MinWidth);
    int maxW = style()->maxWidth().isUndefined() ? width : calcReplacedWidthUsing(MaxWidth);

    if (width > maxW)
        width = maxW;

    if (width < minW)
        width = minW;

    return width;
}

int RenderImage::calcReplacedHeight() const
{
    int height;
    if (isHeightSpecified())
        height = calcReplacedHeightUsing(Height);
    else
        height = calcAspectRatioHeight();

    int minH = calcReplacedHeightUsing(MinHeight);
    int maxH = style()->maxHeight().isUndefined() ? height : calcReplacedHeightUsing(MaxHeight);

    if (height > maxH)
        height = maxH;

    if (height < minH)
        height = minH;

    return height;
}

#if 0
void RenderImage::caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const
{
    RenderReplaced::caretPos(offset, flags, _x, _y, width, height);

#if 0	// doesn't work reliably
    height = intrinsicHeight();
    width = override && offset == 0 ? intrinsicWidth() : 0;
    _x = xPos();
    _y = yPos();
    if (offset > 0) _x += intrinsicWidth();

    RenderObject *cb = containingBlock();

    int absx, absy;
    if (cb && cb != this && cb->absolutePosition(absx,absy))
    {
        _x += absx;
        _y += absy;
    } else {
        // we don't know our absolute position, and there is no point returning
        // just a relative one
        _x = _y = -1;
    }
#endif
}
#endif
