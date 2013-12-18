/**
 * This file is part of the HTML widget for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 2003 Apple Computer, Inc.
 *           (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2007-2009 Germain Garand (germain@ebooksfrance.org)
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
 */


#include "rendering/render_canvas.h"
#include "rendering/render_layer.h"
#include "rendering/render_replaced.h"
#include "xml/dom_docimpl.h"

#include "khtmlview.h"
#include "khtml_part.h"
#include <QDebug>
#include <QScrollBar>

using namespace khtml;

//#define BOX_DEBUG
//#define SPEED_DEBUG
#ifdef SPEED_DEBUG
  #include <QTime>
#endif

RenderCanvas::RenderCanvas(DOM::NodeImpl* node, KHTMLView *view)
    : RenderBlock(node)
{
    // init RenderObject attributes
    setInline(false);
    setIsAnonymous(false);

    m_view = view;
    // try to contrain the width to the views width

    m_minWidth = 0;
    m_height = 0;

    m_width = m_minWidth;
    m_maxWidth = m_minWidth;

    m_rootWidth = m_rootHeight = 0;
    m_viewportWidth = m_viewportHeight = 0;
    m_cachedDocWidth = m_cachedDocHeight = -1;

    setPositioned(true); // to 0,0 :)

    m_staticMode = false;
    m_pagedMode = false;
    m_printImages = true;

    m_pageTop = 0;
    m_pageBottom = 0;

    m_page = 0;

    m_maximalOutlineSize = 0;

    m_selectionStart = 0;
    m_selectionEnd = 0;
    m_selectionStartPos = -1;
    m_selectionEndPos = -1;

    m_needsWidgetMasks = false;

    m_isPerformingLayout = false;

    // Create a new root layer for our layer hierarchy.
    m_layer = new (node->document()->renderArena()) RenderLayer(this);
}

RenderCanvas::~RenderCanvas()
{
    delete m_page;
}

void RenderCanvas::setStyle(RenderStyle* style)
{
    /*
    if (m_pagedMode)
        style->setOverflow(OHIDDEN); */
    RenderBlock::setStyle(style);
}

void RenderCanvas::calcHeight()
{
    if (m_pagedMode || !m_view)
        m_height = m_rootHeight;
    else
        m_height = m_view->visibleHeight();
}

void RenderCanvas::calcWidth()
{
    // the width gets set by KHTMLView::print when printing to a printer.
    if(m_pagedMode || !m_view)
    {
        m_width = m_rootWidth;
        return;
    }

    m_width = m_view ? m_view->frameWidth() : m_minWidth;

    if (style()->marginLeft().isFixed())
        m_marginLeft = style()->marginLeft().value();
    else
        m_marginLeft = 0;

    if (style()->marginRight().isFixed())
        m_marginRight = style()->marginRight().value();
    else
        m_marginRight = 0;
}

void RenderCanvas::calcMinMaxWidth()
{
    KHTMLAssert( !minMaxKnown() );

    RenderBlock::calcMinMaxWidth();

    m_maxWidth = m_minWidth;

    setMinMaxKnown();
}

void RenderCanvas::layout()
{
    m_isPerformingLayout = true;

    if (m_pagedMode) {
       m_minWidth = m_width;
//        m_maxWidth = m_width;
    }

    m_needsFullRepaint =  markedForRepaint() || !view() || view()->needsFullRepaint() || m_pagedMode;

    setChildNeedsLayout(true);
    setMinMaxKnown(false);
    for(RenderObject* c = firstChild(); c; c = c->nextSibling())
        c->setChildNeedsLayout(true);

    int oldWidth = m_width;
    int oldHeight = m_height;

    m_cachedDocWidth = m_cachedDocHeight = -1;

    if (m_pagedMode || !m_view) {
        m_width = m_rootWidth;
        m_height = m_rootHeight;
    }
    else
    {
        m_viewportWidth = m_width = m_view->visibleWidth();
        m_viewportHeight = m_height = m_view->visibleHeight();
    }

#ifdef SPEED_DEBUG
    QTime qt;
    qt.start();
#endif

    if ( recalcMinMax() )
	recalcMinMaxWidths();

#ifdef SPEED_DEBUG
    qDebug() << "RenderCanvas::calcMinMax time used=" << qt.elapsed();
    qt.start();
#endif

    bool relayoutChildren = (oldWidth != m_width) || (oldHeight != m_height);

    RenderBlock::layoutBlock( relayoutChildren );

#ifdef SPEED_DEBUG
    qDebug() << "RenderCanvas::layout time used=" << qt.elapsed();
    qt.start();
#endif

    updateDocumentSize();

    layer()->updateLayerPositions( layer(), needsFullRepaint(), true );

    if (!m_pagedMode && m_needsWidgetMasks)
        layer()->updateWidgetMasks(layer());

    scheduleDeferredRepaints();
    setNeedsLayout(false);

    m_isPerformingLayout = false;
#ifdef SPEED_DEBUG
    qDebug() << "RenderCanvas::end time used=" << qt.elapsed();
#endif
}

void RenderCanvas::setNeedsWidgetMasks( bool b ) 
{
    if (b == m_needsWidgetMasks)
        return;
    m_needsWidgetMasks = b;
    KHTMLWidget* k = dynamic_cast<KHTMLWidget*>(m_view);
    // ### should be reversible
    if (k && b && k->m_kwp->isRedirected()) {
        k->m_kwp->setIsRedirected(!b);
        if (k->m_kwp->renderWidget())
            k->m_kwp->renderWidget()->setNeedsLayout(true);
    }
}

void RenderCanvas::updateDocumentSize()
{
     // update our cached document size
    int hDocH = m_cachedDocHeight = docHeight();
    int hDocW = m_cachedDocWidth = docWidth();
    
    int zLevel = m_view? m_view->zoomLevel() : 100;
    hDocW = hDocW*zLevel/100;
    hDocH = hDocH*zLevel/100;

    if (!m_pagedMode && m_view) {

        // we need to adjust the document's size to make sure we don't enter
        // an endless cycle of scrollbars being added, then removed at the next layout.

        bool vss = m_view->verticalScrollBar()->isVisible();
        bool hss = m_view->horizontalScrollBar()->isVisible();

        // calculate the extent of scrollbars
        int vsPixSize = m_view->verticalScrollBar()->sizeHint().width();
        int hsPixSize = m_view->horizontalScrollBar()->sizeHint().height();

        // this variable holds the size the viewport will have after the inner content is resized to
        // the new document dimensions
        QSize viewport = m_view->maximumViewportSize();

        // of course, if the scrollbar policy isn't auto, there's no point adjusting any value..
        int overrideH = m_view->verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded ? 0 : hDocH;
        int overrideW = m_view->verticalScrollBarPolicy() == Qt::ScrollBarAsNeeded ? 0 : hDocW;
        
        if ( !overrideW && hDocW > viewport.width() )
            viewport.setHeight( viewport.height() - hsPixSize );
        if ( !overrideH && hDocH > viewport.height() )
            viewport.setWidth( viewport.width() - vsPixSize );
            
        // if we are about to show a scrollbar, and the document is sized to the viewport w or h,
        // then reserve the scrollbar space so that it doesn't trigger the _other_ scrollbar

        if (!vss && m_width - vsPixSize == viewport.width() &&
            hDocW <= m_width)
            hDocW = qMin( hDocW, viewport.width() );

        if (!hss && m_height - hsPixSize == viewport.height() &&
            hDocH <= m_height)
            hDocH = qMin( hDocH, viewport.height() );

        // likewise, if a scrollbar is shown, and we have a cunning plan to turn it off,
        // think again if we are falling downright in the hysteresis zone

        if (vss && viewport.width() > hDocW && hDocW > m_view->visibleWidth())
            hDocW = viewport.width()+1;

        if (hss && viewport.height() > hDocH && hDocH > m_view->visibleHeight())
            hDocH = viewport.height()+1;

        m_view->resizeContents((overrideW ? overrideW : hDocW), (overrideH ? overrideH : hDocH));

    }
    layer()->resize( qMax( m_cachedDocWidth,int( m_width ) ), qMax( m_cachedDocHeight,m_height ) );
}

void RenderCanvas::updateDocSizeAfterLayerTranslation( RenderObject* o, bool posXOffset, bool posYOffset )
{
    if (needsLayout())
        return;
    int rightmost, lowest;
    o->absolutePosition( rightmost, lowest );
    if (posXOffset) {
        rightmost += o->rightmostPosition(false, true);
        setCachedDocWidth( qMax(docWidth(), rightmost) );
    } else {
        setCachedDocWidth( -1 );
    }
    if (posYOffset) {
        lowest += o->lowestPosition(false, true);
        setCachedDocHeight( qMax(docHeight(), lowest) );
    } else {
        setCachedDocHeight( -1 );
    }
//    qDebug() << " posXOffset: " << posXOffset << " posYOffset " << posYOffset << " m_cachedDocWidth  " <<  m_cachedDocWidth << " m_cachedDocHeight  " << m_cachedDocHeight;
    updateDocumentSize();
}

QRegion RenderCanvas::staticRegion() const
{
   QRegion ret = QRegion();

   // position:fixed objects
   if (m_positionedObjects) {
       RenderObject* obj;
       QListIterator<RenderObject*> it(*m_positionedObjects);
        while (it.hasNext()) {
            obj = it.next();
            if (obj->style()->position() == PFIXED && obj->layer()) {
                ret += obj->layer()->paintedRegion(layer());
                assert( m_fixedPosition.contains(obj) );
            }
        }
   }

   // background-attachment:fixed images
   QSetIterator<RenderObject *> i(m_fixedBackground);
   while (i.hasNext()) {
       RenderObject *ro = i.next();
       if (ro && ro->isBox()) {
           int d1, d2, d3, d4;
           const BackgroundLayer* bgLayer = ro->style()->backgroundLayers();
           while (bgLayer) {
               CachedImage* bg = bgLayer->backgroundAttachment() == BGAFIXED ? bgLayer->backgroundImage() : 0;
               if (bg && bg->isComplete() && !bg->isTransparent() && !bg->isErrorImage()) {
                   int xpos, ypos;
                   absolutePosition(xpos,ypos);
                   ret += static_cast<RenderBox*>(ro)->getFixedBackgroundImageRect(bgLayer, d1, d2, d3, d4)
                             .intersected(QRect(xpos,ypos,ro->width(),ro->height()));
               }
               bgLayer = bgLayer->next();
           }
       }
   }
   return ret;
}

bool RenderCanvas::needsFullRepaint() const
{
    return m_needsFullRepaint || m_pagedMode;
}

void RenderCanvas::repaintViewRectangle(int x, int y, int w, int h, bool asap)
{
  KHTMLAssert( view() );
  view()->scheduleRepaint( x, y, w, h, asap );
}

bool RenderCanvas::absolutePosition(int &xPos, int &yPos, bool f) const
{
    if ( f && m_pagedMode) {
        xPos = 0;
        yPos = m_pageTop;
    }
    else if ( f && m_view) {
        xPos = m_view->contentsX();
        yPos = m_view->contentsY();
    }
    else {
        xPos = yPos = 0;
    }
    return true;
}

void RenderCanvas::paint(PaintInfo& paintInfo, int _tx, int _ty)
{
#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << this << " ::paintObject() w/h = (" << width() << "/" << height() << ")";
#endif

    // 1. paint background, borders etc
    if(paintInfo.phase == PaintActionElementBackground) {
        paintBoxDecorations(paintInfo, _tx, _ty);
        return;
    }

    // 2. paint contents
    for( RenderObject *child = firstChild(); child; child=child->nextSibling())
        if(!child->layer() && !child->isFloating())
            child->paint(paintInfo, _tx, _ty);

    // 3. paint floats.
    if (paintInfo.phase == PaintActionFloat)
        paintFloats(paintInfo, _tx, _ty);

#ifdef BOX_DEBUG
    if (m_view)
    {
        _tx += m_view->contentsX();
        _ty += m_view->contentsY();
    }

    outlineBox(paintInfo.p, _tx, _ty);
#endif

}

void RenderCanvas::paintBoxDecorations(PaintInfo& paintInfo, int /*_tx*/, int /*_ty*/)
{
    if ((firstChild() && firstChild()->style()->visibility() == VISIBLE) || !view())
        return;

    paintInfo.p->fillRect(paintInfo.r, view()->palette().color(QPalette::Active, QPalette::Base));
}

void RenderCanvas::repaintRectangle(int x, int y, int w, int h, Priority p, bool f)
{
    if (m_staticMode) return;
//    qDebug() << "updating views contents (" << x << "/" << y << ") (" << w << "/" << h << ")";

    if (f && m_pagedMode) {
        y += m_pageTop;
    } else
    if ( f && m_view ) {
        x += m_view->contentsX();
        y += m_view->contentsY();
    }

    QRect vr = viewRect();
    QRect ur(x, y, w, h);

    if (m_view && ur.intersects(vr)) {

        if (p == RealtimePriority)
            m_view->updateContents(ur);
        else if (p == HighPriority)
            m_view->scheduleRepaint(x, y, w, h, true /*asap*/);
        else
            m_view->scheduleRepaint(x, y, w, h);
    }
}

void RenderCanvas::deferredRepaint( RenderObject* o )
{
    m_dirtyChildren.append( o );
}

void RenderCanvas::scheduleDeferredRepaints()
{
    if (!needsFullRepaint()) {
        QList<RenderObject*>::const_iterator it;
        for ( it = m_dirtyChildren.constBegin(); it != m_dirtyChildren.constEnd(); ++it )
            (*it)->repaint();
    }
    //qDebug() << "scheduled deferred repaints: " << m_dirtyChildren.count() << " needed full repaint: " << needsFullRepaint();
    m_dirtyChildren.clear();
}

void RenderCanvas::repaint(Priority p)
{
    if (m_view && !m_staticMode) {
        if (p == RealtimePriority) {
            //m_view->resizeContents(docWidth(), docHeight());
            m_view->unscheduleRepaint();
            if (needsLayout()) {
                m_view->scheduleRelayout();
                return;
            }
	    // ### same as in repaintRectangle
            m_view->updateContents(m_view->contentsX(), m_view->contentsY(),
                                   m_view->visibleWidth(), m_view->visibleHeight());
        }
        else if (p == HighPriority)
            m_view->scheduleRepaint(m_view->contentsX(), m_view->contentsY(),
                                    m_view->visibleWidth(), m_view->visibleHeight(), true /*asap*/);
        else
            m_view->scheduleRepaint(m_view->contentsX(), m_view->contentsY(),
                                    m_view->visibleWidth(), m_view->visibleHeight());
    }
}

static QRect enclosingPositionedRect (RenderObject *n)
{
    RenderObject *enclosingParent =  n->containingBlock();
    QRect rect(0,0,0,0);
    if (enclosingParent) {
        int ox, oy;
        enclosingParent->absolutePosition(ox, oy);
        if (!enclosingParent->hasOverflowClip()) {
            ox += enclosingParent->overflowLeft();
            oy += enclosingParent->overflowTop();
        }
        rect.setX(ox);
        rect.setY(oy);
        rect.setWidth(enclosingParent->effectiveWidth());
        rect.setHeight(enclosingParent->effectiveHeight());
    }
    return rect;
}

void RenderCanvas::addStaticObject( RenderObject*o, bool pos)
{ 
    QSet<RenderObject*>& set = pos ? m_fixedPosition : m_fixedBackground;
    if (!o || !o->isBox() || set.contains(o))
        return;
    set.insert(o);
    if (view())
        view()->addStaticObject(pos);
}

void RenderCanvas::removeStaticObject( RenderObject*o, bool pos)
{
    QSet<RenderObject*>& set = pos ? m_fixedPosition : m_fixedBackground;
    if (!o || !o->isBox() || !set.contains(o))
        return;
    set.remove(o); 
    if (view())
        view()->removeStaticObject(pos);
}

QRect RenderCanvas::selectionRect() const
{
    RenderObject *r = m_selectionStart;
    if (!r)
        return QRect();

    QRect selectionRect = enclosingPositionedRect(r);

    while (r && r != m_selectionEnd)
    {
        RenderObject* n;
        if ( !(n = r->firstChild()) ){
            if ( !(n = r->nextSibling()) )
            {
                n = r->parent();
                while (n && !n->nextSibling())
                    n = n->parent();
                if (n)
                    n = n->nextSibling();
            }
        }
        r = n;
        if (r) {
            selectionRect = selectionRect.unite(enclosingPositionedRect(r));
        }
    }

    return selectionRect;
}

void RenderCanvas::setSelection(RenderObject *s, int sp, RenderObject *e, int ep)
{
    // Check we got valid renderobjects. www.msnbc.com and clicking
    // around, to find the case where this happened.
    if ( !s || !e )
    {
        qWarning() << "RenderCanvas::setSelection() called with start=" << s << " end=" << e;
        return;
    }
//     qDebug() << "RenderCanvas::setSelection(" << s << "," << sp << "," << e << "," << ep << ")";

    bool changedSelectionBorder = ( s != m_selectionStart || e != m_selectionEnd );

    // Cut out early if the selection hasn't changed.
    if ( !changedSelectionBorder && m_selectionStartPos == sp && m_selectionEndPos == ep )
        return;

    // Record the old selected objects.  Will be used later
    // to delta against the selected objects.

    RenderObject *oldStart = m_selectionStart;
    int oldStartPos = m_selectionStartPos;
    RenderObject *oldEnd = m_selectionEnd;
    int oldEndPos = m_selectionEndPos;
    QList<RenderObject*> oldSelectedInside;
    QList<RenderObject*> newSelectedInside;
    RenderObject *os = oldStart;

    while (os && os != oldEnd)
    {
        RenderObject* no;
        if ( !(no = os->firstChild()) ){
            if ( !(no = os->nextSibling()) )
            {
                no = os->parent();
                while (no && !no->nextSibling())
                    no = no->parent();
                if (no)
                    no = no->nextSibling();
            }
        }
        if (os->selectionState() == SelectionInside && !oldSelectedInside.contains(os))
            oldSelectedInside.append(os);

        os = no;
    }
    if (changedSelectionBorder)
        clearSelection(false);

    while (s->firstChild())
        s = s->firstChild();
    while (e->lastChild())
        e = e->lastChild();

#if 0
    bool changedSelectionBorder = ( s != m_selectionStart || e != m_selectionEnd );

    if ( !changedSelectionBorder && m_selectionStartPos == sp && m_selectionEndPos = ep )
        return;
#endif

    // set selection start
    if (m_selectionStart)
        m_selectionStart->setIsSelectionBorder(false);
    m_selectionStart = s;
    if (m_selectionStart)
        m_selectionStart->setIsSelectionBorder(true);
    m_selectionStartPos = sp;

    // set selection end
    if (m_selectionEnd)
        m_selectionEnd->setIsSelectionBorder(false);
    m_selectionEnd = e;
    if (m_selectionEnd)
        m_selectionEnd->setIsSelectionBorder(true);
    m_selectionEndPos = ep;

#if 0
    // qDebug() << "old selection (" << oldStart << "," << oldStartPos << "," << oldEnd << "," << oldEndPos << ")";
    // qDebug() << "new selection (" << s << "," << sp << "," << e << "," << ep << ")";
#endif

    // update selection status of all objects between m_selectionStart and m_selectionEnd
    RenderObject* o = s;

    while (o && o!=e)
    {
        o->setSelectionState(SelectionInside);
//      qDebug() << "setting selected " << o << ", " << o->isText();
        RenderObject* no;
        if ( !(no = o->firstChild()) )
            if ( !(no = o->nextSibling()) )
            {
                no = o->parent();
                while (no && !no->nextSibling())
                    no = no->parent();
                if (no)
                    no = no->nextSibling();
            }
        if (o->selectionState() == SelectionInside && !newSelectedInside.contains(o))
            newSelectedInside.append(o);

        o=no;
    }
    s->setSelectionState(SelectionStart);
    e->setSelectionState(SelectionEnd);
    if(s == e) s->setSelectionState(SelectionBoth);

    if (!m_view)
        return;

    int i;
    i = newSelectedInside.indexOf(s);
    if (i != -1) newSelectedInside.removeAt(i);
    i = newSelectedInside.indexOf(e);
    if (i != -1) newSelectedInside.removeAt(i);

    QRect updateRect;

    // Don't use repaint() because it will cause all rects to
    // be united (see khtmlview::scheduleRepaint()).  Instead
    // just draw damage rects for objects that have a change
    // in selection state.
    // ### for Qt, updateContents will unite them, too. This has to be
    // circumvented somehow (LS)

    // Are any of the old fully selected objects not in the new selection?
    // If so we have to draw them.
    // Could be faster by building list of non-intersecting rectangles rather
    // than unioning rectangles.
    QListIterator<RenderObject*> oldIterator(oldSelectedInside);
    bool firstRect = true;
    while (oldIterator.hasNext()) {
        o = oldIterator.next();
        if (!newSelectedInside.contains(o)){
            if (firstRect){
                updateRect = enclosingPositionedRect(o);
                firstRect = false;
            }
            else
                updateRect = updateRect.unite(enclosingPositionedRect(o));
        }
    }
    if (!firstRect){
        m_view->updateContents( updateRect );
    }

    // Are any of the new fully selected objects not in the previous selection?
    // If so we have to draw them.
    // Could be faster by building list of non-intersecting rectangles rather
    // than unioning rectangles.
    QListIterator<RenderObject*> newIterator(newSelectedInside);
    firstRect = true;
    while (newIterator.hasNext()) {
        o = newIterator.next();
        if (!oldSelectedInside.contains(o)) {
            if (firstRect) {
                updateRect = enclosingPositionedRect(o);
                firstRect = false;
            }
            else
                updateRect = updateRect.unite(enclosingPositionedRect(o));
        }
    }
    if (!firstRect) {
        m_view->updateContents( updateRect );
    }

    // Is the new starting object different, or did the position in the starting
    // element change?  If so we have to draw it.
    if (oldStart != m_selectionStart ||
        (oldStart == oldEnd && (oldStartPos != m_selectionStartPos || oldEndPos != m_selectionEndPos)) ||
        (oldStart == m_selectionStart && oldStartPos != m_selectionStartPos)){
        m_view->updateContents( enclosingPositionedRect(m_selectionStart) );
    }

    // Draw the old selection start object if it's different than the new selection
    // start object.
    if (oldStart && oldStart != m_selectionStart){
        m_view->updateContents( enclosingPositionedRect(oldStart) );
    }

    // Does the selection span objects and is the new end object different, or did the position
    // in the end element change?  If so we have to draw it.
    if (/*(oldStart != oldEnd || !oldEnd) &&*/
        (oldEnd != m_selectionEnd ||
        (oldEnd == m_selectionEnd && oldEndPos != m_selectionEndPos))){
        m_view->updateContents( enclosingPositionedRect(m_selectionEnd) );
    }

    // Draw the old selection end object if it's different than the new selection
    // end object.
    if (oldEnd && oldEnd != m_selectionEnd){
        m_view->updateContents( enclosingPositionedRect(oldEnd) );
    }
}

void RenderCanvas::clearSelection(bool doRepaint)
{
    // update selection status of all objects between m_selectionStart and m_selectionEnd
    RenderObject* o = m_selectionStart;
    while (o && o!=m_selectionEnd)
    {
        if (o->selectionState()!=SelectionNone)
            if (doRepaint)
                o->repaint();
        o->setSelectionState(SelectionNone);
        o->repaint();
        RenderObject* no;
        if ( !(no = o->firstChild()) )
            if ( !(no = o->nextSibling()) )
            {
                no = o->parent();
                while (no && !no->nextSibling())
                    no = no->parent();
                if (no)
                    no = no->nextSibling();
            }
        o=no;
    }
    if (m_selectionEnd) {
        m_selectionEnd->setSelectionState(SelectionNone);
        if (doRepaint)
            m_selectionEnd->repaint();
    }

    // set selection start & end to 0
    if (m_selectionStart)
        m_selectionStart->setIsSelectionBorder(false);
    m_selectionStart = 0;
    m_selectionStartPos = -1;

    if (m_selectionEnd)
        m_selectionEnd->setIsSelectionBorder(false);
    m_selectionEnd = 0;
    m_selectionEndPos = -1;
}

void RenderCanvas::selectionStartEnd(int& spos, int& epos)
{
    spos = m_selectionStartPos;
    epos = m_selectionEndPos;
}

QRect RenderCanvas::viewRect() const
{
    if (m_pagedMode)
        if (m_pageTop == m_pageBottom) {
            // qDebug() << "viewRect: " << QRect(0, m_pageTop, m_width, m_height);
            return QRect(0, m_pageTop, m_width, m_height);
        }
        else {
            // qDebug() << "viewRect: " << QRect(0, m_pageTop, m_width, m_pageBottom - m_pageTop);
            return QRect(0, m_pageTop, m_width, m_pageBottom - m_pageTop);
        }
    else if (m_view) {
        const int z = m_view->zoomLevel() ? m_view->zoomLevel() : 100;
        return QRect(m_view->contentsX() * 100 / z, m_view->contentsY() * 100 / z,
                     m_view->visibleWidth(), m_view->visibleHeight());
    } else
        return QRect(0,0,m_rootWidth,m_rootHeight);
}

int RenderCanvas::docHeight() const
{
    if (m_cachedDocHeight != -1)
        return m_cachedDocHeight;

    int h;
    if (m_pagedMode || !m_view)
        h = m_height;
    else
        h = 0;

    RenderObject *fc = firstChild();
    if(fc) {
        int dh = fc->overflowHeight() + fc->marginTop() + fc->marginBottom();
        int lowestPos = fc->lowestPosition(false);
// qDebug() << "h " << h << " lowestPos " << lowestPos << " dh " << dh << " fc->rh " << fc->effectiveHeight() << " fc->height() " << fc->height();
        if( lowestPos > dh )
            dh = lowestPos;
        lowestPos = lowestAbsolutePosition();
        if( lowestPos > dh )
            dh = lowestPos;
        if( dh > h )
            h = dh;
    }

    RenderLayer *layer = m_layer;
    h = qMax( h, layer->yPos() + layer->height() );
// qDebug() << "h " << h << " layer(" << layer->renderer()->renderName() << "@" << layer->renderer() << ")->height " << layer->height() << " lp " << (layer->yPos() + layer->height()) << " height() " << layer->renderer()->height() << " rh " << layer->renderer()->effectiveHeight();
    return h;
}

int RenderCanvas::docWidth() const
{
    if (m_cachedDocWidth != -1)
        return m_cachedDocWidth;

    int w;
    if (m_pagedMode || !m_view)
        w = m_width;
    else
        w = 0;

    RenderObject *fc = firstChild();
    if(fc) {
        // ow: like effectiveWidth() but without the negative
        const int ow = fc->hasOverflowClip() ? fc->width() : fc->overflowWidth();
        int dw = ow + fc->marginLeft() + fc->marginRight();
        int rightmostPos = fc->rightmostPosition(false);
// qDebug() << "w " << w << " rightmostPos " << rightmostPos << " dw " << dw << " fc->rw " << fc->effectiveWidth() << " fc->width() " << fc->width();
        if( rightmostPos > dw )
            dw = rightmostPos;
        rightmostPos = rightmostAbsolutePosition();
        if ( rightmostPos > dw )
            dw = rightmostPos;
        if( dw > w )
            w = dw;
    }

    RenderLayer *layer = m_layer;
    w = qMax( w, layer->xPos() + layer->width() );
// qDebug() << "w " << w << " layer(" << layer->renderer()->renderName() << ")->width " << layer->width() << " rm " << (layer->xPos() + layer->width()) << " width() " << layer->renderer()->width() << " rw " << layer->renderer()->effectiveWidth();
    return w;
}

RenderPage* RenderCanvas::page() {
    if (!m_page) m_page = new RenderPage(this);
    return m_page;
}

void RenderCanvas::updateInvalidatedFonts()
{
    for ( RenderObject* o = firstChild(); o ; o = o->nextRenderer() ) {
        if (o->style()->htmlFont().isInvalidated()) {
            o->setNeedsLayoutAndMinMaxRecalc();
//          qDebug() << "updating object using invalid font" << o->style()->htmlFont().getFontDef().family << o->information();
        }
    }
}
