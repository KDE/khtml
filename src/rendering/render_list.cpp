/**
 * This file is part of the HTML rendering engine for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000-2002 Dirk Mueller (mueller@kde.org)
 *           (C) 2003 Apple Computer, Inc.
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
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

#include "rendering/render_list.h"
#include "rendering/render_canvas.h"
#include "rendering/enumerate.h"
#include "rendering/counter_tree.h"
#include "html/html_listimpl.h"
#include "imload/imagepainter.h"
#include "misc/helper.h"
#include "misc/loader.h"
#include "xml/dom_docimpl.h"

#include <QDebug>

//#define BOX_DEBUG

using namespace khtml;
using namespace Enumerate;
using namespace khtmlImLoad;

const int cMarkerPadding = 7;

// -------------------------------------------------------------------------

RenderListItem::RenderListItem(DOM::NodeImpl* node)
    : RenderBlock(node)
{
    // init RenderObject attributes
    setInline(false);   // our object is not Inline

    predefVal = -1;
    m_marker = 0;
    m_counter = 0;
    m_insideList = false;
    m_deleteMarker = false;
}

void RenderListItem::setStyle(RenderStyle *_style)
{
    RenderBlock::setStyle(_style);

    RenderStyle *newStyle = new RenderStyle();
    newStyle->ref();

    newStyle->inheritFrom(style());

    if(!m_marker && style()->listStyleType() != LNONE) {
        m_marker = new (renderArena()) RenderListMarker(element());
        m_marker->setIsAnonymous( true );
        m_marker->setStyle(newStyle);
        m_marker->setListItem( this );
        m_deleteMarker = true;
    } else if ( m_marker && style()->listStyleType() == LNONE) {
        m_marker->detach();
        m_marker = 0;
    }
    else if ( m_marker ) {
        m_marker->setStyle(newStyle);
    }

    newStyle->deref();
}

void RenderListItem::detach()
{
    if ( m_marker && m_deleteMarker )
        m_marker->detach();
    RenderBlock::detach();
}

static RenderObject* getParentOfFirstLineBox(RenderBlock* curr, RenderObject* marker)
{
    RenderObject* firstChild = curr->firstChild();
    if (!firstChild)
        return 0;

    for (RenderObject* currChild = firstChild;
         currChild; currChild = currChild->nextSibling()) {
        if (currChild == marker)
            continue;

        if (currChild->isInline()&& (!currChild->isInlineFlow() || curr->inlineChildNeedsLineBox(currChild)))
            return curr;

        if (currChild->isFloating() || currChild->isPositioned())
            continue;

        if (currChild->isTable() || !currChild->isRenderBlock())
            break;

        if (curr->isListItem() && currChild->style()->htmlHacks() && currChild->element() &&
            (currChild->element()->id() == ID_UL || currChild->element()->id() == ID_OL ||
             currChild->element()->id() == ID_DIR || currChild->element()->id() == ID_MENU))
            break;

        RenderObject* lineBox = getParentOfFirstLineBox(static_cast<RenderBlock*>(currChild), marker);
        if (lineBox)
            return lineBox;
    }

    return 0;
}

static RenderObject* firstNonMarkerChild(RenderObject* parent)
{
    RenderObject* result = parent->firstChild();
    while (result && result->isListMarker())
        result = result->nextSibling();
    return result;
}

void RenderListItem::updateMarkerLocation()
{
    // Sanity check the location of our marker.
    if (m_marker) {
        RenderObject* markerPar = m_marker->parent();
        RenderObject* lineBoxParent = getParentOfFirstLineBox(this, m_marker);
        if (!lineBoxParent) {
            // If the marker is currently contained inside an anonymous box,
            // then we are the only item in that anonymous box (since no line box
            // parent was found).  It's ok to just leave the marker where it is
            // in this case.
            if (markerPar && markerPar->isAnonymousBlock())
                lineBoxParent = markerPar;
            else
                lineBoxParent = this;
        }
        if (markerPar != lineBoxParent)
        {
            if (markerPar)
                markerPar->removeChild(m_marker);
            if (!lineBoxParent)
                lineBoxParent = this;
            lineBoxParent->addChild(m_marker, firstNonMarkerChild(lineBoxParent));
            m_deleteMarker = false;
            if (!m_marker->minMaxKnown())
                m_marker->calcMinMaxWidth();
            recalcMinMaxWidths();
        }
    }
}

void RenderListItem::calcMinMaxWidth()
{
    // Make sure our marker is in the correct location.
    updateMarkerLocation();
    if (!minMaxKnown())
        RenderBlock::calcMinMaxWidth();
}
/*
short RenderListItem::marginLeft() const
{
    if (m_insideList)
        return RenderBlock::marginLeft();
    else
        return qMax(m_marker->markerWidth(), RenderBlock::marginLeft());
}

short RenderListItem::marginRight() const
{
    return RenderBlock::marginRight();
}*/

void RenderListItem::layout( )
{
    KHTMLAssert( needsLayout() );
    KHTMLAssert( minMaxKnown() );

    updateMarkerLocation();
    RenderBlock::layout();
}

// -----------------------------------------------------------

RenderListMarker::RenderListMarker(DOM::NodeImpl* node)
    : RenderBox(node), m_listImage(0), m_markerWidth(0)
{
    // init RenderObject attributes
    setInline(true);   // our object is Inline
    setReplaced(true); // pretend to be replaced
    // val = -1;
    // m_listImage = 0;
}

RenderListMarker::~RenderListMarker()
{
    if(m_listImage)
        m_listImage->deref(this);
    if (m_listItem)
        m_listItem->resetListMarker();
}

void RenderListMarker::setStyle(RenderStyle *s)
{
    if ( style() && (s->listStylePosition() != style()->listStylePosition()  || s->listStyleType() != style()->listStyleType()) )
        setNeedsLayoutAndMinMaxRecalc();

    RenderBox::setStyle(s);

    if ( m_listImage != style()->listStyleImage() ) {
	if(m_listImage)  m_listImage->deref(this);
	m_listImage = style()->listStyleImage();
	if(m_listImage)  m_listImage->ref(this);
    }
}


void RenderListMarker::paint(PaintInfo& paintInfo, int _tx, int _ty)
{
    if (paintInfo.phase != PaintActionForeground)
        return;

    if (style()->visibility() != VISIBLE)  return;

    _tx += m_x;
    _ty += m_y;

    if((_ty > paintInfo.r.bottom()) || (_ty + m_height <= paintInfo.r.top()))
        return;

    if(shouldPaintBackgroundOrBorder())
        paintBoxDecorations(paintInfo, _tx, _ty);

    QPainter* p = paintInfo.p;
#ifdef DEBUG_LAYOUT
    qDebug() << nodeName().string() << "(ListMarker)::paintObject(" << _tx << ", " << _ty << ")";
#endif
    p->setFont(style()->font());
    const QFontMetrics fm = p->fontMetrics();


    // The marker needs to adjust its tx, for the case where it's an outside marker.
    RenderObject* listItem = 0;
    int leftLineOffset = 0;
    int rightLineOffset = 0;
    if (!listPositionInside()) {
        listItem = this;
        int yOffset = 0;
        int xOffset = 0;
        while (listItem && listItem != m_listItem) {
            yOffset += listItem->yPos();
            xOffset += listItem->xPos();
            listItem = listItem->parent();
        }

        // Now that we have our xoffset within the listbox, we need to adjust ourselves by the delta
        // between our current xoffset and our desired position (which is just outside the border box
        // of the list item).
        if (style()->direction() == LTR) {
            leftLineOffset = m_listItem->leftRelOffset(yOffset, m_listItem->leftOffset(yOffset));
            _tx -= (xOffset - leftLineOffset) + m_listItem->paddingLeft() + m_listItem->borderLeft();
        }
        else {
            rightLineOffset = m_listItem->rightRelOffset(yOffset, m_listItem->rightOffset(yOffset));
            _tx += (rightLineOffset-xOffset) + m_listItem->paddingRight() + m_listItem->borderRight();
        }
    }

    int offset = fm.ascent()*2/3;
    bool haveImage = m_listImage && !m_listImage->isErrorImage();
    if (haveImage)
        offset = m_listImage->pixmap_size().width();

    int xoff = 0;
    int yoff = fm.ascent() - offset;

    int bulletWidth = offset/2;
    if (offset%2)
        bulletWidth++;
    if (!listPositionInside()) {
        if (listItem && listItem->style()->direction() == LTR)
            xoff = -cMarkerPadding - offset;
        else
            xoff = cMarkerPadding + (haveImage ? 0 : (offset - bulletWidth));
    }
    else if (style()->direction() == RTL)
        xoff += haveImage ? cMarkerPadding : (m_width - bulletWidth);

    if ( m_listImage && !m_listImage->isErrorImage()) {
        ImagePainter painter(m_listImage->image());
        painter.paint( _tx + xoff, _ty, p );
        return;
    }

#ifdef BOX_DEBUG
    p->setPen( Qt::red );
    p->drawRect( _tx + xoff, _ty + yoff, offset, offset );
#endif

    const QColor color( style()->color() );
    p->setPen( color );

    switch(style()->listStyleType()) {
    case LDISC:
        p->setBrush( color );
        p->drawEllipse( _tx + xoff, _ty + (3 * yoff)/2, (offset>>1), (offset>>1) );
        return;
    case LCIRCLE:
        p->setBrush( Qt::NoBrush );
        p->drawEllipse( _tx + xoff, _ty + (3 * yoff)/2, (offset>>1), (offset>>1) );
        return;
    case LSQUARE:
        p->setBrush( color );
        p->drawRect( _tx + xoff, _ty + (3 * yoff)/2, (offset>>1), (offset>>1) );
        return;
    case LBOX:
        p->setBrush( Qt::NoBrush );
        p->drawRect( _tx + xoff, _ty + (3 * yoff)/2, (offset>>1), (offset>>1) );
        return;
    case LDIAMOND: {
        static QPolygon diamond(4);
        int x = _tx + xoff;
        int y = _ty + (3 * yoff)/2 - 1;
        int s = (offset>>2)+1;
        diamond[0] = QPoint(x+s,   y);
        diamond[1] = QPoint(x+2*s, y+s);
        diamond[2] = QPoint(x+s,   y+2*s);
        diamond[3] = QPoint(x,     y+s);
        p->setBrush( color );

        p->drawConvexPolygon( diamond.constData(), 4 );
        return;
    }
    case LNONE:
        return;
    default:
        if (!m_item.isEmpty()) {
            if(listPositionInside()) {
                //BEGIN HACK
#ifdef __GNUC__
  #warning "KDE4: hack for testregression, remove when main branch"
#endif
                _tx += qMax(-fm.minLeftBearing(), 0);
                //END HACK
            	if( style()->direction() == LTR) {
                    p->drawText(_tx, _ty, 0, 0, Qt::AlignLeft|Qt::TextDontClip, m_item);
                    p->drawText(_tx + fm.width(m_item), _ty, 0, 0, Qt::AlignLeft|Qt::TextDontClip,
                                QLatin1String(". "));
                }
            	else {
                    const QString& punct(QLatin1String(" ."));
                    p->drawText(_tx, _ty, 0, 0, Qt::AlignLeft|Qt::TextDontClip, punct);
            	    p->drawText(_tx + fm.width(punct), _ty, 0, 0, Qt::AlignLeft|Qt::TextDontClip, m_item);
                }
            } else {
                if (style()->direction() == LTR) {
                    const QString& punct(QLatin1String(". "));
                    int itemWidth = fm.width(m_item);
                    int punctWidth = fm.width(punct);
                    p->drawText(_tx-offset/2-punctWidth, _ty, 0, 0, Qt::AlignLeft|Qt::TextDontClip, punct);
                    p->drawText(_tx-offset/2-punctWidth-itemWidth, _ty, 0, 0, Qt::AlignLeft|Qt::TextDontClip, m_item);
                }
            	else {
                //BEGIN HACK
#ifdef __GNUC__
  #warning "KDE4: hack for testregression, remove when main branch"
#endif
                _tx += qMax(-fm.minLeftBearing(), 0);
                //END HACK

                    const QString& punct(QLatin1String(" ."));
            	    p->drawText(_tx+offset/2, _ty, 0, 0, Qt::AlignLeft|Qt::TextDontClip, punct);
                    p->drawText(_tx+offset/2+fm.width(punct), _ty, 0, 0, Qt::AlignLeft|Qt::TextDontClip, m_item);
                }
	    }
        }
    }
}

void RenderListMarker::layout()
{
    KHTMLAssert( needsLayout() );

    if ( !minMaxKnown() )
        calcMinMaxWidth();

    setNeedsLayout(false);
}

void RenderListMarker::updatePixmap( const QRect& r, CachedImage *o)
{
    if(o != m_listImage) {
        RenderBox::updatePixmap(r, o);
        return;
    }

    if(m_width != m_listImage->pixmap_size().width() || m_height != m_listImage->pixmap_size().height())
        setNeedsLayoutAndMinMaxRecalc();
    else
        repaintRectangle(0, 0, m_width, m_height);
}

void RenderListMarker::calcMinMaxWidth()
{
    KHTMLAssert( !minMaxKnown() );

    m_markerWidth = m_width = 0;

    if(m_listImage && !m_listImage->isErrorImage()) {
        m_markerWidth = m_listImage->pixmap_size().width() + cMarkerPadding;
        if (listPositionInside())
            m_width = m_markerWidth;
        m_height = m_listImage->pixmap_size().height();
        m_minWidth = m_maxWidth = m_width;
        setMinMaxKnown();
        return;
    }

    const QFontMetrics &fm = style()->fontMetrics();
    m_height = fm.ascent();

    // Skip uncounted elements
    switch(style()->listStyleType()) {
    // Glyphs:
        case LDISC:
        case LCIRCLE:
        case LSQUARE:
        case LBOX:
        case LDIAMOND:
            m_markerWidth = fm.ascent();
        	goto end;
        default:
            break;
    }

    { // variable scope
    CounterNode *counter = m_listItem->m_counter;
    if (!counter) {
        counter = m_listItem->getCounter("list-item", true);
        counter->setRenderer(this);
        m_listItem->m_counter = counter;
    }


    assert(counter);
    int value = counter->count();
    if (counter->isReset()) value = counter->value();
    int total = value;
    if (counter->parent()) total = counter->parent()->total();

    switch(style()->listStyleType())
    {
// Numeric:
    case LDECIMAL:
        m_item.setNum ( value );
        break;
    case DECIMAL_LEADING_ZERO: {
        int decimals = 2;
        int t = total/100;
        while (t>0) {
            t = t/10;
            decimals++;
        }
        decimals = qMax(decimals, 2);
        QString num = QString::number(value);
        m_item.fill('0',decimals-num.length());
        m_item.append(num);
        break;
    }
    case ARABIC_INDIC:
        m_item = toArabicIndic( value );
        break;
    case LAO:
        m_item = toLao( value );
        break;
    case PERSIAN:
    case URDU:
        m_item = toPersianUrdu( value );
        break;
    case THAI:
        m_item = toThai( value );
        break;
    case TIBETAN:
        m_item = toTibetan( value );
        break;
// Algoritmic:
    case LOWER_ROMAN:
        m_item = toRoman( value, false );
        break;
    case UPPER_ROMAN:
        m_item = toRoman( value, true );
        break;
    case HEBREW:
        m_item = toHebrew( value );
        break;
    case ARMENIAN:
        m_item = toArmenian( value );
        break;
    case GEORGIAN:
        m_item = toGeorgian( value );
        break;
// Alphabetic:
    case LOWER_ALPHA:
    case LOWER_LATIN:
        m_item = toLowerLatin( value );
        break;
    case UPPER_ALPHA:
    case UPPER_LATIN:
        m_item = toUpperLatin( value );
        break;
    case LOWER_GREEK:
        m_item = toLowerGreek( value );
        break;
    case UPPER_GREEK:
        m_item = toUpperGreek( value );
        break;
    case HIRAGANA:
        m_item = toHiragana( value );
        break;
    case HIRAGANA_IROHA:
        m_item = toHiraganaIroha( value );
        break;
    case KATAKANA:
        m_item = toKatakana( value );
        break;
    case KATAKANA_IROHA:
        m_item = toKatakanaIroha( value );
        break;
// Ideographic:
    case JAPANESE_FORMAL:
        m_item = toJapaneseFormal( value );
        break;
    case JAPANESE_INFORMAL:
        m_item = toJapaneseInformal( value );
        break;
    case SIMP_CHINESE_FORMAL:
        m_item = toSimpChineseFormal( value );
        break;
    case SIMP_CHINESE_INFORMAL:
        m_item = toSimpChineseInformal( value );
        break;
    case TRAD_CHINESE_FORMAL:
        m_item = toTradChineseFormal( value );
        break;
    case CJK_IDEOGRAPHIC:
        // CSS 3 List says treat as trad-chinese-informal
    case TRAD_CHINESE_INFORMAL:
        m_item = toTradChineseInformal( value );
        break;
// special:
    case LNONE:
        break;
    default:
        KHTMLAssert(false);
    }
    m_markerWidth = fm.width(m_item) + fm.width(QLatin1String(". "));
    }

end:
    if(listPositionInside())
        m_width = m_markerWidth;

    m_minWidth = m_width;
    m_maxWidth = m_width;

    setMinMaxKnown();
}

short RenderListMarker::lineHeight(bool /*b*/) const
{
    return height();
}

short RenderListMarker::baselinePosition(bool /*b*/) const
{
    return height();
}

void RenderListMarker::calcWidth()
{
    RenderBox::calcWidth();
}

/*
int CounterListItem::recount() const
{
    static_cast<RenderListItem*>(m_renderer)->m_marker->setNeedsLayoutAndMinMaxRecalc();
}

void CounterListItem::setSelfDirty()
{

}*/


#undef BOX_DEBUG
