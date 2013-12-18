/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2003 Apple Computer, Inc.
 *           (C) 2005,2011 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2008 Germain Garand (germain@ebooksfrance.org)
 *           (C) 2009 Carlos Licea (carlos.licea@kdemail.net)
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

//#define TABLE_DEBUG
//#define TABLE_PRINT
//#define DEBUG_LAYOUT
//#define BOX_DEBUG
#include "rendering/render_table.h"
#include "rendering/render_replaced.h"
#include "rendering/render_canvas.h"
#include "rendering/table_layout.h"
#include "html/html_tableimpl.h"
#include "html/html_formimpl.h"
#include "rendering/render_line.h"
#include "xml/dom_docimpl.h"

#include <QApplication>
#include <QStyle>

#include <QDebug>
#include <assert.h>

using namespace khtml;
using namespace DOM;

RenderTable::RenderTable(DOM::NodeImpl* node)
    : RenderBlock(node)
{

    tCaption = 0;
    head = foot = firstBody = 0;
    tableLayout = 0;
    m_currentBorder = 0;

    has_col_elems = false;
    hspacing = vspacing = 0;
    padding = 0;
    needSectionRecalc = false;
    padding = 0;

    columnPos.resize( 2 );
    columnPos.fill( 0 );
    columns.resize( 1 );
    columns.fill( ColumnStruct() );

    columnPos[0] = 0;
}

RenderTable::~RenderTable()
{
    delete tableLayout;
}

void RenderTable::setStyle(RenderStyle *_style)
{
    ETableLayout oldTableLayout = style() ? style()->tableLayout() : TAUTO;
    if ( _style->display() == INLINE ) _style->setDisplay( INLINE_TABLE );
    if ( _style->display() != INLINE_TABLE ) _style->setDisplay(TABLE);
    if ( !_style->flowAroundFloats() ) _style->setFlowAroundFloats(true);
    RenderBlock::setStyle(_style);

    // init RenderObject attributes
    setInline(style()->display()==INLINE_TABLE && !isPositioned());
    setReplaced(style()->display()==INLINE_TABLE);

    // In the collapsed border model, there is no cell spacing.
    hspacing = collapseBorders() ? 0 : style()->borderHorizontalSpacing();
    vspacing = collapseBorders() ? 0 : style()->borderVerticalSpacing();
    columnPos[0] = hspacing;

    if ( !tableLayout || style()->tableLayout() != oldTableLayout ) {
	delete tableLayout;

        // According to the CSS2 spec, you only use fixed table layout if an
        // explicit width is specified on the table.  Auto width implies auto table layout.
	if (style()->tableLayout() == TFIXED && !style()->width().isAuto()) {
	    tableLayout = new FixedTableLayout(this);
#ifdef DEBUG_LAYOUT
	    qDebug() << "using fixed table layout";
#endif
	} else
	    tableLayout = new AutoTableLayout(this);
    }
}

short RenderTable::lineHeight(bool b) const
{
    // Inline tables are replaced elements. Otherwise, just pass off to
    // the base class.
    if (isReplaced())
        return height()+marginTop()+marginBottom();
    return RenderBlock::lineHeight(b);
}

short RenderTable::baselinePosition(bool b) const
{
    // CSS2.1 - 10.8.1 The baseline of an 'inline-table' is the baseline of the first row of the table.
    if (isReplaced() && !needsLayout()) {
        // compatibility with Gecko: only apply to generic containers, not to HTML Table.
        if (element() && element()->id() == ID_TABLE)
            return height()+marginTop()+marginBottom();

        if (firstBodySection() && firstBodySection()->grid.size())
            return firstBodySection()->grid[0].baseLine + firstBodySection()->yPos() + marginTop();
        return 0;
    }
    return RenderBox::baselinePosition(b);
}

static inline void resetSectionPointerIfNotBefore(RenderTableSection*& ptr, RenderObject* before)
{
    if (!before || !ptr)
        return;
    RenderObject* o = before->previousSibling();
    while (o && o != ptr)
        o = o->previousSibling();
    if (!o)
        ptr = 0;
}

void RenderTable::addChild(RenderObject *child, RenderObject *beforeChild)
{
#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << "(Table)::addChild( " << child->renderName() << ", " <<
                       (beforeChild ? beforeChild->renderName() : "0") << " )" << endl;
#endif
    bool wrapInAnonymousSection = false;

    switch(child->style()->display())
    {
        case TABLE_CAPTION:
            if (child->isRenderBlock()) {
                // First caption wins.
                if (beforeChild && tCaption) {
                    RenderObject* o = beforeChild->previousSibling();
                    while (o && o != tCaption)
                        o = o->previousSibling();
                    if (!o)
                        tCaption = 0;
                }
                if (!tCaption)
                    tCaption = static_cast<RenderBlock*>(child);
            }
            break;
        case TABLE_COLUMN:
        case TABLE_COLUMN_GROUP:
            has_col_elems = true;
            break;
        case TABLE_HEADER_GROUP:
            if (child->isTableSection()) {
                resetSectionPointerIfNotBefore(head, beforeChild);
                if (!head) {
                    head = static_cast<RenderTableSection *>(child);
                } else {
                    resetSectionPointerIfNotBefore(firstBody, beforeChild);
                    if (!firstBody)
                        firstBody = static_cast<RenderTableSection*>(child);
                }
            }
            break;
        case TABLE_FOOTER_GROUP:
            if (child->isTableSection()) {
                resetSectionPointerIfNotBefore(foot, beforeChild);
                if (!foot) {
                    foot = static_cast<RenderTableSection *>(child);
                    break;
                }
            }
            // fall through
        case TABLE_ROW_GROUP:
            if (child->isTableSection()) {
                resetSectionPointerIfNotBefore(firstBody, beforeChild);
                if (!firstBody)
                    firstBody = static_cast<RenderTableSection*>(child);
            }
            break;
        case TABLE_CELL:
        case TABLE_ROW:
            wrapInAnonymousSection = true;
            break;
        case BLOCK:
//         case BOX:
        case COMPACT:
        case INLINE:
        case INLINE_BLOCK:
//         case INLINE_BOX:
        case INLINE_TABLE:
        case LIST_ITEM:
        case NONE:
        case RUN_IN:
        case TABLE:
            // The special TABLE > FORM quirk allows the form to sit directly under the table
            if (child->element() && child->element()->isHTMLElement() && child->element()->id() == ID_FORM)
                wrapInAnonymousSection = !static_cast<HTMLFormElementImpl*>(child->element())->isMalformed();
            else
                wrapInAnonymousSection = true;
            break;
    }

    if (!wrapInAnonymousSection) {
        RenderContainer::addChild(child, beforeChild);
        return;
    }

    if (!beforeChild && lastChild() && lastChild()->isTableSection() && lastChild()->isAnonymous()) {
        lastChild()->addChild(child);
        return;
    }

    RenderObject *lastBox = beforeChild;
    while (lastBox && lastBox->parent()->isAnonymous() &&
            !lastBox->isTableSection() && lastBox->style()->display() != TABLE_CAPTION) {
        lastBox = lastBox->parent();
    }
    if (lastBox && lastBox->isAnonymous()) {
        lastBox->addChild(child, beforeChild);
        return;
    }

    if (beforeChild && !beforeChild->isTableSection())
        beforeChild = 0;
    RenderTableSection* section = new (renderArena()) RenderTableSection(document() /* anonymous */);
    RenderStyle* newStyle = new RenderStyle();
    newStyle->inheritFrom(style());
    newStyle->setDisplay(TABLE_ROW_GROUP);
    section->setStyle(newStyle);
    addChild(section, beforeChild);
    section->addChild(child);
}

void RenderTable::calcWidth()
{
    if ( isPositioned() ) {
        calcAbsoluteHorizontal();
    }

    RenderBlock *cb = containingBlock();
    int availableWidth = cb->lineWidth( m_y );

    LengthType widthType = style()->width().type();
    if(widthType > Relative && style()->width().isPositive()) {
        // Percent or fixed table
        // Percent is calculated from contentWidth, not available width
        m_width = calcBoxWidth(style()->width().minWidth( cb->contentWidth() ));
    } else {
        // Subtract out any fixed margins from our available width for auto width tables.
        int marginTotal = 0;
        if (!style()->marginLeft().isAuto())
            marginTotal += style()->marginLeft().width(availableWidth);
        if (!style()->marginRight().isAuto())
            marginTotal += style()->marginRight().width(availableWidth);

        // Subtract out our margins to get the available content width.
        int availContentWidth = qMax(0, availableWidth - marginTotal);

        // Ensure we aren't bigger than our max width or smaller than our min width.
        m_width = qMin(availContentWidth, m_maxWidth);
    }

    m_width = qMax (m_width, m_minWidth);

    // Finally, with our true width determined, compute our margins for real.
    m_marginRight=0;
    m_marginLeft=0;

    calcHorizontalMargins(style()->marginLeft(),style()->marginRight(),availableWidth);
}

QList< QRectF > RenderTable::getClientRects()
{
    RenderFlow* cap = caption();
    if (cap) {
        // tables with caption report y&height inclusive caption, but we need them
        // exclusive and a extra rect for the caption
        // NOTE: first table, then caption
        QList<QRectF> list;

        int x = 0;
        int y = 0;
        absolutePosition(x, y);

        QRectF tableRect(x, y + cap->height(), width(), height() - cap->height());
        list.append(clientRectToViewport(tableRect));

        QRectF captionRect(x, y, cap->width(), cap->height());
        list.append(clientRectToViewport(captionRect));

        return list;
    } else {
        return RenderObject::getClientRects();
    }
}

void RenderTable::layout()
{
    KHTMLAssert( needsLayout() );
    KHTMLAssert( minMaxKnown() );
    KHTMLAssert( !needSectionRecalc );

    if (markedForRepaint()) {
        repaintDuringLayout();
        setMarkedForRepaint(false);
    }

    if (posChildNeedsLayout() && !normalChildNeedsLayout() && !selfNeedsLayout()) {
        // All we have to is lay out our positioned objects.
        layoutPositionedObjects(true);
        setNeedsLayout(false);
        return;
    }

    m_height = 0;
    initMaxMarginValues();

    int oldWidth = m_width;
    calcWidth();
    m_overflowWidth = m_width;

    if (tCaption && (oldWidth != m_width || tCaption->style()->height().isPercent()))
        tCaption->setChildNeedsLayout(true);

    // the optimization below doesn't work since the internal table
    // layout could have changed.  we need to add a flag to the table
    // layout that tells us if something has changed in the min max
    // calculations to do it correctly.
//     if ( oldWidth != m_width || columns.size() + 1 != columnPos.size() )
    tableLayout->layout();

#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << "(Table)::layout1() width=" << width() << ", marginLeft=" << marginLeft() << " marginRight=" << marginRight();
#endif

    setCellWidths();

    // layout child objects
    int calculatedHeight = 0;

    RenderObject *child = firstChild();
    while( child ) {
        // FIXME: What about a form that has a display value that makes it a table section?
	if ( child->needsLayout() && !(child->element() && child->element()->id() == ID_FORM) )
	    child->layout();
	if ( child->isTableSection() ) {
	    static_cast<RenderTableSection *>(child)->calcRowHeight();
	    calculatedHeight += static_cast<RenderTableSection *>(child)->layoutRows( 0 );
	}
	child = child->nextSibling();
    }

    // ### collapse caption margin
    if(tCaption && tCaption->style()->captionSide() != CAPBOTTOM) {
        tCaption->setPos(tCaption->marginLeft(), tCaption->marginTop()+m_height);
        m_height += tCaption->height() + tCaption->marginTop() + tCaption->marginBottom();
    }

    int bpTop = borderTop() + (collapseBorders() ? 0 : paddingTop());
    int bpBottom = borderBottom() + (collapseBorders() ? 0 : paddingBottom());

    m_height += bpTop;

    int oldHeight = m_height;
    if (isPositioned())
        m_height += calculatedHeight + bpBottom;
    calcHeight();
    int newHeight = m_height;
    m_height = oldHeight;

    Length h = style()->height();
    int th = -(bpTop + bpBottom); // Tables size as though CSS height includes border/padding.
    if (isPositioned())
        th += newHeight;
    else if (h.isFixed())
        th += h.value();
    else if (h.isPercent())
        th += calcPercentageHeight(h);

    // layout rows
    if ( th > calculatedHeight ) {
	// we have to redistribute that height to get the constraint correctly
	// just force the first body to the height needed
	// ### FIXME This should take height constraints on all table sections into account and distribute
	// accordingly. For now this should be good enough
        if (firstBody) {
            firstBody->calcRowHeight();
            firstBody->layoutRows( th - calculatedHeight );
        }
        else if (!style()->htmlHacks()) {
            // Completely empty tables (with no sections or anything) should at least honor specified height
            // in strict mode.
            m_height += th;
        }
    }

    int bl = borderLeft();
    if (!collapseBorders())
        bl += paddingLeft();

    // position the table sections
    RenderTableSection* section = head ? head : (firstBody ? firstBody : foot);
    while (section) {
        section->setPos(bl, m_height);

        m_height += section->height();
        m_overflowLeft = qMin(m_overflowLeft, section->effectiveXPos());
        m_overflowWidth = qMax(m_overflowWidth, section->effectiveXPos() + section->effectiveWidth());
        section = sectionBelow(section);
    }

    m_height += bpBottom;

    if(tCaption && tCaption->style()->captionSide()==CAPBOTTOM) {
        tCaption->setPos(tCaption->marginLeft(), tCaption->marginTop()+m_height);
        m_height += tCaption->height() + tCaption->marginTop() + tCaption->marginBottom();
    }

    if (canvas()->pagedMode()) {
        RenderObject *child = firstChild();
        // relayout taking real position into account
        while( child ) {
            if ( !(child->element() && child->element()->id() == ID_FORM) ) {
                child->setNeedsLayout(true);
                child->layout();
                if (child->containsPageBreak()) setContainsPageBreak(true);
                if (child->needsPageClear()) setNeedsPageClear(true);
            }
            child = child->nextSibling();
        }
    }

    //qDebug() << "table height: " << m_height;

    // table can be containing block of positioned elements.
    // ### only pass true if width or height changed.
    layoutPositionedObjects( true );

    m_overflowHeight = m_height;

    setNeedsLayout(false);
}

void RenderTable::setCellWidths()
{
#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << "(Table, this=0x" << this << ")::setCellWidths()";
#endif

    RenderObject *child = firstChild();
    while( child ) {
	if ( child->isTableSection() )
	    static_cast<RenderTableSection *>(child)->setCellWidths();
	child = child->nextSibling();
    }
}

void RenderTable::paint( PaintInfo& pI, int _tx, int _ty)
{
    if(needsLayout()) return;

    _tx += xPos();
    _ty += yPos();

#ifdef TABLE_PRINT
    qDebug() << "RenderTable::paint() w/h = (" << width() << "/" << height() << ")";
#endif
    if (!isRelPositioned() && !isPositioned())
    {
        int os = 2*maximalOutlineSize(pI.phase);
        if((_ty > pI.r.y() + pI.r.height() + os) || (_ty + height() < pI.r.y() - os)) return;
        if((_tx > pI.r.x() + pI.r.width() + os) || (_tx + width() < pI.r.x() - os)) return;
    }

#ifdef TABLE_PRINT
    qDebug() << "RenderTable::paint(2) " << _tx << "/" << _ty << " (" << _y << "/" << _h << ")";
#endif

    if (pI.phase == PaintActionOutline)
        paintOutline(pI.p, _tx, _ty, width(), height(), style());

    if(( pI.phase == PaintActionElementBackground || pI.phase == PaintActionChildBackground )
       && shouldPaintBackgroundOrBorder() && style()->visibility() == VISIBLE)
	paintBoxDecorations(pI, _tx, _ty);

    if ( pI.phase == PaintActionElementBackground )
        return;

    PaintAction oldphase = pI.phase;
    if ( pI.phase == PaintActionChildBackgrounds )
        pI.phase = PaintActionChildBackground;

    for( RenderObject *child = firstChild(); child; child = child->nextSibling())
	if ( child->isTableSection() || child == tCaption )
	    child->paint( pI, _tx, _ty );

    if (collapseBorders() &&
        (pI.phase == PaintActionElementBackground || pI.phase == PaintActionChildBackground)
        && style()->visibility() == VISIBLE) {
        // Collect all the unique border styles that we want to paint in a sorted list.  Once we
        // have all the styles sorted, we then do individual passes, painting each style of border
        // from lowest precedence to highest precedence.
        pI.phase = PaintActionCollapsedTableBorders;
        QList<CollapsedBorderValue> borderStyles;
        collectBorders(borderStyles);
#if 0
       QString m;
       for (uint i = 0; i < borderStyles.count(); i++)
         m += QString("%1 ").arg((*borderStyles.at(i)).width());
       // qDebug() << m;
#endif
        QList<CollapsedBorderValue>::Iterator it = borderStyles.begin();
        QList<CollapsedBorderValue>::Iterator end = borderStyles.end();
        for (; it != end; ++it) {
            m_currentBorder = &*it;
            for (RenderObject *child = firstChild(); child; child = child->nextSibling()) {
                if (child->isTableSection())
                    child->paint(pI, _tx, _ty);
            }
        }
        m_currentBorder = 0;
    }

    pI.phase = oldphase;
#ifdef BOX_DEBUG
    outlineBox(pI.p, _tx, _ty, "blue");
#endif
}

void RenderTable::paintBoxDecorations(PaintInfo &pI, int _tx, int _ty)
{
    int w = width();
    int h = height();

    // Account for the caption.
    if (tCaption) {
        int captionHeight = (tCaption->height() + tCaption->marginBottom() +  tCaption->marginTop());
        h -= captionHeight;
        if (tCaption->style()->captionSide() != CAPBOTTOM)
            _ty += captionHeight;
    }
    QRect cr;
    cr.setX(qMax(_tx, pI.r.x()));
    cr.setY(qMax(_ty, pI.r.y()));
    int mw, mh;
    if (_ty<pI.r.y())
        mh= qMax(0,h-(pI.r.y()-_ty));
    else
        mh = qMin(pI.r.height(),h);
    if (_tx<pI.r.x())
        mw = qMax(0,w-(pI.r.x()-_tx));
    else
        mw = qMin(pI.r.width(),w);
    cr.setWidth(mw);
    cr.setHeight(mh);

    paintOneBackground(pI.p, style()->backgroundColor(), style()->backgroundLayers(), cr, _tx, _ty, w, h);

    if (style()->hasBorder() && !collapseBorders())
        paintBorder(pI.p, _tx, _ty, w, h, style());
}

void RenderTable::calcMinMaxWidth()
{
    KHTMLAssert( !minMaxKnown() );

    if ( needSectionRecalc )
	recalcSections();

#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << "(Table " << this << ")::calcMinMaxWidth()";
#endif

    tableLayout->calcMinMaxWidth();

    if (tCaption) {
        tCaption->calcWidth();
        if (tCaption->marginLeft()+tCaption->marginRight()+tCaption->minWidth() > m_minWidth)
            m_minWidth = tCaption->marginLeft()+tCaption->marginRight()+tCaption->minWidth();
    }

    setMinMaxKnown();
#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << " END: (Table " << this << ")::calcMinMaxWidth() min = " << m_minWidth << " max = " << m_maxWidth;
#endif
}

void RenderTable::close()
{
//    qDebug() << "RenderTable::close()";
    setNeedsLayoutAndMinMaxRecalc();
}

void RenderTable::splitColumn( int pos, int firstSpan )
{
    // we need to add a new columnStruct
    int oldSize = columns.size();
    columns.resize( oldSize + 1 );
    int oldSpan = columns[pos].span;
//     qDebug("splitColumn( %d,%d ), oldSize=%d, oldSpan=%d", pos, firstSpan, oldSize, oldSpan );
    KHTMLAssert( oldSpan > firstSpan );
    columns[pos].span = firstSpan;
    memmove( columns.data()+pos+1, columns.data()+pos, (oldSize-pos)*sizeof(ColumnStruct) );
    columns[pos+1].span = oldSpan - firstSpan;

    // change width of all rows.
    RenderObject *child = firstChild();
    while ( child ) {
	if ( child->isTableSection() ) {
	    RenderTableSection *section = static_cast<RenderTableSection *>(child);
	    int size = section->grid.size();
	    int row = 0;
	    if ( section->cCol > pos )
		section->cCol++;
	    while ( row < size ) {
		section->grid[row].row->resize( oldSize+1 );
		RenderTableSection::Row &r = *section->grid[row].row;
		memmove( r.data()+pos+1, r.data()+pos, (oldSize-pos)*sizeof( RenderTableCell * ) );
// 		qDebug("moving from %d to %d, num=%d", pos, pos+1, (oldSize-pos-1) );
		r[pos+1] = r[pos] ? (RenderTableCell *)-1 : 0;
		row++;
	    }
	}
	child = child->nextSibling();
    }
    columnPos.resize( numEffCols()+1 );
    setNeedsLayoutAndMinMaxRecalc();
}

void RenderTable::appendColumn( int span )
{
    // easy case.
    int pos = columns.size();
//     qDebug("appendColumn( %d ), size=%d", span, pos );
    int newSize = pos + 1;
    columns.resize( newSize );
    columns[pos].span = span;
    //qDebug("appending column at %d, span %d", pos,  span );

    // change width of all rows.
    RenderObject *child = firstChild();
    while ( child ) {
	if ( child->isTableSection() ) {
	    RenderTableSection *section = static_cast<RenderTableSection *>(child);
	    int size = section->grid.size();
	    int row = 0;
	    while ( row < size ) {
		section->grid[row].row->resize( newSize );
		section->cellAt( row, pos ) = 0;
		row++;
	    }

	}
	child = child->nextSibling();
    }
    columnPos.resize( numEffCols()+1 );
    setNeedsLayoutAndMinMaxRecalc();
}

RenderTableCol *RenderTable::colElement( int col, bool* startEdge, bool* endEdge) const
{
    if ( !has_col_elems )
	return 0;
    RenderObject *child = firstChild();
    int cCol = 0;
    while ( child ) {
	if ( child->isTableCol() ) {
	    RenderTableCol *colElem = static_cast<RenderTableCol *>(child);
	    int span = colElem->span();
	    if ( !colElem->firstChild() ) {
                int startCol = cCol;
                int endCol = cCol + span - 1;
		cCol += span;
		if ( cCol > col ) {
                    if (startEdge)
                        *startEdge = startCol == col;
                    if (endEdge)
                        *endEdge = endCol == col;
		    return colElem;
                }
	    }

	    RenderObject *next = child->firstChild();
	    if ( !next )
		next = child->nextSibling();
	    if ( !next && child->parent()->isTableCol() )
		next = child->parent()->nextSibling();
	    child = next;
	} else if (child == tCaption) {
	    child = child->nextSibling();
        } else
	    break;
    }
    return 0;
}

void RenderTable::recalcSections()
{
    tCaption = 0;
    head = foot = firstBody = 0;
    has_col_elems = false;

    RenderObject *child = firstChild();
    // We need to get valid pointers to caption, head, foot and firstbody again
    while ( child ) {
	switch(child->style()->display()) {
	case TABLE_CAPTION:
	    if ( !tCaption && child->isRenderBlock() ) {
		tCaption = static_cast<RenderBlock*>(child);
                tCaption->setNeedsLayout(true);
            }
	    break;
	case TABLE_COLUMN:
	case TABLE_COLUMN_GROUP:
	    has_col_elems = true;
	    break;
        case TABLE_HEADER_GROUP:
            if (child->isTableSection()) {
                RenderTableSection *section = static_cast<RenderTableSection *>(child);
                if (!head)
                    head = section;
                else if (!firstBody)
                    firstBody = section;
                if (section->needCellRecalc)
                    section->recalcCells();
            }
            break;
        case TABLE_FOOTER_GROUP:
            if (child->isTableSection()) {
                RenderTableSection *section = static_cast<RenderTableSection *>(child);
                if (!foot)
                    foot = section;
                else if (!firstBody)
                    firstBody = section;
                if (section->needCellRecalc)
                    section->recalcCells();
            }
            break;
        case TABLE_ROW_GROUP:
            if (child->isTableSection()) {
                RenderTableSection *section = static_cast<RenderTableSection *>(child);
                if (!firstBody)
                    firstBody = section;
                if (section->needCellRecalc)
                    section->recalcCells();
            }
            break;
	default:
	    break;
	}
	child = child->nextSibling();
    }

    int maxCols = 0;
    for (RenderObject *child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableSection()) {
            RenderTableSection *section = static_cast<RenderTableSection *>(child);
            int sectionCols = section->numColumns();
            if (sectionCols > maxCols)
                maxCols = sectionCols;
        }
    }

    columns.resize(maxCols);
    columnPos.resize(maxCols+1);

    needSectionRecalc = false;
    setNeedsLayout(true);
}

RenderObject* RenderTable::removeChildNode(RenderObject* child)
{
    setNeedSectionRecalc();
    return RenderContainer::removeChildNode( child );
}

int RenderTable::borderLeft() const
{
    if (collapseBorders()) {
        // FIXME: For strict mode, returning 0 is correct, since the table border half spills into the margin,
        // but I'm working to get this changed.  For now, follow the spec.
        return 0;
    }
    return RenderBlock::borderLeft();
}

int RenderTable::borderRight() const
{
    if (collapseBorders()) {
        // FIXME: For strict mode, returning 0 is correct, since the table border half spills into the margin,
        // but I'm working to get this changed.  For now, follow the spec.
        return 0;
    }
    return RenderBlock::borderRight();
}

int RenderTable::borderTop() const
{
    if (collapseBorders()) {
        // FIXME: For strict mode, returning 0 is correct, since the table border half spills into the margin,
        // but I'm working to get this changed.  For now, follow the spec.
        return 0;
    }
    return RenderBlock::borderTop();
}

int RenderTable::borderBottom() const
{
    if (collapseBorders()) {
        // FIXME: For strict mode, returning 0 is correct, since the table border half spills into the margin,
        // but I'm working to get this changed.  For now, follow the spec.
        return 0;
    }
    return RenderBlock::borderBottom();
}

RenderTableSection* RenderTable::sectionAbove(const RenderTableSection* section, bool skipEmptySections)
{
    if (needSectionRecalc)
        recalcSections();

    if (section == head)
        return 0;
    RenderObject *prevSection = section == foot ? lastChild() : section->previousSibling();
    while (prevSection) {
        if (prevSection->isTableSection() && prevSection != head && prevSection != foot && (!skipEmptySections || static_cast<RenderTableSection*>(prevSection)->numRows()))
            break;
        prevSection = prevSection->previousSibling();
    }
    if (!prevSection && head && (!skipEmptySections || head->numRows()))
        prevSection = head;
    return static_cast<RenderTableSection*>(prevSection);
}

RenderTableSection* RenderTable::sectionBelow(const RenderTableSection* section, bool skipEmptySections)
{
    if (needSectionRecalc)
        recalcSections();

    if (section == foot)
        return 0;
    RenderObject *nextSection = section == head ? firstChild() : section->nextSibling();
    while (nextSection) {
        if (nextSection->isTableSection() && nextSection != head && nextSection != foot && (!skipEmptySections || static_cast<RenderTableSection*>(nextSection)->numRows()))
            break;
        nextSection = nextSection->nextSibling();
    }
    if (!nextSection && foot && (!skipEmptySections || foot->numRows()))
        nextSection = foot;
    return static_cast<RenderTableSection*>(nextSection);
}

RenderTableCell* RenderTable::cellAbove(const RenderTableCell* cell)
{
    if (needSectionRecalc)
        recalcSections();

    // Find the section and row to look in
    int r = cell->row();
    RenderTableSection* section = 0;
    int rAbove = 0;
    if (r > 0) {
        // cell is not in the first row, so use the above row in its own section
        section = cell->section();
        rAbove = r-1;
    } else {
        section = sectionAbove(cell->section(), true);
        if (section)
            rAbove = section->numRows() - 1;
    }

    // Look up the cell in the section's grid, which requires effective col index
    if (section) {
        int effCol = colToEffCol(cell->col());
        RenderTableCell* aboveCell;
        // If we hit a span back up to a real cell.
        do {
            aboveCell = section->cellAt(rAbove, effCol);
            effCol--;
        } while (aboveCell == (RenderTableCell *)-1 && effCol >=0);
        return (aboveCell == (RenderTableCell *)-1) ? 0 : aboveCell;
    } else {
        return 0;
    }
}

RenderTableCell* RenderTable::cellBelow(const RenderTableCell* cell)
{
    if (needSectionRecalc)
        recalcSections();

    // Find the section and row to look in
    int r = cell->row() + cell->rowSpan() - 1;
    RenderTableSection* section = 0;
    int rBelow = 0;
    if (r < cell->section()->numRows()-1) {
        // The cell is not in the last row, so use the next row in the section.
        section = cell->section();
        rBelow= r+1;
    } else {
        section = sectionBelow(cell->section(), true);
        if (section)
            rBelow = 0;
    }

    // Look up the cell in the section's grid, which requires effective col index
    if (section) {
        int effCol = colToEffCol(cell->col());
        RenderTableCell* belowCell;
        // If we hit a colspan back up to a real cell.
        do {
            belowCell = section->cellAt(rBelow, effCol);
            effCol--;
        } while (belowCell == (RenderTableCell *)-1 && effCol >=0);
        return (belowCell == (RenderTableCell *)-1) ? 0 : belowCell;
    } else {
        return 0;
    }
}

RenderTableCell* RenderTable::cellBefore(const RenderTableCell* cell)
{
    if (needSectionRecalc)
        recalcSections();

    RenderTableSection* section = cell->section();
    int effCol = colToEffCol(cell->col());
    if (effCol == 0)
        return 0;

    // If we hit a colspan back up to a real cell.
    RenderTableCell* prevCell;
    do {
        prevCell = section->cellAt(cell->row(), effCol-1);
        effCol--;
    } while (prevCell == (RenderTableCell *)-1 && effCol >=0);
    return (prevCell == (RenderTableCell *)-1) ? 0 : prevCell;
}

RenderTableCell* RenderTable::cellAfter(const RenderTableCell* cell)
{
    if (needSectionRecalc)
        recalcSections();

    int effCol = colToEffCol(cell->col()+cell->colSpan());
    if (effCol >= numEffCols())
        return 0;
    RenderTableCell* result = cell->section()->cellAt(cell->row(), effCol);
    return (result == (RenderTableCell*)-1) ? 0 : result;
}

#ifdef ENABLE_DUMP
void RenderTable::dump(QTextStream &stream, const QString &ind) const
{
    RenderBlock::dump(stream, ind);

    if (tCaption)
	stream << " tCaption";
    if (head)
	stream << " head";
    if (foot)
	stream << " foot";

    stream << " [cspans:";
    for ( int i = 0; i < columns.size(); i++ )
	stream << " " << columns[i].span;
    stream << "]";
}

#endif

FindSelectionResult RenderTable::checkSelectionPoint( int _x, int _y, int _tx, int _ty, DOM::NodeImpl*& node, int & offset, SelPointState &state )
{
    int off = offset;
    DOM::NodeImpl* nod = node;

    FindSelectionResult pos;
    TableSectionIterator it(this);
    for (; *it; ++it) {
        pos = (*it)->checkSelectionPoint(_x, _y, _tx + m_x, _ty + m_y, nod, off, state);
        switch(pos) {
        case SelectionPointBeforeInLine:
        case SelectionPointInside:
            //qDebug() << "RenderTable::checkSelectionPoint " << this << " returning SelectionPointInside offset=" << offset;
            node = nod;
            offset = off;
            return SelectionPointInside;
        case SelectionPointBefore:
            //x,y is before this element -> stop here
            if ( state.m_lastNode ) {
                node = state.m_lastNode;
                offset = state.m_lastOffset;
                //qDebug() << "RenderTable::checkSelectionPoint " << this << " before this child "
                //              << node << "-> returning SelectionPointInside, offset=" << offset << endl;
                return SelectionPointInside;
            } else {
                node = nod;
                offset = off;
                //qDebug() << "RenderTable::checkSelectionPoint " << this << " before us -> returning SelectionPointBefore " << node << "/" << offset;
                return SelectionPointBefore;
            }
            break;
        case SelectionPointAfter:
	    if (state.m_afterInLine) break;
	    // fall through
        case SelectionPointAfterInLine:
	    if (pos == SelectionPointAfterInLine) state.m_afterInLine = true;
            //qDebug() << "RenderTable::checkSelectionPoint: selection after: " << nod << " offset: " << off << " afterInLine: " << state.m_afterInLine;
            state.m_lastNode = nod;
            state.m_lastOffset = off;
            // No "return" here, obviously. We must keep looking into the children.
            break;
        }
    }
    // If we are after the last child, return lastNode/lastOffset
    // But lastNode can be 0L if there is no child, for instance.
    if ( state.m_lastNode )
    {
        node = state.m_lastNode;
        offset = state.m_lastOffset;
    }
    // Fallback
    return SelectionPointAfter;
}

// --------------------------------------------------------------------------

RenderTableSection::RenderTableSection(DOM::NodeImpl* node)
    : RenderBox(node)
{
    // init RenderObject attributes
    setInline(false);   // our object is not Inline
    cCol = 0;
    cRow = -1;
    needCellRecalc = false;
}

RenderTableSection::~RenderTableSection()
{
    clearGrid();
}

void RenderTableSection::detach()
{
    // recalc cell info because RenderTable has unguarded pointers
    // stored that point to this RenderTableSection.
    if (table())
        table()->setNeedSectionRecalc();

    RenderBox::detach();
}

void RenderTableSection::setStyle(RenderStyle* _style)
{
    // we don't allow changing this one
    if (style())
        _style->setDisplay(style()->display());
    else if (_style->display() != TABLE_FOOTER_GROUP && _style->display() != TABLE_HEADER_GROUP)
        _style->setDisplay(TABLE_ROW_GROUP);

    RenderBox::setStyle(_style);
}

void RenderTableSection::addChild(RenderObject *child, RenderObject *beforeChild)
{
#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << "(TableSection)::addChild( " << child->renderName()  << ", beforeChild=" <<
                       (beforeChild ? beforeChild->renderName() : "0") << " )" << endl;
#endif
    if ( !child->isTableRow() ) {
        // TBODY > FORM quirk (???)
        if (child->element() && child->element()->isHTMLElement() && child->element()->id() == ID_FORM &&
            static_cast<HTMLFormElementImpl*>(child->element())->isMalformed())
        {
            RenderContainer::addChild(child, beforeChild);
            return;
        }

        RenderObject* last = beforeChild;
        if (!last)
            last = lastChild();
        if (last && last->isAnonymous()) {
            last->addChild(child);
            return;
        }

        // If beforeChild is inside an anonymous cell/row, insert into the cell or into
        // the anonymous row containing it, if there is one.
        RenderObject* lastBox = last;
        while (lastBox && lastBox->parent()->isAnonymous() && !lastBox->isTableRow())
            lastBox = lastBox->parent();
        if (lastBox && lastBox->isAnonymous()) {
            lastBox->addChild(child, beforeChild);
            return;
        }

        RenderObject* row = new (renderArena()) RenderTableRow(document() /* anonymous table */);
        RenderStyle* newStyle = new RenderStyle();
        newStyle->inheritFrom(style());
        newStyle->setDisplay(TABLE_ROW);
        row->setStyle(newStyle);
        addChild(row, beforeChild);
        row->addChild(child);
        return;
    }

    if (beforeChild)
	setNeedCellRecalc();

    cRow++;
    cCol = 0;

    ensureRows( cRow+1 );
    KHTMLAssert( child->isTableRow() );
    grid[cRow].rowRenderer = static_cast<RenderTableRow*>(child);

    if (!beforeChild) {
        grid[cRow].height = child->style()->height();
        if ( grid[cRow].height.isRelative() )
            grid[cRow].height = Length();
    }


    RenderContainer::addChild(child,beforeChild);
}

void RenderTableSection::ensureRows( int numRows )
{
    int nRows = grid.size();
    int nCols = table()->numEffCols();
    if ( numRows > nRows ) {
	grid.resize( numRows );
	for ( int r = nRows; r < numRows; r++ ) {
	    grid[r].row = new Row( nCols );
	    grid[r].row->fill( 0 );
            grid[r].rowRenderer = 0;
	    grid[r].baseLine = 0;
	    grid[r].height = Length();
	}
    }

}

void RenderTableSection::addCell( RenderTableCell *cell, RenderTableRow *row )
{
    int rSpan = cell->rowSpan();
    int cSpan = cell->colSpan();
    const QVector<RenderTable::ColumnStruct> &columns = table()->columns;
    int nCols = columns.size();

    // ### mozilla still seems to do the old HTML way, even for strict DTD
    // (see the annotation on table cell layouting in the CSS specs and the testcase below:
    // <TABLE border>
    // <TR><TD>1 <TD rowspan="2">2 <TD>3 <TD>4
    // <TR><TD colspan="2">5
    // </TABLE>
	while ( cCol < nCols && cellAt( cRow, cCol ) )
	    ++cCol;

//       qDebug("adding cell at %d/%d span=(%d/%d)",  cRow, cCol, rSpan, cSpan );

    if ( rSpan == 1 ) {
	// we ignore height settings on rowspan cells
	Length height = cell->style()->height();
	if ( height.isPositive() || (height.isRelative() && (height.isPositive() || height.isZero()) )) {
	    Length cRowHeight = grid[cRow].height;
	    switch( height.type() ) {
	    case Percent:
		if ( !cRowHeight.isPercent() ||
		     (cRowHeight.isPercent() && cRowHeight.rawValue() < height.rawValue() ) )
		    grid[cRow].height = height;
		break;
	    case Fixed:
		if ( cRowHeight.type() < Percent ||
		     ( cRowHeight.isFixed() && cRowHeight.value() < height.value() ) )
		    grid[cRow].height = height;
		break;
	    case Relative:
#if 0
		// we treat this as variable. This is correct according to HTML4, as it only specifies length for the height.
		if ( cRowHeight.type() == Auto ||
		     ( cRowHeight.type() == Relative && cRowHeight.value() < height.value() ) )
		     grid[cRow].height = height;
		     break;
#endif
	    default:
		break;
	    }
	}
    }

    //What:Postion the cell
    //How: we take care of special case when colspan or rowspan equals 0
    //setting an initial "guess" of a colspan and rowspan, look at
    //http://www.w3.org/TR/html401/struct/tables.html for details on cells with span = 0
    //after that position the cell normally, we do it to tell the cell where it is
    //and tell other cells where they can't be located (marking the cells as -1),
    //later taking the span into account (and in other function) the cell is
    //then painted (that's why we need to set the colspan and rowspan properly
    //when any of them is zero, so that the cell is properly painted)

    // make sure we have enough rows
    ensureRows( cRow + (rSpan? rSpan : 1 ) );

    grid[cRow].rowRenderer = row;

    int col = cCol;
    // tell the cell where it is
    RenderTableCell *set = cell;
//     qDebug()<<"row"<<cRow<<"col"<<cCol;
//     qDebug()<<"cSpan"<<cSpan<<"rSpan"<<rSpan;

    //check whether we need to update any of the cells with span = 0
    QList< int > columnsToAvoid;
    if( !cellsWithColSpanZero.isEmpty() ) {
        //Update any column which its last span update was in a previous column
        int lowestCol = cellsWithColSpanZero.lowerBound( 0 ).key();
        if( lowestCol < cCol ) {
            //add the columns for this cell
            if ( cCol >= nCols ) {
                table()->appendColumn( cSpan );
                nCols = columns.size();
            }
            //check and update all the cells that are updated to a previous column
            while( lowestCol < nCols ) {
                while( RenderTableCell* cell = cellsWithColSpanZero.take( lowestCol ) ) {
                    const int cellRow = cell->row();
                    //const int cellRowSpan = cell->rowSpan();
                    int finalSpan = cell->colSpan();
                    for( int i = lowestCol; i < nCols; ++i ) {
                        if( !cellAt( cellRow, i ) ) {
                            cellAt( cellRow, i ) = (RenderTableCell *)-1;
                        }
                        ++finalSpan;
                    }
                    cell->setColSpan( finalSpan );
                    cellsWithColSpanZero.insertMulti( nCols, cell );
                }
                lowestCol = cellsWithColSpanZero.lowerBound( 0 ).key();
            }
        }
    }

    if( !cellsWithRowSpanZero.isEmpty() ) {
        if( cellsWithRowSpanZero.contains( cRow ) ) {
            //No need to check if we have enough columns, we already found the first cell 
            //when rowspan="0", and as such, we've already inserted it
            while( RenderTableCell* cell = cellsWithRowSpanZero.take( cRow ) ) {
                const int cellCol = cell->col();
                const int finalCol = cellCol + cell->colSpan() - 1;
                RenderTableCell* set = cell;
                for( int i = cellCol; i <= finalCol; ++i ) {
                    if( !cellAt( cRow, i ) ) {
                        cellAt( cRow, i ) = set;
                    }
                    set = (RenderTableCell *)-1;
                    columnsToAvoid << i;
                }
                cell->setRowSpan( cell->rowSpan() + 1 );
                //mark it to be inserted in next row
                cellsWithRowSpanZero.insertMulti( cRow + 1, cell );
            }
        }
    }

    //Save the column if the its span is 0
    if( !cSpan ) {
        //Check if we only span in the current colgroup
        if( RenderTableCol* colgroup = table()->colElement( cCol ) ) {
            //Calculate the correct span and then handle the cell normally

            int firstColumnOfColgroup = cCol;
            while( --firstColumnOfColgroup >= 0 && colgroup == table()->colElement(firstColumnOfColgroup) ) ;
            ++firstColumnOfColgroup;

            int alreadyUsedSpan = 0;
            RenderTableCell* colgroupCell = cellAt( cRow, firstColumnOfColgroup + alreadyUsedSpan );
            while( firstColumnOfColgroup + alreadyUsedSpan < cCol ) {
                alreadyUsedSpan += colgroupCell->colSpan();
                colgroupCell = cellAt( cRow, firstColumnOfColgroup + alreadyUsedSpan );
            }

            const int finalSpan = colgroup->span() - alreadyUsedSpan;
            cell->setColSpan( finalSpan );

            //We know exactly the cSpan so we can handle the cell as a normal cell
            //unless, of course, the rowspan is also 0
            cSpan = finalSpan;
        }
        //or if we span in the whole table and mark it as inserted till
        //this column and updated whenever another column is inserted
        else {
            //We define the proper span and let the other code handle it
            const int finalSpan = nCols - cCol;
            cSpan = finalSpan;

            cell->setColSpan( finalSpan );

            cellsWithColSpanZero.insertMulti( cCol + finalSpan - 1, cell );
        }
    }

    if( !rSpan ) {
        //For now we span 1
        rSpan = 1;
        cell->setRowSpan( 1 );

        //mark it to be inserted in next row
        cellsWithRowSpanZero.insertMulti( cRow + 1, cell );
    }

    while ( cSpan ) {
        int currentSpan;
        if ( cCol >= nCols ) {
            table()->appendColumn( cSpan );
            currentSpan = cSpan;
        } else {
            if ( cSpan < columns[cCol].span ) {
                table()->splitColumn( cCol, cSpan );
            }
            currentSpan = columns[cCol].span;
        }

        while( columnsToAvoid.contains(cCol) ) 
            ++cCol;

        int r = 0;
        while ( r < rSpan ) {
            if ( !cellAt( cRow + r, cCol ) ) {
//     qDebug("    adding cell at %d, %d",  cRow + r, cCol );
                cellAt( cRow + r, cCol ) = set;
            }
            ++r;
        }
        ++cCol;
        cSpan -= currentSpan;
        set = (RenderTableCell *)-1;
    }


    if ( cell ) {
        cell->setRow( cRow );
        cell->setCol( table()->effColToCol( col ) );
    }
}

void RenderTableSection::setCellWidths()
{
#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << "(Table, this=0x" << this << ")::setCellWidths()";
#endif
    const QVector<int> &columnPos = table()->columnPos;

	int rows = grid.size();
    for ( int i = 0; i < rows; i++ ) {
	Row &row = *grid[i].row;
	int cols = row.size();
	for ( int j = 0; j < cols; j++ ) {
	    RenderTableCell *cell = row[j];
// 	    qDebug("cell[%d,%d] = %p", i, j, cell );
	    if ( !cell || cell == (RenderTableCell *)-1 )
		continue;
	    int endCol = j;
	    int cspan = cell->colSpan();
	    while ( cspan && endCol < cols ) {
		cspan -= table()->columns[endCol].span;
		endCol++;
	    }
	    int w = columnPos[endCol] - columnPos[j] - table()->borderHSpacing();
#ifdef DEBUG_LAYOUT
	    qDebug() << "setting width of cell " << cell << " " << cell->row() << "/" << cell->col() << " to " << w << " colspan=" << cell->colSpan() << " start=" << j << " end=" << endCol;
#endif
	    int oldWidth = cell->width();
	    if ( w != oldWidth ) {
		cell->setNeedsLayout(true);
		cell->setWidth( w );
	    }
	}
    }
}

void RenderTableSection::calcRowHeight()
{
    int indx;
    RenderTableCell *cell;

    int totalRows = grid.size();
    int vspacing = table()->borderVSpacing();

    rowPos.resize( totalRows + 1 );
    rowPos[0] =  vspacing + borderTop();

    for ( int r = 0; r < totalRows; r++ ) {
	rowPos[r+1] = 0;

	int baseline=0;
	int bdesc = 0;
        grid[r].baseLine = 0;
// 	qDebug("height of row %d is %d/%d", r, grid[r].height.value, grid[r].height.type );
	int ch = grid[r].height.minWidth( 0 );
	int pos = rowPos[r] + ch + (grid[r].rowRenderer ? vspacing : 0);

	if ( pos > rowPos[r+1] )
	    rowPos[r+1] = pos;

	Row *row = grid[r].row;
	int totalCols = row->size();
	int totalRows = grid.size();
	bool pagedMode = canvas()->pagedMode();

	grid[r].needFlex = false;

	for ( int c = 0; c < totalCols; c++ ) {
	    cell = cellAt(r, c);
	    if ( !cell || cell == (RenderTableCell *)-1 )
		continue;
	    if ( r < totalRows - 1 && cellAt(r+1, c) == cell )
		continue;

	    if ( ( indx = r - cell->rowSpan() + 1 ) < 0 )
		indx = 0;

            if (cell->cellPercentageHeight() != -1) {
                cell->setCellPercentageHeight(-1);
                cell->setChildNeedsLayout(true, false);
                if (cell->hasFlexedAnonymous()) {
                    for (RenderObject* o = cell->firstChild(); o ; o = o->nextSibling())
                        if (o->isAnonymousBlock())
                            o->setChildNeedsLayout(true, false);
                }
                if (pagedMode) cell->setNeedsLayout(true);
                cell->layoutIfNeeded();
            }

            ch = cell->style()->height().width(0);
            if ( cell->height() > ch)
                ch = cell->height();

            if (!cell->style()->height().isAuto())
                grid[r].needFlex = true;

	    pos = rowPos[indx] + ch + (grid[r].rowRenderer ? vspacing : 0);

	    if ( pos > rowPos[r+1] )
		rowPos[r+1] = pos;

	    // find out the baseline
	    EVerticalAlign va = cell->style()->verticalAlign();
	    if (va == BASELINE || va == TEXT_BOTTOM || va == TEXT_TOP
		|| va == SUPER || va == SUB)
	    {
		int b=cell->baselinePosition();
                if (b > cell->borderTop() + cell->paddingTop()) {
                    if (b>baseline)
                        baseline=b;

                    int td = rowPos[ indx ] + ch - b;
                    if (td>bdesc)
                        bdesc = td;
                }
	    }
	}

	//do we have baseline aligned elements?
	if (baseline) {
	    // increase rowheight if baseline requires
	    int bRowPos = baseline + bdesc + (grid[r].rowRenderer ? vspacing : 0);
	    if (rowPos[r+1]<bRowPos)
		rowPos[r+1]=bRowPos;

	    grid[r].baseLine = baseline;
	}

	if ( rowPos[r+1] < rowPos[r] )
	    rowPos[r+1] = rowPos[r];
//   	qDebug("rowpos(%d)=%d",  r, rowPos[r] );
    }
}

int RenderTableSection::layoutRows( int toAdd )
{
    int rHeight;
    int rindx;
    int totalRows = grid.size();
    int hspacing = table()->borderHSpacing();
    int vspacing = table()->borderVSpacing();

    // Set the width of our section now.  The rows will also be this width.
    m_width = table()->contentWidth();

    if (markedForRepaint()) {
        repaintDuringLayout();
        setMarkedForRepaint(false);
    }

    if (toAdd && totalRows && (rowPos[totalRows] || !nextSibling())) {

	int totalHeight = rowPos[totalRows] + toAdd;
//	qDebug("layoutRows: totalHeight = %d",  totalHeight );

        int dh = toAdd;
	int totalPercent = 0;
	int numAuto = 0;
	for ( int r = 0; r < totalRows; r++ ) {
            if ( grid[r].height.isAuto() && !emptyRow(r))
		numAuto++;
	    else if ( grid[r].height.isPercent() )
		totalPercent += grid[r].height.rawValue();
	}
	if ( totalPercent ) {
//	    qDebug("distributing %d over percent rows totalPercent=%d", dh,  totalPercent );
	    // try to satisfy percent
	    int add = 0;
	    if ( totalPercent > 100 * PERCENT_SCALE_FACTOR )
		totalPercent = 100 * PERCENT_SCALE_FACTOR;
	    int rh = rowPos[1]-rowPos[0];
	    for ( int r = 0; r < totalRows; r++ ) {
		if ( totalPercent > 0 && grid[r].height.isPercent() ) {
		    int toAdd = qMin( dh, (totalHeight * grid[r].height.rawValue() / (100 * PERCENT_SCALE_FACTOR))-rh );
		    // If toAdd is negative, then we don't want to shrink the row (this bug
                    // affected Outlook Web Access).
                    toAdd = qMax(0, toAdd);
		    add += toAdd;
		    dh -= toAdd;
		    totalPercent -= grid[r].height.rawValue();
//		    qDebug( "adding %d to row %d", toAdd, r );
		}
		if ( r < totalRows-1 )
		    rh = rowPos[r+2] - rowPos[r+1];
                rowPos[r+1] += add;
	    }
	}
	if ( numAuto ) {
	    // distribute over non-empty variable rows
//	    qDebug("distributing %d over variable rows numAuto=%d", dh,  numAuto );
	    int add = 0;
            int toAdd = dh/numAuto;
            for ( int r = 0; r < totalRows; r++ ) {
                if ( grid[r].height.isAuto() && !emptyRow(r)) {
		    add += toAdd;
		}
                rowPos[r+1] += add;
	    }
            dh -= add;
        }
        if (dh>0 && rowPos[totalRows]) {
	    // if some left overs, distribute weighted.
            int tot=rowPos[totalRows];
            int add=0;
            int prev=rowPos[0];
            for ( int r = 0; r < totalRows; r++ ) {
                //weight with the original height
                add+=dh*(rowPos[r+1]-prev)/tot;
                prev=rowPos[r+1];
                rowPos[r+1]+=add;
            }
            dh -= add;
        }
        if (dh > totalRows) {
            // distribute to tables with all empty rows
            int add=0;
            int toAdd = dh/totalRows;
            for ( int r = 0; r < totalRows; r++ ) {
                add += toAdd;
                rowPos[r+1] += add;
            }
            dh -= add;
        }
        // Finally distribute round-off values
        if (dh > 0) {
            // There is not enough for every row
            int add=0;
            for ( int r = 0; r < totalRows; r++ ) {
                if (add < dh) add++;
                rowPos[r+1] += add;
            }
            dh -= add;
        }
        assert (dh == 0);
    }

    int leftOffset = borderLeft() + hspacing;

    int nEffCols = table()->numEffCols();
    for ( int r = 0; r < totalRows; r++ )
    {
	Row *row = grid[r].row;
	int totalCols = row->size();

        // Set the row's x/y position and width/height.
        if (grid[r].rowRenderer) {
            grid[r].rowRenderer->setPos(0, rowPos[r]);
            grid[r].rowRenderer->setWidth(m_width);
            grid[r].rowRenderer->setHeight(rowPos[r+1] - rowPos[r] - vspacing);
        }

        for ( int c = 0; c < nEffCols; c++ )
        {
            RenderTableCell *cell = cellAt(r, c);
            if (!cell || cell == (RenderTableCell *)-1 )
                continue;
            if ( r < totalRows - 1 && cell == cellAt(r+1, c) )
		continue;

            if ( ( rindx = r-cell->rowSpan()+1 ) < 0 )
                rindx = 0;

            rHeight = rowPos[r+1] - rowPos[rindx] - vspacing;

            // Force percent height children to lay themselves out again.
            // This will cause, e.g., textareas to grow to
            // fill the area.  FIXME: <div>s and blocks still don't
            // work right.  We'll need to have an efficient way of
            // invalidating all percent height objects in a render subtree.
            // For now, we just handle immediate children. -dwh

            bool flexAllChildren = grid[r].needFlex || (!table()->style()->height().isAuto() && rHeight != cell->height());
            cell->setHasFlexedAnonymous(false);
            if ( flexAllChildren && flexCellChildren(cell) ) {
                cell->setCellPercentageHeight(qMax(0,
                                                   rHeight - cell->borderTop() - cell->paddingTop() -
                                                   cell->borderBottom() - cell->paddingBottom()));
                cell->layoutIfNeeded();

                // If the baseline moved, we may have to update the data for our row. Find out the new baseline.
                EVerticalAlign va = cell->style()->verticalAlign();
                if (va == BASELINE || va == TEXT_BOTTOM || va == TEXT_TOP || va == SUPER || va == SUB) {
                    int b = cell->baselinePosition();
                    if (b > cell->borderTop() + cell->paddingTop())
                        grid[r].baseLine = qMax(grid[r].baseLine, b);
                }
            }
            {
#ifdef DEBUG_LAYOUT
		qDebug() << "setting position " << r << "/" << c << ": "
				<< table()->columnPos[c] /*+ padding */ << "/" << rowPos[rindx] << " height=" << rHeight<< endl;
#endif

		EVerticalAlign va = cell->style()->verticalAlign();
		int te=0;
		switch (va)
		{
		case SUB:
		case SUPER:
		case TEXT_TOP:
		case TEXT_BOTTOM:
		case BASELINE:
		    te = getBaseline(r) - cell->baselinePosition() ;
		    break;
		case TOP:
		    te = 0;
		    break;
		case MIDDLE:
		    te = (rHeight - cell->height())/2;
		    break;
		case BOTTOM:
		    te = rHeight - cell->height();
		    break;
		default:
		    break;
		}
		te = qMax( 0, te );
#ifdef DEBUG_LAYOUT
		qDebug() << "CELL " << cell << " te=" << te << ", be=" << rHeight - cell->height() - te << ", rHeight=" << rHeight << ", valign=" << va;
#endif
		cell->setCellTopExtra( te );
		cell->setCellBottomExtra( rHeight - cell->height() - te);
	    }
            if (style()->direction()==RTL) {
                cell->setPos(
		    table()->columnPos[(int)totalCols] -
		    table()->columnPos[table()->colToEffCol(cell->col()+cell->colSpan())] +
		    leftOffset,
                    0 );
            } else {
                cell->setPos( table()->columnPos[c] + leftOffset, 0 );
	    }
        }
    }

    m_height = rowPos[totalRows];
    return m_height;
}

bool RenderTableSection::flexCellChildren(RenderObject* p) const
{
    if (!p)
        return false;
    RenderObject* o = p->firstChild();
    bool didFlex = false;
    while (o) {
        if (!o->isText() && o->style()->height().isPercent()) {
            o->setNeedsLayout(true, false);
            p->setChildNeedsLayout(true, false);
            didFlex = true;
        } else if (o->isAnonymousBlock() && flexCellChildren( o )) {
            p->setChildNeedsLayout(true, false);
            if (p->isTableCell())
                static_cast<RenderTableCell*>(p)->setHasFlexedAnonymous();
            didFlex = true;
        }
        o = o->nextSibling();
    }
    return didFlex;
}

inline static RenderTableRow *firstTableRow(RenderObject *row)
{
    while (row && !row->isTableRow())
        row = row->nextSibling();
    return static_cast<RenderTableRow *>(row);
}

inline static RenderTableRow *nextTableRow(RenderObject *row)
{
    row = row ? row->nextSibling() : row;
    while (row && !row->isTableRow())
        row = row->nextSibling();
    return static_cast<RenderTableRow *>(row);
}

int RenderTableSection::lowestPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int bottom = RenderBox::lowestPosition(includeOverflowInterior, includeSelf);
    if (!includeOverflowInterior && hasOverflowClip())
        return bottom;

    for (RenderObject *row = firstChild(); row; row = row->nextSibling()) {
        for (RenderObject *cell = row->firstChild(); cell; cell = cell->nextSibling())
            if (cell->isTableCell()) {
                int bp = row->yPos() + cell->lowestPosition(false);
                bottom = qMax(bottom, bp);
        }
    }

    return bottom;
}

int RenderTableSection::rightmostPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int right = RenderBox::rightmostPosition(includeOverflowInterior, includeSelf);
    if (!includeOverflowInterior && hasOverflowClip())
        return right;

    for (RenderObject *row = firstChild(); row; row = row->nextSibling()) {
        for (RenderObject *cell = row->firstChild(); cell; cell = cell->nextSibling())
            if (cell->isTableCell()) {
                int rp = cell->xPos() + cell->rightmostPosition(false);
                right = qMax(right, rp);
        }
    }

    return right;
}

int RenderTableSection::leftmostPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int left = RenderBox::leftmostPosition(includeOverflowInterior, includeSelf);
    if (!includeOverflowInterior && hasOverflowClip())
        return left;

    for (RenderObject *row = firstChild(); row; row = row->nextSibling()) {
        for (RenderObject *cell = row->firstChild(); cell; cell = cell->nextSibling())
            if (cell->isTableCell()) {
                int lp = cell->xPos() + cell->leftmostPosition(false);
                left = qMin(left, lp);
        }
    }

    return left;
}

int RenderTableSection::highestPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int top = RenderBox::highestPosition(includeOverflowInterior, includeSelf);
    if (!includeOverflowInterior && hasOverflowClip())
        return top;

    for (RenderObject *row = firstChild(); row; row = row->nextSibling()) {
        for (RenderObject *cell = row->firstChild(); cell; cell = cell->nextSibling())
            if (cell->isTableCell()) {
                int hp = row->yPos() + cell->highestPosition(false);
                top = qMin(top, hp);
        }
    }

    return top;
}

// Search from first_row to last_row for the row containing y
static unsigned int findRow(unsigned int first_row, unsigned int last_row,
                            const QVector<int> &rowPos, int y)
{
    unsigned int under = first_row;
    unsigned int over = last_row;
    int offset = (over - under)/2;
    while (over-under > 1) {
        if (rowPos[under+offset] <= y)
            under = under+offset;
        else
            over = under+offset;
        offset = (over - under)/2;
    }

    assert(under == first_row || rowPos[under] <= y);
    assert(over == last_row || rowPos[over] > y);

    return under;
}

static void findRowCover(unsigned int &startrow, unsigned int &endrow,
                         const QVector<int> &rowPos,
                         int min_y, int max_y)
{
    assert(max_y >= min_y);
    unsigned int totalRows = endrow;

    unsigned int index = 0;
    // Initial binary search boost:
    if (totalRows >= 8) {
        int offset = (endrow - startrow)/2;
        while (endrow - startrow > 1) {
            index = startrow+offset;
            if (rowPos[index] <= min_y )
                // index is below both min_y and max_y
                startrow = index;
            else
            if (rowPos[index] > max_y)
                // index is above both min_y and max_y
                endrow = index;
            else
                // index is within the selection
                break;
            offset = (endrow - startrow)/2;
        }
    }

    // Binary search for startrow
    startrow = findRow(startrow, endrow, rowPos, min_y);
    // Binary search for endrow
    endrow = findRow(startrow, endrow, rowPos, max_y) + 1;
    if (endrow > totalRows) endrow = totalRows;
}

void RenderTableSection::paint( PaintInfo& pI, int tx, int ty )
{
    unsigned int totalRows = grid.size();
    unsigned int totalCols = table()->columns.size();

    tx += m_x;
    ty += m_y;

    if (pI.phase == PaintActionOutline)
        paintOutline(pI.p, tx, ty, width(), height(), style());

    CollapsedBorderValue *cbs = table()->currentBorderStyle();
    int cbsw2 = cbs ? cbs->width()/2 : 0;
    int cbsw21 = cbs ? (cbs->width()+1)/2 : 0;

    int x = pI.r.x(), y = pI.r.y(), w = pI.r.width(), h = pI.r.height();
    // check which rows and cols are visible and only paint these
    int os = 2*maximalOutlineSize(pI.phase);
    unsigned int startrow = 0;
    unsigned int endrow = totalRows;
    findRowCover(startrow, endrow, rowPos, y - os - ty - qMax(cbsw21, os), y + h + os - ty + qMax(cbsw2, os));

    // A binary search is probably not worthwhile for columns
    unsigned int startcol = 0;
    unsigned int endcol = totalCols;
    if ( style()->direction() == LTR ) {
	for ( ; startcol < totalCols; startcol++ ) {
	    if ( tx + table()->columnPos[startcol+1] + qMax(cbsw21, os) > x - os )
		break;
	}
	for ( ; endcol > 0; endcol-- ) {
	    if ( tx + table()->columnPos[endcol-1] - qMax(cbsw2, os) < x + w + os )
		break;
	}
    }
    if ( startcol < endcol ) {
	// draw the cells
	for ( unsigned int r = startrow; r < endrow; r++ ) {
	    // paint the row
	    if (grid[r].rowRenderer) {
	        int height = rowPos[r+1] - rowPos[r] - table()->borderVSpacing();
	        grid[r].rowRenderer->paintRow(pI, tx, ty + rowPos[r], width(), height);
	    }

	    unsigned int c = startcol;
	    Row *row = grid[r].row;
	    Row *nextrow = (r < endrow - 1) ? grid[r+1].row : 0;
	    // since a cell can be -1 (indicating a colspan) we might have to search backwards to include it
	    while ( c && (*row)[c] == (RenderTableCell *)-1 )
		c--;
	    for ( ; c < endcol; c++ ) {
          RenderTableCell *cell = (*row)[c];
          if ( !cell || cell == (RenderTableCell *)-1 || (nextrow && (*nextrow)[c] == cell) )
		          continue;
                RenderObject* rowr = cell->parent();
                int rtx = tx+rowr->xPos();
                int rty = ty+rowr->yPos();
#ifdef TABLE_PRINT
		qDebug() << "painting cell " << r << "/" << c;
#endif
                if (pI.phase == PaintActionElementBackground || pI.phase == PaintActionChildBackground) {
                    // We need to handle painting a stack of backgrounds.  This stack (from bottom to top) consists of
                    // the column group, column, row group, row, and then the cell.
                    RenderObject* col = table()->colElement(c);
                    RenderObject* colGroup = 0;
                    if (col) {
                        RenderStyle *style = col->parent()->style();
                        if (style->display() == TABLE_COLUMN_GROUP)
                            colGroup = col->parent();
                    }


                    // ###
                    // Column groups and columns first.
                    // FIXME: Columns and column groups do not currently support opacity, and they are being painted "too late" in
                    // the stack, since we have already opened a transparency layer (potentially) for the table row group.
                    // Note that we deliberately ignore whether or not the cell has a layer, since these backgrounds paint "behind" the
                    // cell.
                    cell->paintBackgroundsBehindCell(pI, rtx, rty, colGroup);
                    cell->paintBackgroundsBehindCell(pI, rtx, rty, col);

                    // Paint the row group next.
                    cell->paintBackgroundsBehindCell(pI, rtx, rty, this);

                    // Paint the row next, but only if it doesn't have a layer.  If a row has a layer, it will be responsible for
                    // painting the row background for the cell.
                    if (!rowr->layer())
                        cell->paintBackgroundsBehindCell(pI, rtx, rty, rowr);
                }

                if ((!cell->layer() && !cell->parent()->layer()) || pI.phase == PaintActionCollapsedTableBorders)
                    cell->paint(pI, rtx, rty);
	    }
	}
    }
}

int RenderTableSection::numColumns() const
{
    int result = 0;

    for (int r = 0; r < numRows(); ++r) {
        for (int c = result; c < table()->numEffCols(); ++c) {
            if (cellAt(r, c))
                result = c;
        }
    }

    return result + 1;
}

void RenderTableSection::recalcCells()
{
    cCol = 0;
    cRow = -1;
    clearGrid();
    grid.resize( 0 );
    cellsWithColSpanZero.clear();
    cellsWithRowSpanZero.clear();

    for (RenderObject *row = firstChild(); row; row = row->nextSibling()) {
        if (row->isTableRow())  {
	cRow++;
	cCol = 0;
	ensureRows( cRow+1 );
            grid[cRow].rowRenderer = static_cast<RenderTableRow*>(row);

            for (RenderObject *cell = row->firstChild(); cell; cell = cell->nextSibling())
                if (cell->isTableCell())
                    addCell( static_cast<RenderTableCell *>(cell), static_cast<RenderTableRow *>(row) );

	}
    }
    needCellRecalc = false;
    setNeedsLayout(true);
}

void RenderTableSection::clearGrid()
{
    int rows = grid.size();
    while ( rows-- ) {
	delete grid[rows].row;
    }
}

bool RenderTableSection::emptyRow(int rowNum) {
    Row &r = *grid[rowNum].row;
    const int s = r.size();
    RenderTableCell *cell;
    for(int i=0; i<s; i++) {
        cell = r[i];
        if (!cell || cell==(RenderTableCell*)-1)
            continue;
        return false;
    }
    return true;
}

RenderObject* RenderTableSection::removeChildNode(RenderObject* child)
{
    setNeedCellRecalc();
    return RenderContainer::removeChildNode( child );
}

bool RenderTableSection::canClear(RenderObject * /*child*/, PageBreakLevel level)
{
    // We cannot clear rows yet.
    return parent()->canClear(this, level);
}

void RenderTableSection::addSpaceAt(int pos, int dy)
{
    const int totalRows = numRows();
    for ( int r = 0; r < totalRows; r++ ) {
        if (rowPos[r] > pos) {
            rowPos[r] += dy;
	    if (grid[r].rowRenderer)
		grid[r].rowRenderer->setPos(0, rowPos[r]);
        }
    }
    if (rowPos[totalRows] > pos)
        rowPos[totalRows] += dy;
    m_height = rowPos[totalRows];

    setContainsPageBreak(true);
}


#ifdef ENABLE_DUMP
void RenderTableSection::dump(QTextStream &stream, const QString &ind) const
{
    RenderContainer::dump(stream,ind);

    stream << " grid=(" << grid.size() << "," << table()->numEffCols() << ")";
    for ( int r = 0; r < grid.size(); r++ ) {
	for ( int c = 0; c < table()->numEffCols(); c++ ) {
	    if ( cellAt( r,  c ) && cellAt( r, c ) != (RenderTableCell *)-1 )
		stream << " (" << cellAt( r, c )->row() << "," << cellAt( r, c )->col() << ","
                       << cellAt(r, c)->rowSpan() << "," << cellAt(r, c)->colSpan() << ") ";
	    else
		stream << " null cell";
	}
    }
}
#endif

static RenderTableCell *seekCell(RenderTableSection *section, int row, int col)
{
    if (row < 0 || col < 0 || row >= section->numRows()) return 0;
    // since a cell can be -1 (indicating a colspan) we might have to search backwards to include it
    while ( col && section->cellAt( row, col ) == (RenderTableCell *)-1 )
	col--;

    return section->cellAt(row, col);
}

/** Looks for the first element suitable for text selection, beginning from
 * the last.
 * @param base search is restricted within this node. This node must have
 *	a renderer.
 * @return the element or @p base if no suitable element found.
 */
static NodeImpl *findLastSelectableNode(NodeImpl *base)
{
  NodeImpl *last = base;
  // Look for last text/cdata node that has a renderer,
  // or last childless replaced element
  while ( last && !(last->renderer()
  	&& ((last->nodeType() == Node::TEXT_NODE || last->nodeType() == Node::CDATA_SECTION_NODE)
		|| (last->renderer()->isReplaced() && !last->renderer()->lastChild()))))
  {
    NodeImpl *next = last->lastChild();
    if ( !next ) next = last->previousSibling();
    while ( last && last != base && !next )
    {
      last = last->parentNode();
      if ( last && last != base )
        next = last->previousSibling();
    }
    last = next;
  }

  return last ? last : base;
}

FindSelectionResult RenderTableSection::checkSelectionPoint( int _x, int _y, int _tx, int _ty, DOM::NodeImpl*& node, int & offset, SelPointState &state )
{
    Q_UNUSED( node );
    Q_UNUSED( offset );
    // Table sections need extra treatment for selections. The rows are scanned
    // from top to bottom, and within each row, only the cell that matches
    // the given position best is descended into.

    unsigned int totalRows = grid.size();
    unsigned int totalCols = table()->columns.size();

//    absolutePosition(_tx, _ty, false);

    _tx += m_x;
    _ty += m_y;

//    bool save_last = false;	// true to save last investigated cell

    if (needsLayout() || _y < _ty) return SelectionPointBefore;
//    else if (_y >= _ty + height()) save_last = true;

    // Find the row containing the pointer
    int row_idx = findRow(0, totalRows, rowPos, _y - _ty);

    int col_idx;
    if ( style()->direction() == LTR ) {
	for ( col_idx = (int)totalCols - 1; col_idx >= 0; col_idx-- ) {
	    if ( _tx + table()->columnPos[col_idx] < _x )
		break;
	}
	if (col_idx < 0) col_idx = 0;
    } else {
	for ( col_idx = 0; col_idx < (int)totalCols; col_idx++ ) {
	    if ( _tx + table()->columnPos[col_idx] > _x )
		break;
	}
	if (col_idx >= (int)totalCols) col_idx = (int)totalCols-1;
    }

    FindSelectionResult pos = SelectionPointBefore;

    RenderTableCell *cell = seekCell(this, row_idx, col_idx);
    // ### dunno why cell can be 0, maybe due to weird spans? (LS)
    if (cell) {
        SelPointState localState;
//        pos = cell->checkSelectionPoint(_x, _y, _tx, _ty, node, offset, localState);
    }

    if (pos != SelectionPointBefore) return pos;

    // store last column of last line
    row_idx--;
    col_idx = totalCols - 1;
    cell = seekCell(this, row_idx, col_idx);

    // end of section? take previous section
    RenderTableSection *sec = this;
    if (!cell) {
        sec = *--TableSectionIterator(sec);
        if (!sec) return pos;

	cell = seekCell(sec, sec->grid.size() - 1, col_idx);
	if (!cell) return pos;
    }

    // take last child of previous cell, and store this one as last node
    NodeImpl *element = cell->element();
    if (!element) return SelectionPointBefore;

    element = findLastSelectableNode(element);

    state.m_lastNode = element;
    state.m_lastOffset = element->maxOffset();
    return SelectionPointBefore;
}

// Hit Testing
bool RenderTableSection::nodeAtPoint(NodeInfo& info, int x, int y, int tx, int ty, HitTestAction action, bool inside)
{
    // Table sections cannot ever be hit tested.  Effectively they do not exist.
    // Just forward to our children always.
    tx += m_x;
    ty += m_y;

    for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
        // FIXME: We have to skip over inline flows, since they can show up inside table rows
        // at the moment (a demoted inline <form> for example). If we ever implement a
        // table-specific hit-test method (which we should do for performance reasons anyway),
        // then we can remove this check.
        if (!child->layer() && !child->isInlineFlow() && child->nodeAtPoint(info, x, y, tx, ty, action, inside)) {
            return true;
        }
    }

    return false;
}


// -------------------------------------------------------------------------

RenderTableRow::RenderTableRow(DOM::NodeImpl* node)
    : RenderBox(node)
{
    // init RenderObject attributes
    setInline(false);   // our object is not Inline
}

RenderObject* RenderTableRow::removeChildNode(RenderObject* child)
{
    RenderTableSection *s = section();
    if (s)
        s->setNeedCellRecalc();

    return RenderContainer::removeChildNode( child );
}

void RenderTableRow::detach()
{
    RenderTableSection *s = section();
    if (s)
        s->setNeedCellRecalc();

    RenderContainer::detach();
}

void RenderTableRow::setStyle(RenderStyle* newStyle)
{
    if (section() && style() && style()->height() != newStyle->height())
        section()->setNeedCellRecalc();

    newStyle->setDisplay(TABLE_ROW);
    RenderContainer::setStyle(newStyle);
}

void RenderTableRow::addChild(RenderObject *child, RenderObject *beforeChild)
{
#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << "(TableRow)::addChild( " << child->renderName() << " )"  << ", " <<
                       (beforeChild ? beforeChild->renderName() : "0") << " )" << endl;
#endif

    if ( !child->isTableCell() ) {
        // TR > FORM quirk (???)
        if (child->element() && child->element()->isHTMLElement() && child->element()->id() == ID_FORM &&
            static_cast<HTMLFormElementImpl*>(child->element())->isMalformed())
        {
            RenderContainer::addChild(child, beforeChild);
            return;
        }

        RenderObject* last = beforeChild;
        if (!last)
            last = lastChild();
        if (last && last->isAnonymous() && last->isTableCell()) {
            last->addChild(child);
            return;
        }

        // If beforeChild is inside an anonymous cell, insert into the cell.
        if (last && !last->isTableCell() && last->parent() && last->parent()->isAnonymous()) {
            last->parent()->addChild(child, beforeChild);
            return;
        }

        RenderTableCell* cell = new (renderArena()) RenderTableCell(document() /* anonymous object */);
        RenderStyle* newStyle = new RenderStyle();
        newStyle->inheritFrom(style());
        newStyle->setDisplay(TABLE_CELL);
        cell->setStyle(newStyle);
        addChild(cell, beforeChild);
        cell->addChild(child);
        return;
    }

    RenderTableCell* cell = static_cast<RenderTableCell*>(child);

    section()->addCell( cell, this );

    RenderContainer::addChild(cell,beforeChild);

    if ( beforeChild || nextSibling() )
        section()->setNeedCellRecalc();
}

void RenderTableRow::layout()
{
    KHTMLAssert( needsLayout() );
    KHTMLAssert( minMaxKnown() );

    RenderObject *child = firstChild();
    const bool pagedMode = canvas()->pagedMode();
    while( child ) {
	if ( child->isTableCell() ) {
            RenderTableCell *cell = static_cast<RenderTableCell *>(child);
            if (pagedMode) {
                cell->setNeedsLayout(true);
                int oldHeight = child->height();
                cell->layout();
                if (oldHeight > 0 && child->containsPageBreak() && child->height() != oldHeight) {
            // child has grown to accommodate a page-page, grow the same amount ourselves,
		    // and shift following rows down without relayouting the entire table
		    int adjust = child->height() - oldHeight;
		    setHeight(height()+adjust);
                    section()->addSpaceAt(yPos()+1, adjust);
		}
            } else
            if ( child->needsLayout() ) {
                if (markedForRepaint())
                    cell->setMarkedForRepaint( true );
                cell->calcVerticalMargins();
                cell->layout();
                cell->setCellTopExtra(0);
                cell->setCellBottomExtra(0);
                if (child->containsPageBreak()) setContainsPageBreak(true);
            }
	}
	child = child->nextSibling();
    }
    setMarkedForRepaint(false);
    setNeedsLayout(false);
}

int RenderTableRow::offsetLeft() const
{
    RenderObject *child = firstChild();
    while (child && !child->isTableCell())
        child = child->nextSibling();

    if (!child)
        return 0;

    return child->offsetLeft();
}

int RenderTableRow::offsetTop() const
{
    RenderObject *child = firstChild();
    while (child && !child->isTableCell())
        child = child->nextSibling();

    if (!child)
        return 0;

    return child->offsetTop() -
                  static_cast<RenderTableCell*>(child)->cellTopExtra();
}

int RenderTableRow::offsetHeight() const
{
    RenderObject *child = firstChild();
    while (child && !child->isTableCell())
        child = child->nextSibling();

    if (!child)
        return 0;

    return child->offsetHeight();
}

short RenderTableRow::offsetWidth() const
{
    RenderObject *fc = firstChild();
    RenderObject *lc = lastChild();
    while (fc && !fc->isTableCell())
        fc = fc->nextSibling();
    while (lc && !lc->isTableCell())
        lc = lc->previousSibling();
    if (!lc || !fc)
        return 0;

    return lc->xPos()+lc->width()-fc->xPos();
}

void RenderTableRow::paintRow( PaintInfo& pI, int tx, int ty, int w, int h )
{
    if (pI.phase == PaintActionOutline)
        paintOutline(pI.p, tx, ty, w, h, style());
}

void RenderTableRow::paint(PaintInfo& i, int tx, int ty)
{
    KHTMLAssert(layer());
    if (!layer())
        return;

    tx += m_x;
    ty += m_y;

    for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
        if (child->isTableCell()) {
            // Paint the row background behind the cell.
            if (i.phase == PaintActionElementBackground || i.phase == PaintActionChildBackground) {
                RenderTableCell* cell = static_cast<RenderTableCell*>(child);
                cell->paintBackgroundsBehindCell(i, tx, ty, this);
            }
            if (!child->layer())
                child->paint(i, tx, ty);
        }
    }
}

// Hit Testing
bool RenderTableRow::nodeAtPoint(NodeInfo& info, int x, int y, int tx, int ty, HitTestAction action, bool inside)
{
    // Table rows cannot ever be hit tested.  Effectively they do not exist.
    // Just forward to our children always.
    tx += m_x;
    ty += m_y;

    for (RenderObject* child = lastChild(); child; child = child->previousSibling()) {
        // FIXME: We have to skip over inline flows, since they can show up inside table rows
        // at the moment (a demoted inline <form> for example). If we ever implement a
        // table-specific hit-test method (which we should do for performance reasons anyway),
        // then we can remove this check.
        if (!child->layer() && !child->isInlineFlow() && child->nodeAtPoint(info, x, y, tx, ty, action, inside)) {
            return true;
        }
    }

    return false;
}

// -------------------------------------------------------------------------

RenderTableCell::RenderTableCell(DOM::NodeImpl* _node)
  : RenderBlock(_node)
{
  _col = -1;
  _row = -1;
  cSpan = 1;
  rSpan = 1;
  updateFromElement();
  setShouldPaintBackgroundOrBorder(true);
  _topExtra = 0;
  _bottomExtra = 0;
  m_percentageHeight = -1;
  m_hasFlexedAnonymous = false;
  m_widthChanged = false;
}

void RenderTableCell::detach()
{
    if (parent() && section())
        section()->setNeedCellRecalc();

    RenderBlock::detach();
}

void RenderTableCell::updateFromElement()
{
  DOM::NodeImpl *node = element();
  if ( node && (node->id() == ID_TD || node->id() == ID_TH) ) {
      DOM::HTMLTableCellElementImpl *tc = static_cast<DOM::HTMLTableCellElementImpl *>(node);
      int oldCSpan = cSpan;
      int oldRSpan = rSpan;

      cSpan = tc->colSpan();
      rSpan = tc->rowSpan();
      if ((oldRSpan != rSpan || oldCSpan != cSpan) && style() && parent()) {
          setNeedsLayoutAndMinMaxRecalc();
          if (section())
              section()->setNeedCellRecalc();
      }
  } else {
      cSpan = rSpan = 1;
  }
}

Length RenderTableCell::styleOrColWidth()
{
    Length w = style()->width();
    if (colSpan() > 1 || !w.isAuto())
        return w;
    RenderTableCol* col = table()->colElement(_col);
    if (col) {
        w = col->style()->width();

        // Column widths specified on <col> apply to the border box of the cell.
        // Percentages don't need to be handled since they're always treated this way (even when specified on the cells).
        if (w.isFixed() && w.isPositive())
            w = Length(qMax(0, w.value() - borderLeft() - borderRight() - paddingLeft() - paddingRight()), Fixed);
    }
    return w;
}

void RenderTableCell::calcMinMaxWidth()
{
    KHTMLAssert( !minMaxKnown() );
#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << "(TableCell)::calcMinMaxWidth() known=" << minMaxKnown();
#endif

    if (section()->needCellRecalc)
        section()->recalcCells();

    RenderBlock::calcMinMaxWidth();
    if (element() && style()->whiteSpace() == NORMAL) {
        // See if nowrap was set.
        Length w = styleOrColWidth();
        DOMString nowrap = static_cast<ElementImpl*>(element())->getAttribute(ATTR_NOWRAP);
        if (!nowrap.isNull() && w.isFixed() &&
            m_minWidth < w.value() )
            // Nowrap is set, but we didn't actually use it because of the
            // fixed width set on the cell.  Even so, it is a WinIE/Moz trait
            // to make the minwidth of the cell into the fixed width.  They do this
            // even in strict mode, so do not make this a quirk.  Affected the top
            // of hiptop.com.
            m_minWidth = w.value();
    }

    setMinMaxKnown();
}

void RenderTableCell::calcWidth()
{
}

void RenderTableCell::setWidth( int width )
{
    if ( width != m_width ) {
	m_width = width;
	m_widthChanged = true;
    }
}

void RenderTableCell::layout()
{
    layoutBlock( m_widthChanged );
    m_widthChanged = false;
}

void RenderTableCell::close()
{
    RenderBlock::close();

#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << "(RenderTableCell)::close() total height =" << m_height;
#endif
}

bool RenderTableCell::requiresLayer() const {
    // table-cell display is never positioned (css 2.1-9.7)
    return style()->opacity() < 1.0f || hasOverflowClip() || isRelPositioned();
}

void RenderTableCell::repaintRectangle(int x, int y, int w, int h, Priority p, bool f)
{
    RenderBlock::repaintRectangle(x, y, w, h + _topExtra + _bottomExtra, p, f);
}

int RenderTableCell::pageTopAfter(int y) const
{
    return parent()->pageTopAfter(y+m_y + _topExtra) - (m_y  + _topExtra);
}

short RenderTableCell::baselinePosition( bool ) const
{
    RenderObject* o = firstChild();
    int offset = paddingTop() + borderTop();
    if (!o) return offset + contentHeight();
    while (o->firstChild()) {
	if (!o->isInline())
	    offset += o->paddingTop() + o->borderTop();
	o = o->firstChild();
    }

    if (!o->isInline())
        return paddingTop() + borderTop() + contentHeight();

    offset += o->baselinePosition( true );
    return offset;
}


void RenderTableCell::setStyle( RenderStyle *newStyle )
{
    if (parent() && section() && style() && style()->height() != newStyle->height())
        section()->setNeedCellRecalc();

    newStyle->setDisplay(TABLE_CELL);
    RenderBlock::setStyle( newStyle );
    setShouldPaintBackgroundOrBorder(true);

    if (newStyle->whiteSpace() == KHTML_NOWRAP) {
      // Figure out if we are really nowrapping or if we should just
      // use normal instead.  If the width of the cell is fixed, then
      // we don't actually use NOWRAP.
      if (newStyle->width().isFixed())
	newStyle->setWhiteSpace(NORMAL);
      else
	newStyle->setWhiteSpace(NOWRAP);
    }
}

bool RenderTableCell::nodeAtPoint(NodeInfo& info, int _x, int _y, int _tx, int _ty, HitTestAction hitTestAction, bool inside)
{
  int tx = _tx + m_x;
  int ty = _ty + m_y;

  // also include the top and bottom extra space
  inside |= hitTestAction != HitTestChildrenOnly && style()->visibility() != HIDDEN
      && (_y >= ty) && (_y < ty + height() + _topExtra + _bottomExtra)
      && (_x >= tx) && (_x < tx + width());

  return RenderBlock::nodeAtPoint(info, _x, _y, _tx, _ty, hitTestAction, inside);
}

// The following rules apply for resolving conflicts and figuring out which border
// to use.
// (1) Borders with the 'border-style' of 'hidden' take precedence over all other conflicting
// borders. Any border with this value suppresses all borders at this location.
// (2) Borders with a style of 'none' have the lowest priority. Only if the border properties of all
// the elements meeting at this edge are 'none' will the border be omitted (but note that 'none' is
// the default value for the border style.)
// (3) If none of the styles are 'hidden' and at least one of them is not 'none', then narrow borders
// are discarded in favor of wider ones. If several have the same 'border-width' then styles are preferred
// in this order: 'double', 'solid', 'dashed', 'dotted', 'ridge', 'outset', 'groove', and the lowest: 'inset'.
// (4) If border styles differ only in color, then a style set on a cell wins over one on a row,
// which wins over a row group, column, column group and, lastly, table. It is undefined which color
// is used when two elements of the same type disagree.
static CollapsedBorderValue compareBorders(const CollapsedBorderValue& border1,
                                           const CollapsedBorderValue& border2)
{
    // Sanity check the values passed in.  If either is null, return the other.
    if (!border2.exists()) return border1;
    if (!border1.exists()) return border2;

    // Rule #1 above.
    if (border1.style() == BHIDDEN || border2.style() == BHIDDEN)
        return CollapsedBorderValue(); // No border should exist at this location.

    // Rule #2 above.  A style of 'none' has lowest priority and always loses to any other border.
    if (border2.style() == BNONE) return border1;
    if (border1.style() == BNONE) return border2;

    // The first part of rule #3 above. Wider borders win.
    if (border1.width() != border2.width())
        return border1.width() > border2.width() ? border1 : border2;

    // The borders have equal width.  Sort by border style.
    if (border1.style() != border2.style())
        return border1.style() > border2.style() ? border1 : border2;

    // The border have the same width and style.  Rely on precedence (cell over row over row group, etc.)
    return border1.precedence >= border2.precedence ? border1 : border2;
}

CollapsedBorderValue RenderTableCell::collapsedLeftBorder(bool rtl) const
{
    RenderTable* tableElt = table();
    bool leftmostColumn;
    if (!rtl)
        leftmostColumn = col() == 0;
    else {
        int effCol = tableElt->colToEffCol(col() + colSpan() - 1);
        leftmostColumn = effCol == tableElt->numEffCols() - 1;
    }

    // For border left, we need to check, in order of precedence:
    // (1) Our left border.
    CollapsedBorderValue result(&style()->borderLeft(), BCELL);

    // (2) The right border of the cell to the left.
    RenderTableCell* prevCell = rtl ? tableElt->cellAfter(this) : tableElt->cellBefore(this);
    if (prevCell) {
        result = compareBorders(result, CollapsedBorderValue(&prevCell->style()->borderRight(), BCELL));
        if (!result.exists())
            return result;
    }
    else if (leftmostColumn) {
        // (3) Our row's left border.
        result = compareBorders(result, CollapsedBorderValue(&parent()->style()->borderLeft(), BROW));
        if (!result.exists())
            return result;

        // (4) Our row group's left border.
        result = compareBorders(result, CollapsedBorderValue(&section()->style()->borderLeft(), BROWGROUP));
        if (!result.exists())
            return result;
    }

    // (5) Our column and column group's left borders.
    bool startColEdge;
    bool endColEdge;
    RenderTableCol* colElt = table()->colElement(col() + (rtl ? colSpan() - 1 : 0), &startColEdge, &endColEdge);
    if (colElt && (!rtl ? startColEdge : endColEdge)) {
        result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderLeft(), BCOL));
        if (!result.exists())
            return result;
        if (colElt->parent()->isTableCol() && (!rtl ? !colElt->previousSibling() : !colElt->nextSibling())) {
            result = compareBorders(result, CollapsedBorderValue(&colElt->parent()->style()->borderLeft(), BCOLGROUP));
            if (!result.exists())
                return result;
        }
    }

    // (6) The previous column's right border.
    if (!leftmostColumn) {
        colElt = table()->colElement(col() + (rtl ? colSpan() : -1), &startColEdge, &endColEdge);
        if (colElt && (!rtl ? endColEdge : startColEdge)) {
            result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderRight(), BCOL));
            if (!result.exists())
                return result;
        }
    } else {
        // (7) The table's left border.
        result = compareBorders(result, CollapsedBorderValue(&table()->style()->borderLeft(), BTABLE));
        if (!result.exists())
            return result;
    }

    return result;
}

CollapsedBorderValue RenderTableCell::collapsedRightBorder(bool rtl) const
{
    RenderTable* tableElt = table();
    bool rightmostColumn;
    if (rtl)
        rightmostColumn = col() == 0;
    else {
        int effCol = tableElt->colToEffCol(col() + colSpan() - 1);
        rightmostColumn = effCol == tableElt->numEffCols() - 1;
    }

    // For border right, we need to check, in order of precedence:
    // (1) Our right border.
    CollapsedBorderValue result = CollapsedBorderValue(&style()->borderRight(), BCELL);

    // (2) The left border of the cell to the right.
    if (!rightmostColumn) {
        RenderTableCell* nextCell = rtl ? tableElt->cellBefore(this) : tableElt->cellAfter(this);
        if (nextCell && nextCell->style()) {
            result = compareBorders(result, CollapsedBorderValue(&nextCell->style()->borderLeft(), BCELL));
            if (!result.exists())
                return result;
        }
    } else {
        // (3) Our row's right border.
        result = compareBorders(result, CollapsedBorderValue(&parent()->style()->borderRight(), BROW));
        if (!result.exists())
            return result;

        // (4) Our row group's right border.
        result = compareBorders(result, CollapsedBorderValue(&section()->style()->borderRight(), BROWGROUP));
        if (!result.exists())
            return result;
    }

    // (5) Our column and column group's right borders.
    bool startColEdge;
    bool endColEdge;
    RenderTableCol* colElt = tableElt->colElement(col() + (rtl ? 0 : colSpan() - 1), &startColEdge, &endColEdge);
    if (colElt && (!rtl ? endColEdge : startColEdge)) {
        result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderRight(), BCOL));
        if (!result.exists())
            return result;
        if (colElt->parent()->isTableCol() && (!rtl ? !colElt->nextSibling() : !colElt->previousSibling())) {
            result = compareBorders(result, CollapsedBorderValue(&colElt->parent()->style()->borderRight(), BCOLGROUP));
            if (!result.exists())
                return result;
        }
    }

    // (6) The next column's left border.
    if (!rightmostColumn) {
        colElt = tableElt->colElement(col() + (rtl ? -1 : colSpan()), &startColEdge, &endColEdge);
        if (colElt && (!rtl ? startColEdge : endColEdge)) {
            result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderLeft(), BCOL));
            if (!result.exists())
                return result;
        }
    }
    else {
        // (7) The table's right border.
        result = compareBorders(result, CollapsedBorderValue(&tableElt->style()->borderRight(), BTABLE));
        if (!result.exists())
            return result;
    }

    return result;
}

CollapsedBorderValue RenderTableCell::collapsedTopBorder() const
{
    // For border top, we need to check, in order of precedence:
    // (1) Our top border.
    CollapsedBorderValue result = CollapsedBorderValue(&style()->borderTop(), BCELL);

    RenderTableCell* prevCell = table()->cellAbove(this);
    if (prevCell) {
        // (2) A previous cell's bottom border.
        result = compareBorders(result, CollapsedBorderValue(&prevCell->style()->borderBottom(), BCELL));
        if (!result.exists())
            return result;
    }

    // (3) Our row's top border.
    result = compareBorders(result, CollapsedBorderValue(&parent()->style()->borderTop(), BROW));
    if (!result.exists())
        return result;

    // (4) The previous row's bottom border.
    if (prevCell) {
        RenderObject* prevRow = 0;
        if (prevCell->section() == section())
            prevRow = parent()->previousSibling();
        else
            prevRow = prevCell->section()->lastChild();

        if (prevRow) {
            result = compareBorders(result, CollapsedBorderValue(&prevRow->style()->borderBottom(), BROW));
            if (!result.exists())
                return result;
        }
    }

    // Now check row groups.
    RenderTableSection* currSection = section();
    if (row() == 0) {
        // (5) Our row group's top border.
        result = compareBorders(result, CollapsedBorderValue(&currSection->style()->borderTop(), BROWGROUP));
        if (!result.exists())
            return result;

        // (6) Previous row group's bottom border.
        currSection = table()->sectionAbove(currSection);
        if (currSection) {
            result = compareBorders(result, CollapsedBorderValue(&currSection->style()->borderBottom(), BROWGROUP));
            if (!result.exists())
                return result;
        }
    }

    if (!currSection) {
        // (8) Our column and column group's top borders.
        RenderTableCol* colElt = table()->colElement(col());
        if (colElt) {
            result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderTop(), BCOL));
            if (!result.exists())
                return result;
            if (colElt->parent()->isTableCol()) {
                result = compareBorders(result, CollapsedBorderValue(&colElt->parent()->style()->borderTop(), BCOLGROUP));
                if (!result.exists())
                    return result;
            }
        }

        // (9) The table's top border.
        result = compareBorders(result, CollapsedBorderValue(&table()->style()->borderTop(), BTABLE));
        if (!result.exists())
            return result;
    }

    return result;
}

CollapsedBorderValue RenderTableCell::collapsedBottomBorder() const
{
    // For border top, we need to check, in order of precedence:
    // (1) Our bottom border.
    CollapsedBorderValue result = CollapsedBorderValue(&style()->borderBottom(), BCELL);

    RenderTableCell* nextCell = table()->cellBelow(this);
    if (nextCell) {
        // (2) A following cell's top border.
        result = compareBorders(result, CollapsedBorderValue(&nextCell->style()->borderTop(), BCELL));
        if (!result.exists())
            return result;
    }

    // (3) Our row's bottom border. (FIXME: Deal with rowspan!)
    result = compareBorders(result, CollapsedBorderValue(&parent()->style()->borderBottom(), BROW));
    if (!result.exists())
        return result;

    // (4) The next row's top border.
    if (nextCell) {
        result = compareBorders(result, CollapsedBorderValue(&nextCell->parent()->style()->borderTop(), BROW));
        if (!result.exists())
            return result;
    }

    // Now check row groups.
    RenderTableSection* currSection = section();
    if (row()+rowSpan() >= static_cast<RenderTableSection*>(currSection)->numRows()) {
        // (5) Our row group's bottom border.
        result = compareBorders(result, CollapsedBorderValue(&currSection->style()->borderBottom(), BROWGROUP));
        if (!result.exists())
            return result;

        // (6) Following row group's top border.
        currSection = table()->sectionBelow(currSection);
        if (currSection) {
            result = compareBorders(result, CollapsedBorderValue(&currSection->style()->borderTop(), BROWGROUP));
            if (!result.exists())
                return result;
        }
    }

    if (!currSection) {
        // (8) Our column and column group's bottom borders.
        RenderTableCol* colElt = table()->colElement(col());
        if (colElt) {
            result = compareBorders(result, CollapsedBorderValue(&colElt->style()->borderBottom(), BCOL));
            if (!result.exists())
                return result;
            if (colElt->parent()->isTableCol()) {
                result = compareBorders(result, CollapsedBorderValue(&colElt->parent()->style()->borderBottom(), BCOLGROUP));
                if (!result.exists())
                    return result;
            }
        }

        // (9) The table's bottom border.
        result = compareBorders(result, CollapsedBorderValue(&table()->style()->borderBottom(), BTABLE));
        if (!result.exists())
            return result;
    }

    return result;
}

int RenderTableCell::borderLeft() const
{
    if (table()->collapseBorders()) {
        CollapsedBorderValue border = collapsedLeftBorder(table()->style()->direction() == RTL);
        if (border.exists())
            return (border.width()+1)/2; // Give the extra pixel to top and left.
        return 0;
    }
    return RenderBlock::borderLeft();
}

int RenderTableCell::borderRight() const
{
    if (table()->collapseBorders()) {
        CollapsedBorderValue border = collapsedRightBorder(table()->style()->direction() == RTL);
        if (border.exists())
            return border.width()/2;
        return 0;
    }
    return RenderBlock::borderRight();
}

int RenderTableCell::borderTop() const
{
    if (table()->collapseBorders()) {
        CollapsedBorderValue border = collapsedTopBorder();
        if (border.exists())
            return (border.width()+1)/2; // Give the extra pixel to top and left.
        return 0;
    }
    return RenderBlock::borderTop();
}

int RenderTableCell::borderBottom() const
{
    if (table()->collapseBorders()) {
        CollapsedBorderValue border = collapsedBottomBorder();
        if (border.exists())
            return border.width()/2;
        return 0;
    }
    return RenderBlock::borderBottom();
}

#ifdef BOX_DEBUG
#include <qpainter.h>

static void outlineBox(QPainter *p, int _tx, int _ty, int w, int h)
{
    p->setPen(QPen(QColor("yellow"), 3, Qt::DotLine));
    p->setBrush( Qt::NoBrush );
    p->drawRect(_tx, _ty, w, h );
}
#endif

void RenderTableCell::paint(PaintInfo& pI, int _tx, int _ty)
{

#ifdef TABLE_PRINT
    qDebug() << renderName() << "(RenderTableCell)::paint() w/h = (" << width() << "/" << height() << ")" << " _y/_h=" << pI.r.y() << "/" << pI.r.height();
#endif

    if (needsLayout()) return;

    _tx += m_x;
    _ty += m_y/* + _topExtra*/;

    RenderTable *tbl = table();

    // check if we need to do anything at all...
    int os = qMax(tbl->currentBorderStyle() ? (tbl->currentBorderStyle()->border->width+1)/2 : 0, 2*maximalOutlineSize(pI.phase));
    if ((_ty >= pI.r.y() + pI.r.height() + os)
         || (_ty + _topExtra + m_height + _bottomExtra <= pI.r.y() - os)) return;

    if (pI.phase == PaintActionOutline) {
        paintOutline( pI.p, _tx, _ty, width(), height() + borderTopExtra() + borderBottomExtra(), style());
    }

    if (pI.phase == PaintActionCollapsedTableBorders && style()->visibility() == VISIBLE) {
        int w = width();
        int h = height() + borderTopExtra() + borderBottomExtra();
        paintCollapsedBorder(pI.p, _tx, _ty, w, h);
    }
    else
        RenderBlock::paintObject(pI, _tx, _ty + _topExtra, false);

#ifdef BOX_DEBUG
    ::outlineBox(pI.p, _tx, _ty - _topExtra, width(), height() + borderTopExtra() + borderBottomExtra());
#endif
}

static EBorderStyle collapsedBorderStyle(EBorderStyle style)
{
    if (style == OUTSET)
        style = GROOVE;
    else if (style == INSET)
        style = RIDGE;
    return style;
}

struct CollapsedBorder {
    CollapsedBorder(){}

    CollapsedBorderValue border;
    RenderObject::BorderSide side;
    bool shouldPaint;
    int x1;
    int y1;
    int x2;
    int y2;
    EBorderStyle style;
};

class CollapsedBorders
{
public:
    CollapsedBorders() :count(0) {}

    void addBorder(const CollapsedBorderValue& b, RenderObject::BorderSide s, bool paint,
                   int _x1, int _y1, int _x2, int _y2,
                   EBorderStyle _style)
    {
        if (b.exists() && paint) {
            borders[count].border = b;
            borders[count].side = s;
            borders[count].shouldPaint = paint;
            borders[count].x1 = _x1;
            borders[count].x2 = _x2;
            borders[count].y1 = _y1;
            borders[count].y2 = _y2;
            borders[count].style = _style;
            count++;
        }
    }

    CollapsedBorder* nextBorder() {
        for (int i = 0; i < count; i++) {
            if (borders[i].border.exists() && borders[i].shouldPaint) {
                borders[i].shouldPaint = false;
                return &borders[i];
            }
        }

        return 0;
    }

    CollapsedBorder borders[4];
    int count;
};

static void addBorderStyle(QList<CollapsedBorderValue>& borderStyles, CollapsedBorderValue borderValue)
{
    if (!borderValue.exists() || borderStyles.contains(borderValue))
        return;

    QList<CollapsedBorderValue>::Iterator it = borderStyles.begin();
    QList<CollapsedBorderValue>::Iterator end = borderStyles.end();
    for (; it != end; ++it) {
        CollapsedBorderValue result = compareBorders(*it, borderValue);
        if (result == *it) {
            borderStyles.insert(it, borderValue);
            return;
        }
    }

    borderStyles.append(borderValue);
}

void RenderTableCell::collectBorders(QList<CollapsedBorderValue>& borderStyles)
{
    bool rtl = table()->style()->direction() == RTL;
    addBorderStyle(borderStyles, collapsedLeftBorder(rtl));
    addBorderStyle(borderStyles, collapsedRightBorder(rtl));
    addBorderStyle(borderStyles, collapsedTopBorder());
    addBorderStyle(borderStyles, collapsedBottomBorder());
}

void RenderTableCell::paintCollapsedBorder(QPainter* p, int _tx, int _ty, int w, int h)
{
    if (!table()->currentBorderStyle())
        return;

    bool rtl = table()->style()->direction() == RTL;
    CollapsedBorderValue leftVal = collapsedLeftBorder(rtl);
    CollapsedBorderValue rightVal = collapsedRightBorder(rtl);
    CollapsedBorderValue topVal = collapsedTopBorder();
    CollapsedBorderValue bottomVal = collapsedBottomBorder();

    // Adjust our x/y/width/height so that we paint the collapsed borders at the correct location.
    int topWidth = topVal.width();
    int bottomWidth = bottomVal.width();
    int leftWidth = leftVal.width();
    int rightWidth = rightVal.width();

    _tx -= leftWidth/2;
    _ty -= topWidth/2;
    w += leftWidth/2 + (rightWidth+1)/2;
    h += topWidth/2 + (bottomWidth+1)/2;

    bool tt = topVal.isTransparent();
    bool bt = bottomVal.isTransparent();
    bool rt = rightVal.isTransparent();
    bool lt = leftVal.isTransparent();

    EBorderStyle ts = collapsedBorderStyle(topVal.style());
    EBorderStyle bs = collapsedBorderStyle(bottomVal.style());
    EBorderStyle ls = collapsedBorderStyle(leftVal.style());
    EBorderStyle rs = collapsedBorderStyle(rightVal.style());

    bool render_t = ts > BHIDDEN && !tt && (topVal.precedence != BCELL || *topVal.border == style()->borderTop());
    bool render_l = ls > BHIDDEN && !lt && (leftVal.precedence != BCELL || *leftVal.border == style()->borderLeft());
    bool render_r = rs > BHIDDEN && !rt && (rightVal.precedence != BCELL || *rightVal.border == style()->borderRight());
    bool render_b = bs > BHIDDEN && !bt && (bottomVal.precedence != BCELL || *bottomVal.border == style()->borderBottom());

    // We never paint diagonals at the joins.  We simply let the border with the highest
    // precedence paint on top of borders with lower precedence.
    CollapsedBorders borders;
    borders.addBorder(topVal, BSTop, render_t, _tx, _ty, _tx + w, _ty + topWidth, ts);
    borders.addBorder(bottomVal, BSBottom, render_b, _tx, _ty + h - bottomWidth, _tx + w, _ty + h, bs);
    borders.addBorder(leftVal, BSLeft, render_l, _tx, _ty, _tx + leftWidth, _ty + h, ls);
    borders.addBorder(rightVal, BSRight, render_r, _tx + w - rightWidth, _ty, _tx + w, _ty + h, rs);

    for (CollapsedBorder* border = borders.nextBorder(); border; border = borders.nextBorder()) {
        if (border->border == *table()->currentBorderStyle())
            drawBorder(p, border->x1, border->y1, border->x2, border->y2, border->side,
                       border->border.color(), style()->color(), border->style, 0, 0);
    }
}

void RenderTableCell::paintBackgroundsBehindCell(PaintInfo& pI, int _tx, int _ty, RenderObject* bgObj)
{
    if (!bgObj || style()->visibility() != VISIBLE)
        return;

    RenderTable* tableElt = table();

    int w = bgObj->width();
    int h = bgObj->height() + bgObj->borderTopExtra() + bgObj->borderBottomExtra();

    int cellx = _tx;
    int celly = _ty;
    int cellw = w;
    int cellh = h;
    if (bgObj != this) {
        cellx += m_x;
        celly += m_y;
        cellw = width();
        cellh = height() + borderTopExtra() + borderBottomExtra();
    }

    QRect cr;
    cr.setX(qMax(cellx, pI.r.x()));
    cr.setY(qMax(celly, pI.r.y()));
    cr.setWidth(cellx<pI.r.x() ? qMax(0,cellw-(pI.r.x()-cellx)) : qMin(pI.r.width(),cellw));
    cr.setHeight(celly<pI.r.y() ? qMax(0,cellh-(pI.r.y()-celly)) : qMin(pI.r.height(),cellh));

    QColor c = bgObj->style()->backgroundColor();
    const BackgroundLayer* bgLayer = bgObj->style()->backgroundLayers();

    if (bgLayer->hasImage() || c.isValid()) {
        // We have to clip here because the background would paint
        // on top of the borders otherwise.  This only matters for cells and rows.
        bool hasLayer = bgObj->layer() && (bgObj == this || bgObj == parent());
        if (hasLayer && tableElt->collapseBorders()) {
            pI.p->save();
            QRect clipRect(cellx + borderLeft(), celly + borderTop(), cellw - borderLeft() - borderRight(), cellh - borderTop() - borderBottom());
            clipRect = pI.p->combinedMatrix().mapRect(clipRect);
            QRegion creg(clipRect);
            QRegion old = pI.p->clipRegion();
            if (!old.isEmpty())
                creg = old.intersect(creg);
            pI.p->setClipRegion(creg);
        }
        KHTMLAssert(bgObj->isBox());
        static_cast<RenderBox*>(bgObj)->paintAllBackgrounds(pI.p, c, bgLayer, cr, _tx, _ty, w, h);
        if (hasLayer && tableElt->collapseBorders())
            pI.p->restore();
    }
}

void RenderTableCell::paintBoxDecorations(PaintInfo& pI, int _tx, int _ty)
{
    RenderTable* tableElt = table();
    bool drawBorders = true;
    // Moz paints bgcolor/bgimage on <td>s in quirks mode even if
    // empty-cells are on. Fixes regression on #43426, attachment #354
    if (!tableElt->collapseBorders() && style()->emptyCells() == HIDE && !firstChild())
        drawBorders = false;
    if (!style()->htmlHacks() && !drawBorders) return;

    _ty -= borderTopExtra();

    // Paint our cell background.
    paintBackgroundsBehindCell(pI, _tx, _ty, this);

    int w = width();
    int h = height() + borderTopExtra() + borderBottomExtra();

    if (drawBorders && style()->hasBorder() && !tableElt->collapseBorders())
        paintBorder(pI.p, _tx, _ty, w, h, style());
}


#ifdef ENABLE_DUMP
void RenderTableCell::dump(QTextStream &stream, const QString &ind) const
{
    RenderFlow::dump(stream,ind);
    stream << " row=" << _row;
    stream << " col=" << _col;
    stream << " rSpan=" << rSpan;
    stream << " cSpan=" << cSpan;
//    *stream << " nWrap=" << nWrap;
}
#endif

// -------------------------------------------------------------------------

RenderTableCol::RenderTableCol(DOM::NodeImpl* node)
    : RenderBox(node), m_span(1)
{
    // init RenderObject attributes
    setInline(true);   // our object is not Inline
    updateFromElement();
}

void RenderTableCol::updateFromElement()
{
  DOM::NodeImpl *node = element();
  if ( node && (node->id() == ID_COL || node->id() == ID_COLGROUP) ) {
      DOM::HTMLTableColElementImpl *tc = static_cast<DOM::HTMLTableColElementImpl *>(node);
      m_span = tc->span();
  } else
      m_span = ! ( style() && style()->display() == TABLE_COLUMN_GROUP );
}

#ifdef ENABLE_DUMP
void RenderTableCol::dump(QTextStream &stream, const QString &ind) const
{
    RenderContainer::dump(stream,ind);
    stream << " _span=" << m_span;
}
#endif

// -------------------------------------------------------------------------

TableSectionIterator::TableSectionIterator(RenderTable *table, bool fromEnd)
{
  if (fromEnd) {
    sec = table->foot;
    if (sec) return;

    sec = static_cast<RenderTableSection *>(table->lastChild());
    while (sec && (!sec->isTableSection()
    		|| sec == table->head || sec == table->foot))
      sec = static_cast<RenderTableSection *>(sec->previousSibling());
    if (sec) return;

    sec = table->head;
  } else {
    sec = table->head;
    if (sec) return;

    sec = static_cast<RenderTableSection *>(table->firstChild());
    while (sec && (!sec->isTableSection()
    		|| sec == table->head || sec == table->foot))
      sec = static_cast<RenderTableSection *>(sec->nextSibling());
    if (sec) return;

    sec = table->foot;
  }/*end if*/

}

TableSectionIterator &TableSectionIterator::operator ++()
{
  RenderTable *table = sec->table();
  if (sec == table->head) {

    sec = static_cast<RenderTableSection *>(table->firstChild());
    while (sec && (!sec->isTableSection()
    		|| sec == table->head || sec == table->foot))
      sec = static_cast<RenderTableSection *>(sec->nextSibling());
    if (sec) return *this;

  } else if (sec == table->foot) {
    sec = 0;
    return *this;

  } else {

    do {
      sec = static_cast<RenderTableSection *>(sec->nextSibling());
    } while (sec && (!sec->isTableSection() || sec == table->head || sec == table->foot));
    if (sec) return *this;

  }/*end if*/

  sec = table->foot;
  return *this;
}

TableSectionIterator &TableSectionIterator::operator --()
{
  RenderTable *table = sec->table();
  if (sec == table->foot) {

    sec = static_cast<RenderTableSection *>(table->lastChild());
    while (sec && (!sec->isTableSection()
    		|| sec == table->head || sec == table->foot))
      sec = static_cast<RenderTableSection *>(sec->previousSibling());
    if (sec) return *this;

  } else if (sec == table->head) {
    sec = 0;
    return *this;

  } else {

    do {
      sec = static_cast<RenderTableSection *>(sec->previousSibling());
    } while (sec && (!sec->isTableSection() || sec == table->head || sec == table->foot));
    if (sec) return *this;

  }/*end if*/

  sec = table->foot;
  return *this;
}

#undef TABLE_DEBUG
#undef DEBUG_LAYOUT
#undef BOX_DEBUG
