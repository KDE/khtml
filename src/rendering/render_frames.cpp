/**
 * This file is part of the KDE project.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Simon Hausmann <hausmann@kde.org>
 *           (C) 2000 Stefan Schimanski (1Stein@gmx.de)
 *           (C) 2003 Apple Computer, Inc.
 *           (C) 2005 Niels Leenheer <niels.leenheer@gmail.com>
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

#include "rendering/render_frames.h"
#include "rendering/render_canvas.h"
#include "html/html_baseimpl.h"
#include "html/html_objectimpl.h"
#include "html/htmltokenizer.h"
#include "xml/dom2_eventsimpl.h"
#include "xml/dom_docimpl.h"
#include "khtmlview.h"
#include "khtml_part.h"

#include <QDebug>
#include <QPainter>
#include <QCursor>
#include <QApplication>

using namespace khtml;
using namespace DOM;

RenderFrameSet::RenderFrameSet( HTMLFrameSetElementImpl *frameSet)
    : RenderBox(frameSet)
{
  // init RenderObject attributes
    setInline(false);

  for (int k = 0; k < 2; ++k) {
      m_gridLen[k] = -1;
      m_gridDelta[k] = 0;
      m_gridLayout[k] = 0;
  }

  m_resizing = m_clientresizing= false;

  m_cursor = Qt::ArrowCursor;

  m_hSplit = -1;
  m_vSplit = -1;

  m_hSplitVar = 0;
  m_vSplitVar = 0;
}

RenderFrameSet::~RenderFrameSet()
{
    for (int k = 0; k < 2; ++k) {
        delete [] m_gridLayout[k];
        delete [] m_gridDelta[k];
    }
    delete [] m_hSplitVar;
    delete [] m_vSplitVar;
}

bool RenderFrameSet::nodeAtPoint(NodeInfo& info, int _x, int _y, int _tx, int _ty, HitTestAction hitTestAction,  bool inBox)
{
    RenderBox::nodeAtPoint(info, _x, _y, _tx, _ty, hitTestAction, inBox);

    bool inside = m_resizing || canResize(_x, _y);

    if ( inside && element() && !element()->noResize() && !info.readonly()) {
        info.setInnerNode(element());
        info.setInnerNonSharedNode(element());
    }

    return inside || m_clientresizing;
}

void RenderFrameSet::layout( )
{
    KHTMLAssert( needsLayout() );
    KHTMLAssert( minMaxKnown() );

    if ( !parent()->isFrameSet() ) {
        KHTMLView* view = canvas()->view();
        m_width = view ? view->visibleWidth() : 0;
        m_height = view ? view->visibleHeight() : 0;
    }

#ifdef DEBUG_LAYOUT
    // qDebug() << renderName() << "(FrameSet)::layout( ) width=" << width() << ", height=" << height();
#endif

    int remainingLen[2];
    remainingLen[1] = m_width - (element()->totalCols()-1)*element()->border();
    if(remainingLen[1]<0) remainingLen[1]=0;
    remainingLen[0] = m_height - (element()->totalRows()-1)*element()->border();
    if(remainingLen[0]<0) remainingLen[0]=0;

    int availableLen[2];
    availableLen[0] = remainingLen[0];
    availableLen[1] = remainingLen[1];

    if (m_gridLen[0] != element()->totalRows() || m_gridLen[1] != element()->totalCols()) {
        // number of rows or cols changed
        // need to zero out the deltas
        m_gridLen[0] = element()->totalRows();
        m_gridLen[1] = element()->totalCols();
        for (int k = 0; k < 2; ++k) {
            delete [] m_gridDelta[k];
            m_gridDelta[k] = new int[m_gridLen[k]];
            delete [] m_gridLayout[k];
            m_gridLayout[k] = new int[m_gridLen[k]];
            for (int i = 0; i < m_gridLen[k]; ++i)
                m_gridDelta[k][i] = 0;
        }
    }

    for (int k = 0; k < 2; ++k) {
        int totalRelative = 0;
        int totalFixed = 0;
        int totalPercent = 0;
        int countRelative = 0;
        int countFixed = 0;
        int countPercent = 0;
        int gridLen = m_gridLen[k];
        int* gridDelta = m_gridDelta[k];
        khtml::Length* grid =  k ? element()->m_cols : element()->m_rows;
        int* gridLayout = m_gridLayout[k];

        if (grid) {
            // First we need to investigate how many columns of each type we have and
            // how much space these columns are going to require.
            for (int i = 0; i < gridLen; ++i) {
                // Count the total length of all of the fixed columns/rows -> totalFixed
                // Count the number of columns/rows which are fixed -> countFixed
                if (grid[i].isFixed()) {
                    gridLayout[i] = qMax(grid[i].value(), 0);
                    totalFixed += gridLayout[i];
                    countFixed++;
                }

                // Count the total percentage of all of the percentage columns/rows -> totalPercent
                // Count the number of columns/rows which are percentages -> countPercent
                if (grid[i].isPercent()) {
                    gridLayout[i] = qMax(grid[i].width(availableLen[k]), 0);
                    totalPercent += gridLayout[i];
                    countPercent++;
                }

                // Count the total relative of all the relative columns/rows -> totalRelative
                // Count the number of columns/rows which are relative -> countRelative
                if (grid[i].isRelative()) {
                    totalRelative += qMax(grid[i].value(), 1);
                    countRelative++;
                }
            }

            // Fixed columns/rows are our first priority. If there is not enough space to fit all fixed
            // columns/rows we need to proportionally adjust their size.
            if (totalFixed > remainingLen[k]) {
                int remainingFixed = remainingLen[k];

                for (int i = 0; i < gridLen; ++i) {
                    if (grid[i].isFixed()) {
                        gridLayout[i] = (gridLayout[i] * remainingFixed) / totalFixed;
                        remainingLen[k] -= gridLayout[i];
                    }
                }
            } else {
                remainingLen[k] -= totalFixed;
            }

            // Percentage columns/rows are our second priority. Divide the remaining space proportionally
            // over all percentage columns/rows. IMPORTANT: the size of each column/row is not relative
            // to 100%, but to the total percentage. For example, if there are three columns, each of 75%,
            // and the available space is 300px, each column will become 100px in width.
            if (totalPercent > remainingLen[k]) {
                int remainingPercent = remainingLen[k];

                for (int i = 0; i < gridLen; ++i) {
                    if (grid[i].isPercent()) {
                        gridLayout[i] = (gridLayout[i] * remainingPercent) / totalPercent;
                        remainingLen[k] -= gridLayout[i];
                    }
                }
            } else {
                remainingLen[k] -= totalPercent;
            }

            // Relative columns/rows are our last priority. Divide the remaining space proportionally
            // over all relative columns/rows. IMPORTANT: the relative value of 0* is treated as 1*.
            if (countRelative) {
                int lastRelative = 0;
                int remainingRelative = remainingLen[k];

                for (int i = 0; i < gridLen; ++i) {
                    if (grid[i].isRelative()) {
                        gridLayout[i] = (qMax(grid[i].value(), 1) * remainingRelative) / totalRelative;
                        remainingLen[k] -= gridLayout[i];
                        lastRelative = i;
                    }
                }

                // If we could not evently distribute the available space of all of the relative
                // columns/rows, the remainder will be added to the last column/row.
                // For example: if we have a space of 100px and three columns (*,*,*), the remainder will
                // be 1px and will be added to the last column: 33px, 33px, 34px.
                if (remainingLen[k]) {
                    gridLayout[lastRelative] += remainingLen[k];
                    remainingLen[k] = 0;
                }
            }

            // If we still have some left over space we need to divide it over the already existing
            // columns/rows
            if (remainingLen[k]) {
                // Our first priority is to spread if over the percentage columns. The remaining
                // space is spread evenly, for example: if we have a space of 100px, the columns
                // definition of 25%,25% used to result in two columns of 25px. After this the
                // columns will each be 50px in width.
                if (countPercent && totalPercent) {
                    int remainingPercent = remainingLen[k];
                    int changePercent = 0;

                    for (int i = 0; i < gridLen; ++i) {
                        if (grid[i].isPercent()) {
                            changePercent = (remainingPercent * gridLayout[i]) / totalPercent;
                            gridLayout[i] += changePercent;
                            remainingLen[k] -= changePercent;
                        }
                    }
                } else if (totalFixed) {
                    // Our last priority is to spread the remaining space over the fixed columns.
                    // For example if we have 100px of space and two column of each 40px, both
                    // columns will become exactly 50px.
                    int remainingFixed = remainingLen[k];
                    int changeFixed = 0;

                    for (int i = 0; i < gridLen; ++i) {
                        if (grid[i].isFixed()) {
                            changeFixed = (remainingFixed * gridLayout[i]) / totalFixed;
                            gridLayout[i] += changeFixed;
                            remainingLen[k] -= changeFixed;
                        }
                    }
                }
            }

            // If we still have some left over space we probably ended up with a remainder of
            // a division. We can not spread it evenly anymore. If we have any percentage
            // columns/rows simply spread the remainder equally over all available percentage columns,
            // regardless of their size.
            if (remainingLen[k] && countPercent) {
                int remainingPercent = remainingLen[k];
                int changePercent = 0;

                for (int i = 0; i < gridLen; ++i) {
                    if (grid[i].isPercent()) {
                        changePercent = remainingPercent / countPercent;
                        gridLayout[i] += changePercent;
                        remainingLen[k] -= changePercent;
                    }
                }
            }

            // If we don't have any percentage columns/rows we only have fixed columns. Spread
            // the remainder equally over all fixed columns/rows.
            else if (remainingLen[k] && countFixed) {
                int remainingFixed = remainingLen[k];
                int changeFixed = 0;

                for (int i = 0; i < gridLen; ++i) {
                    if (grid[i].isFixed()) {
                        changeFixed = remainingFixed / countFixed;
                        gridLayout[i] += changeFixed;
                        remainingLen[k] -= changeFixed;
                    }
                }
            }

            // Still some left over... simply add it to the last column, because it is impossible
            // spread it evenly or equally.
            if (remainingLen[k]) {
                gridLayout[gridLen - 1] += remainingLen[k];
            }

            // now we have the final layout, distribute the delta over it
            bool worked = true;
            for (int i = 0; i < gridLen; ++i) {
                if (gridLayout[i] && gridLayout[i] + gridDelta[i] <= 0)
                    worked = false;
                gridLayout[i] += gridDelta[i];
            }
            // now the delta's broke something, undo it and reset deltas
            if (!worked)
                for (int i = 0; i < gridLen; ++i) {
                    gridLayout[i] -= gridDelta[i];
                    gridDelta[i] = 0;
                }
        }
        else
            gridLayout[0] = remainingLen[k];
    }

    positionFrames();

    RenderObject *child = firstChild();
    if ( !child )
      goto end2;

    if(!m_hSplitVar && !m_vSplitVar)
    {
#ifdef DEBUG_LAYOUT
        // qDebug() << "calculationg fixed Splitters";
#endif
        if(!m_vSplitVar && element()->totalCols() > 1)
        {
            m_vSplitVar = new bool[element()->totalCols()];
            for(int i = 0; i < element()->totalCols(); i++) m_vSplitVar[i] = true;
        }
        if(!m_hSplitVar && element()->totalRows() > 1)
        {
            m_hSplitVar = new bool[element()->totalRows()];
            for(int i = 0; i < element()->totalRows(); i++) m_hSplitVar[i] = true;
        }

        for(int r = 0; r < element()->totalRows(); r++)
        {
            for(int c = 0; c < element()->totalCols(); c++)
            {
                bool fixed = false;

                if ( child->isFrameSet() )
                  fixed = static_cast<RenderFrameSet *>(child)->element()->noResize();
                else
                  fixed = static_cast<RenderFrame *>(child)->element()->noResize();

                if(fixed)
                {
#ifdef DEBUG_LAYOUT
                    // qDebug() << "found fixed cell " << r << "/" << c << "!";
#endif
                    if( element()->totalCols() > 1)
                    {
                        if(c>0) m_vSplitVar[c-1] = false;
                        m_vSplitVar[c] = false;
                    }
                    if( element()->totalRows() > 1)
                    {
                        if(r>0) m_hSplitVar[r-1] = false;
                        m_hSplitVar[r] = false;
                    }
                    child = child->nextSibling();
                    if(!child) goto end2;
                }
#ifdef DEBUG_LAYOUT
                else
                    // qDebug() << "not fixed: " << r << "/" << c << "!";
#endif
            }
        }

    }
    RenderContainer::layout();
 end2:
    setNeedsLayout(false);
}

void RenderFrameSet::positionFrames()
{
  int r;
  int c;

  RenderObject *child = firstChild();
  if ( !child )
    return;

  //  NodeImpl *child = _first;
  //  if(!child) return;

  int yPos = 0;

  for(r = 0; r < element()->totalRows(); r++)
  {
    int xPos = 0;
    for(c = 0; c < element()->totalCols(); c++)
    {
      child->setPos( xPos, yPos );
#ifdef DEBUG_LAYOUT
      // qDebug() << "child frame at (" << xPos << "/" << yPos << ") size (" << m_gridLayout[1][c] << "/" << m_gridLayout[0][r] << ")";
#endif
      // has to be resized and itself resize its contents
      if ((m_gridLayout[1][c] != child->width()) || (m_gridLayout[0][r] != child->height())) {
          child->setWidth( m_gridLayout[1][c] );
          child->setHeight( m_gridLayout[0][r] );
          child->setNeedsLayout(true);
          child->layout();
      }

      xPos += m_gridLayout[1][c] + element()->border();
      child = child->nextSibling();

      if ( !child )
        return;

    }

    yPos += m_gridLayout[0][r] + element()->border();
  }

  // all the remaining frames are hidden to avoid ugly
  // spurious unflowed frames
  while ( child ) {
      child->setWidth( 0 );
      child->setHeight( 0 );
      child->setNeedsLayout(false);

      child = child->nextSibling();
  }
}

bool RenderFrameSet::userResize( MouseEventImpl *evt )
{
  if (needsLayout()) return false;

  bool res = false;
  int _x = evt->clientX();
  int _y = evt->clientY();

  if ( ( !m_resizing && evt->id() == EventImpl::MOUSEMOVE_EVENT ) || evt->id() == EventImpl::MOUSEDOWN_EVENT )
  {
#ifdef DEBUG_LAYOUT
    // qDebug() << "mouseEvent:check";
#endif

    m_hSplit = -1;
    m_vSplit = -1;

    // check if we're over a horizontal or vertical boundary
    int pos = m_gridLayout[1][0] + xPos();
    for(int c = 1; c < element()->totalCols(); c++)
    {
      if(_x >= pos && _x <= pos+element()->border())
      {
        if(m_vSplitVar && m_vSplitVar[c-1] == true) m_vSplit = c-1;
#ifdef DEBUG_LAYOUT
        // qDebug() << "vsplit!";
#endif
        res = true;
        break;
      }
      pos += m_gridLayout[1][c] + element()->border();
    }

    pos = m_gridLayout[0][0] + yPos();
    for(int r = 1; r < element()->totalRows(); r++)
    {
      if( _y >= pos && _y <= pos+element()->border())
      {
        if(m_hSplitVar && m_hSplitVar[r-1] == true) m_hSplit = r-1;
#ifdef DEBUG_LAYOUT
        // qDebug() << "hsplitvar = " << m_hSplitVar;
        // qDebug() << "hsplit!";
#endif
        res = true;
        break;
      }
      pos += m_gridLayout[0][r] + element()->border();
    }
#ifdef DEBUG_LAYOUT
    // qDebug() << m_hSplit << "/" << m_vSplit;
#endif
  }

  m_cursor = Qt::ArrowCursor;
  if(m_hSplit != -1 && m_vSplit != -1)
      m_cursor = Qt::SizeAllCursor;
  else if( m_vSplit != -1 )
      m_cursor = Qt::SizeHorCursor;
  else if( m_hSplit != -1 )
      m_cursor = Qt::SizeVerCursor;

  if(!m_resizing && evt->id() == EventImpl::MOUSEDOWN_EVENT)
  {
      setResizing(true);
      QApplication::setOverrideCursor(QCursor(m_cursor));
      m_vSplitPos = _x;
      m_hSplitPos = _y;
      m_oldpos = -1;
  }

  // ### check the resize is not going out of bounds.
  if(m_resizing)
  {
    if (evt->id() == EventImpl::MOUSEUP_EVENT) {
        setResizing(false);
        QApplication::restoreOverrideCursor();
    }

    if(m_vSplit != -1 )
    {
#ifdef DEBUG_LAYOUT
      // qDebug() << "split xpos=" << _x;
#endif
      int delta = m_vSplitPos - _x;
      m_gridDelta[1][m_vSplit] -= delta;
      m_gridDelta[1][m_vSplit+1] += delta;
      m_vSplitPos = _x;
    }
    if(m_hSplit != -1 )
    {
#ifdef DEBUG_LAYOUT
      // qDebug() << "split ypos=" << _y;
#endif
      int delta = m_hSplitPos - _y;
      m_gridDelta[0][m_hSplit] -= delta;
      m_gridDelta[0][m_hSplit+1] += delta;
      m_hSplitPos = _y;
    }

    // this just schedules the relayout
    // important, otherwise the moving indicator is not correctly erased
    setNeedsLayout(true);
  }

/*
  KHTMLView *view = canvas()->view();
  if ((m_resizing || evt->id() == EventImpl::MOUSEUP_EVENT) && view) {
      QPainter paint( view );
      paint.setPen( Qt::gray );
      paint.setBrush( Qt::gray );
      paint.setCompositionMode(QPainter::CompositionMode_Xor);
      QRect r(xPos(), yPos(), width(), height());
      const int rBord = 3;
      int sw = element()->border();
      int p = m_resizing ? (m_vSplit > -1 ? _x : _y) : -1;
      if (m_vSplit > -1) {
          if ( m_oldpos >= 0 )
              paint.drawRect( m_oldpos + sw/2 - rBord , r.y(),
                              2*rBord, r.height() );
          if ( p >= 0 )
              paint.drawRect( p  + sw/2 - rBord, r.y(), 2*rBord, r.height() );
      } else {
          if ( m_oldpos >= 0 )
              paint.drawRect( r.x(), m_oldpos + sw/2 - rBord,
                              r.width(), 2*rBord );
          if ( p >= 0 )
              paint.drawRect( r.x(), p + sw/2 - rBord, r.width(), 2*rBord );
      }
      m_oldpos = p;
  }
*/
  return res;
}

void RenderFrameSet::paintFrameSetRules( QPainter *paint, const QRect& damageRect )
{
  Q_UNUSED( damageRect );
  KHTMLView *view = canvas()->view();
  if (view && !noResize()) {
      paint->setPen( Qt::gray );
      paint->setBrush( Qt::gray );
      const int rBord = 3;
      int sw = element()->border();

      // ### implement me

      (void) rBord;
      (void) sw;
  }

}

void RenderFrameSet::setResizing(bool e)
{
      m_resizing = e;
      for (RenderObject* p = parent(); p; p = p->parent())
          if (p->isFrameSet()) static_cast<RenderFrameSet*>(p)->m_clientresizing = m_resizing;
}

bool RenderFrameSet::canResize( int _x, int _y )
{
    // if we haven't received a layout, then the gridLayout doesn't contain useful data yet
    if (needsLayout() || !m_gridLayout[0] || !m_gridLayout[1] ) return false;

    // check if we're over a horizontal or vertical boundary
    int pos = m_gridLayout[1][0];
    for(int c = 1; c < element()->totalCols(); c++)
        if(_x >= pos && _x <= pos+element()->border())
            return true;

    pos = m_gridLayout[0][0];
    for(int r = 1; r < element()->totalRows(); r++)
        if( _y >= pos && _y <= pos+element()->border())
            return true;

    return false;
}

#ifdef ENABLE_DUMP
void RenderFrameSet::dump(QTextStream &stream, const QString &ind) const
{
    RenderBox::dump(stream,ind);
    stream << " totalrows=" << element()->totalRows();
    stream << " totalcols=" << element()->totalCols();

    if ( m_hSplitVar )
        for (uint i = 0; i < (uint)element()->totalRows(); i++) {
            stream << " hSplitvar(" << i << ")=" << m_hSplitVar[i];
        }

    if ( m_vSplitVar )
        for (uint i = 0; i < (uint)element()->totalCols(); i++)
            stream << " vSplitvar(" << i << ")=" << m_vSplitVar[i];
}
#endif

/**************************************************************************************/

RenderPart::RenderPart(DOM::HTMLElementImpl* node)
    : RenderWidget(node)
{
    // init RenderObject attributes
    setInline(false);

    // HTMLPartContainerElementImpl does memory management, not us
    setDoesNotOwnWidget();
}

void RenderPart::setWidget( QWidget *widget )
{
#ifdef DEBUG_LAYOUT
    // qDebug() << "RenderPart::setWidget()";
#endif

    setQWidget( widget );
    if (widget) {
        widget->setFocusPolicy(Qt::WheelFocus);
        if(widget->inherits("KHTMLView"))
            connect( widget, SIGNAL(cleared()), this, SLOT(slotViewCleared()) );
    }

    setNeedsLayoutAndMinMaxRecalc();

    // make sure the scrollbars are set correctly for restore
    // ### find better fix
    slotViewCleared();
}

short RenderPart::intrinsicWidth() const
{
    return 300;
}

int RenderPart::intrinsicHeight() const
{
    return 150;
}

void RenderPart::slotViewCleared()
{
}

/***************************************************************************************/

RenderFrame::RenderFrame( DOM::HTMLFrameElementImpl *frame )
    : RenderPart(frame)
{
    setInline( false );
}

void RenderFrame::slotViewCleared()
{
    if (QScrollArea *view = qobject_cast<QScrollArea*>(m_widget)) {
#ifdef DEBUG_LAYOUT
        // qDebug() << "frame is a scrollarea!";
#endif
        if(!element()->frameBorder || !((static_cast<HTMLFrameSetElementImpl *>(element()->parentNode()))->frameBorder()))
            view->setFrameStyle(QFrame::NoFrame);
        if(KHTMLView *htmlView = qobject_cast<KHTMLView*>(view)) {
#ifdef DEBUG_LAYOUT
            // qDebug() << "frame is a KHTMLview!";
#endif
	    htmlView->setVerticalScrollBarPolicy(element()->scrolling );
	    htmlView->setHorizontalScrollBarPolicy(element()->scrolling );
            if(element()->marginWidth != -1) htmlView->setMarginWidth(element()->marginWidth);
            if(element()->marginHeight != -1) htmlView->setMarginHeight(element()->marginHeight);
        } else {
          // those are no more virtual in Qt4 ;(
    	    view->setVerticalScrollBarPolicy(element()->scrolling );
	    view->setHorizontalScrollBarPolicy(element()->scrolling );
        }
    }

}

/****************************************************************************************/

RenderPartObject::RenderPartObject( DOM::HTMLElementImpl* element )
    : RenderPart( element )
{
    // init RenderObject attributes
    setInline(true);
}

void RenderPartObject::layout( )
{
    KHTMLAssert( needsLayout() );
    KHTMLAssert( minMaxKnown() );

    calcWidth();
    calcHeight();

    RenderPart::layout();

    setNeedsLayout(false);
}

void RenderPartObject::slotViewCleared()
{
  if(QScrollArea *view = qobject_cast<QScrollArea*>(m_widget)) {
#ifdef DEBUG_LAYOUT
      // qDebug() << "iframe is a scrollview!";
#endif
      int frameStyle = QFrame::NoFrame;
      Qt::ScrollBarPolicy scroll = Qt::ScrollBarAsNeeded;
      int marginw = -1;
      int marginh = -1;
      if ( element()->id() == ID_IFRAME) {
	  HTMLIFrameElementImpl *frame = static_cast<HTMLIFrameElementImpl *>(element());
	  if(frame->frameBorder)
	      frameStyle = QFrame::Box;
	  scroll = frame->scrolling;
	  marginw = frame->marginWidth;
	  marginh = frame->marginHeight;
      }
      view->setFrameStyle(frameStyle);
      if (KHTMLView *htmlView = qobject_cast<KHTMLView*>(view)) {
#ifdef DEBUG_LAYOUT
          // qDebug() << "frame is a KHTMLview!";
#endif
          htmlView->setIgnoreWheelEvents( element()->id() == ID_IFRAME );
          htmlView->setVerticalScrollBarPolicy(scroll );
          htmlView->setHorizontalScrollBarPolicy(scroll );
          if(marginw != -1) htmlView->setMarginWidth(marginw);
          if(marginh != -1) htmlView->setMarginHeight(marginh);
      } else {
          // those are no more virtual in Qt4 ;(
          view->setVerticalScrollBarPolicy(scroll );
          view->setHorizontalScrollBarPolicy(scroll );
      }

  }
}

