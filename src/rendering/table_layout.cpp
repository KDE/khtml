/*
 * This file is part of the HTML rendering engine for KDE.
 *
 * Copyright (C) 2002 Lars Knoll (knoll@kde.org)
 *           (C) 2002 Dirk Mueller (mueller@kde.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License.
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
#include "table_layout.h"
#include "render_table.h"

using namespace khtml;

// #define DEBUG_LAYOUT

/*
  The text below is from the CSS 2.1 specs.

  Fixed table layout
  ------------------

  With this (fast) algorithm, the horizontal layout of the table does
  not depend on the contents of the cells; it only depends on the
  table's width, the width of the columns, and borders or cell
  spacing.

  The table's width may be specified explicitly with the 'width'
  property. A value of 'auto' (for both 'display: table' and 'display:
  inline-table') means use the automatic table layout algorithm.

  In the fixed table layout algorithm, the width of each column is
  determined as follows:

    1. A column element with a value other than 'auto' for the 'width'
    property sets the width for that column.

    2. Otherwise, a cell in the first row with a value other than
    'auto' for the 'width' property sets the width for that column. If
    the cell spans more than one column, the width is divided over the
    columns.

    3. Any remaining columns equally divide the remaining horizontal
    table space (minus borders or cell spacing).

  The width of the table is then the greater of the value of the
  'width' property for the table element and the sum of the column
  widths (plus cell spacing or borders). If the table is wider than
  the columns, the extra space should be distributed over the columns.


  In this manner, the user agent can begin to lay out the table once
  the entire first row has been received. Cells in subsequent rows do
  not affect column widths. Any cell that has content that overflows
  uses the 'overflow' property to determine whether to clip the
  overflow content.

_____________________________________________________

  This is not quite true when comparing to IE. IE always honors
  table-layout:fixed and treats a variable table width as 100%. Makes
  a lot of sense, and is implemented here the same way.

*/

FixedTableLayout::FixedTableLayout( RenderTable *table )
    : TableLayout ( table )
{
}

FixedTableLayout::~FixedTableLayout()
{
}

int FixedTableLayout::calcWidthArray()
{
    int usedWidth = 0;

    // iterate over all <col> elements
    RenderObject *child = table->firstChild();
    int nEffCols = table->numEffCols();
    width.resize( nEffCols );
    width.fill( Length( Auto ) );

#ifdef DEBUG_LAYOUT
    qDebug("FixedTableLayout::calcWidthArray()" );
    qDebug("    col elements:");
#endif

    int currentEffectiveColumn = 0;
    Length grpWidth;
    while ( child ) {
	if ( child->isTableCol() ) {
	    RenderTableCol *col = static_cast<RenderTableCol *>(child);
	    if ( col->firstChild() ) {
		grpWidth = col->style()->width();
	    } else {
		Length w = col->style()->width();
		if ( w.isAuto() )
		    w = grpWidth;
		int effWidth = 0;
		if ( w.isFixed() && w.isPositive() ) {
                    effWidth = w.value();
		    effWidth = qMin( effWidth, 32760 );
		}
#ifdef DEBUG_LAYOUT
		qDebug("    col element: effCol=%d, span=%d: %d w=%d type=%d",
                       cCol, span, effWidth,  w.rawValue(), w.type());
#endif
                int span = col->span();
                while (span) {
                    int spanInCurrentEffectiveColumn;
                    if (currentEffectiveColumn >= nEffCols) {
                        table->appendColumn(span);
			nEffCols++;
                        width.append(Length());
                        spanInCurrentEffectiveColumn = span;
                    } else {
                        if (span < table->spanOfEffCol(currentEffectiveColumn)) {
                            table->splitColumn(currentEffectiveColumn, span);
                            nEffCols++;
                            width.append(Length());
                        }
                        spanInCurrentEffectiveColumn = table->spanOfEffCol(currentEffectiveColumn);
		    }
		    if ( (w.isFixed() || w.isPercent()) && w.isPositive()) {
                        width[currentEffectiveColumn].setRawValue(w.type(), w.rawValue() * spanInCurrentEffectiveColumn);
                        usedWidth += effWidth * spanInCurrentEffectiveColumn;
#ifdef DEBUG_LAYOUT
			qDebug("    setting effCol %d (span=%d) to width %d(type=%d)",
                               cCol+i, eSpan, width[cCol+i].rawValue(), width[cCol+i].type() );
#endif
		    }
                    span -= spanInCurrentEffectiveColumn;
                    currentEffectiveColumn++;
		}
	    }
	} else {
	    break;
	}

	RenderObject *next = child->firstChild();
	if ( !next )
	    next = child->nextSibling();
	if ( !next && child->parent()->isTableCol() ) {
	    next = child->parent()->nextSibling();
	    grpWidth = Length();
	}
	child = next;
    }

#ifdef DEBUG_LAYOUT
    qDebug("    first row:");
#endif
    // iterate over the first row in case some are unspecified.
    RenderTableSection *section = table->head;
    if ( !section )
	section = table->firstBody;
    if ( !section )
	section = table->foot;
    if ( section && section->firstChild() ) {
	int cCol = 0;
	// get the first cell in the first row
	child = section->firstChild()->firstChild();
	while ( child ) {
	    if ( child->isTableCell() ) {
		RenderTableCell *cell = static_cast<RenderTableCell *>(child);
		Length w = cell->styleOrColWidth();
		int span = cell->colSpan();
		int effWidth = 0;
                // FIXME: This does not make sense (mixing percentages with absolute length)
		if ( (w.isFixed() || w.isPercent()) && w.isPositive()) {
                    effWidth = w.isPercent() ? w.rawValue() / PERCENT_SCALE_FACTOR : w.value();
		    effWidth = qMin( effWidth, 32760 );
		}
#ifdef DEBUG_LAYOUT
		qDebug("    table cell: effCol=%d, span=%d: %d",  cCol, span, effWidth);
#endif
		int usedSpan = 0;
		int i = 0;
		while ( usedSpan < span ) {
		    Q_ASSERT( cCol + i < nEffCols );
		    int eSpan = table->spanOfEffCol( cCol+i );
		    // only set if no col element has already set it.
		    if ( width[cCol+i].isAuto() && !w.isAuto() ) {
		    	width[cCol+i].setRawValue(w.type(), w.rawValue() * eSpan);
			usedWidth += effWidth*eSpan;
#ifdef DEBUG_LAYOUT
			qDebug("    setting effCol %d (span=%d) to width %d(type=%d)",
                               cCol+i, eSpan, width[cCol+i].rawValue(), width[cCol+i].type() );
#endif
		    }
#ifdef DEBUG_LAYOUT
		    else {
			qDebug("    width of col %d already defined (span=%d)", cCol, table->spanOfEffCol( cCol ) );
		    }
#endif
		    usedSpan += eSpan;
		    i++;
		}
		cCol += i;
	    } else {
		Q_ASSERT( false );
	    }
	    child = child->nextSibling();
	}
    }

    return usedWidth;

}

void FixedTableLayout::calcMinMaxWidth()
{
    // we might want to wait until we have all of the first row before
    // layouting for the first time.

    // only need to calculate the minimum width as the sum of the
    // cols/cells with a fixed width.
    //
    // The maximum width is qMax( minWidth, tableWidth ) if table
    // width is fixed. If table width is percent, we set maxWidth to
    // unlimited.

    int bs = table->bordersPaddingAndSpacing();
    int tableWidth = 0;
    if (table->style()->width().isFixed()) {
        tableWidth = table->calcBoxWidth(table->style()->width().value());
    }

    int mw = calcWidthArray() + bs;
    table->m_minWidth = qMin( qMax( mw, tableWidth ), 0x7fff );
    table->m_maxWidth = table->m_minWidth;

    if ( !tableWidth ) {
	bool haveNonFixed = false;
	for ( int i = 0; i < width.size(); i++ ) {
	    if ( !width[i].isFixed() ) {
		haveNonFixed = true;
		break;
	    }
	}
	if ( haveNonFixed )
	    table->m_maxWidth = 0x7fff;
    }
#ifdef DEBUG_LAYOUT
    qDebug("FixedTableLayout::calcMinMaxWidth: minWidth=%d, maxWidth=%d", table->m_minWidth, table->m_maxWidth );
#endif
}

void FixedTableLayout::layout()
{
    int tableWidth = table->width() - table->bordersPaddingAndSpacing();
    int available = tableWidth;
    int nEffCols = table->numEffCols();
#ifdef DEBUG_LAYOUT
    qDebug("FixedTableLayout::layout: tableWidth=%d, numEffCols=%d",  tableWidth, nEffCols);
#endif


    QVector<int> calcWidth;
    calcWidth.resize( nEffCols );
    calcWidth.fill( -1 );

    // first assign fixed width
    for ( int i = 0; i < nEffCols; i++ ) {
        if ( width[i].isFixed() ) {
            calcWidth[i] = width[i].value();
            available -= width[i].value();
        }
    }

    // assign percent width
    if ( available > 0 ) {
	int totalPercent = 0;
	for ( int i = 0; i < nEffCols; i++ )
	    if ( width[i].isPercent() )
		totalPercent += width[i].rawValue();

	// calculate how much to distribute to percent cells.
	int base = tableWidth * totalPercent / (100 * PERCENT_SCALE_FACTOR);
	if ( base > available )
	    base = available;

#ifdef DEBUG_LAYOUT
    qDebug("FixedTableLayout::layout: assigning percent width, base=%d, totalPercent=%d", base, totalPercent);
#endif
        for ( int i = 0; available > 0 && i < nEffCols; i++ ) {
            if ( width[i].isPercent() ) {
                // totalPercent may be 0 below if all %-width specified are 0%. (#172557)
                int w = totalPercent ? base * width[i].rawValue() / totalPercent : 0;
                available -= w;
                calcWidth[i] = w;
            }
        }
    }

    // assign variable width
    if ( available > 0 ) {
	int totalAuto = 0;
	for ( int i = 0; i < nEffCols; i++ )
	    if ( width[i].isAuto() )
		totalAuto++;

        for ( int i = 0; available > 0 && i < nEffCols; i++ ) {
            if ( width[i].isAuto() ) {
                // totalAuto may be 0 below if all the variable widths specified are 0.
                int w = totalAuto ? available / totalAuto : 0;
                available -= w;
                calcWidth[i] = w;
		totalAuto--;
            }
        }
    }

    for ( int i = 0; i < nEffCols; i++ )
	if ( calcWidth[i] < 0 )
	    calcWidth[i] = 0; // IE gives min 1 px...

    // spread extra space over columns
    if ( available > 0 ) {
        int total = nEffCols;
        // still have some width to spread
        int i = nEffCols;
        while ( i-- ) {
            int w = available / total;
            available -= w;
            total--;
            calcWidth[i] += w;
        }
    }

    int pos = 0;
    int hspacing = table->borderHSpacing();
    for ( int i = 0; i < nEffCols; i++ ) {
#ifdef DEBUG_LAYOUT
	qDebug("col %d: %d (width %d)", i, pos, calcWidth[i] );
#endif
	table->columnPos[i] = pos;
	pos += calcWidth[i] + hspacing;
    }
    table->columnPos[table->columnPos.size()-1] = pos;
}

// -------------------------------------------------------------------------
// -------------------------------------------------------------------------


AutoTableLayout::AutoTableLayout( RenderTable* table )
    : TableLayout( table )
{
    effWidthDirty = true;
    hasPercent = false;
}

AutoTableLayout::~AutoTableLayout()
{
}

/* recalculates the full structure needed to do layouting and minmax calculations.
   This is usually calculated on the fly, but needs to be done fully when table cells change
   dynamically
*/
void AutoTableLayout::recalcColumn( int effCol )
{
    Layout &l = layoutStruct[effCol];

    RenderObject *child = table->firstChild();
    // first we iterate over all rows.

    RenderTableCell *fixedContributor = 0;
    RenderTableCell *maxContributor = 0;

    while ( child ) {
	if ( child->isTableSection() ) {
	    RenderTableSection *section = static_cast<RenderTableSection *>(child);
	    int numRows = section->numRows();
	    //RenderTableCell *last = 0;
	    for ( int i = 0; i < numRows; i++ ) {
		RenderTableCell *cell = section->cellAt( i,  effCol );
		if ( cell == (RenderTableCell *)-1 )
		    continue;
                bool cellHasContent = cell && (cell->firstChild() || cell->style()->hasBorder() || cell->style()->hasPadding());
                if (cellHasContent)
                    l.emptyCellsOnly = false;
		if ( cell && cell->colSpan() == 1 ) {
                    // A cell originates in this column.  Ensure we have
                    // a min/max width of at least 1px for this column now.
                    l.minWidth = qMax(int( l.minWidth ), 1);
                    l.maxWidth = qMax(int( l.maxWidth ), cellHasContent ? 1 : 0);

		    if ( !cell->minMaxKnown() )
			cell->calcMinMaxWidth();
		    if ( cell->minWidth() > l.minWidth )
			l.minWidth = cell->minWidth();
		    if ( cell->maxWidth() > l.maxWidth ) {
			l.maxWidth = cell->maxWidth();
			maxContributor = cell;
		    }

		    Length w = cell->styleOrColWidth();
		    if (w.rawValue() > 32767)
		        w.setRawValue(32767);
                    if (w.isNegative())
                        w.setValue(0);
		    switch( w.type() ) {
		    case Fixed:
			// ignore width=0
			if ( w.isPositive() && !l.width.isPercent() ) {
                            int wval = cell->calcBoxWidth(w.value());
			    if ( l.width.isFixed() ) {
                                // Nav/IE weirdness
				if ((wval > l.width.value()) ||
				    ((l.width.value() == wval) && (maxContributor == cell))) {
				    l.width.setValue(wval);
				    fixedContributor = cell;
				}
			    } else {
                                l.width = Length( wval, Fixed );
				fixedContributor = cell;
			    }
			}
			break;
		    case Percent:
                        hasPercent = true;
                        if (w.isPositive() && (!l.width.isPercent() || w.rawValue() > l.width.rawValue()))
                            l.width = w;
			break;
		    case Relative:
			if (w.isAuto() || (w.isRelative() && w.value() > l.width.rawValue()))
				l.width = w;
		    default:
			break;
		    }
		} else {
		    if ( cell && (!effCol || section->cellAt( i, effCol-1 ) != cell) ) {
                        // This spanning cell originates in this column.  Ensure we have
                        // a min/max width of at least 1px for this column now.
                        l.minWidth = qMax(int( l.minWidth ), cellHasContent ? 1 : 0);
                        l.maxWidth = qMax(int( l.maxWidth ), 1);
			insertSpanCell( cell );
		    }
		    //last = cell;
		}
	    }
	}
	child = child->nextSibling();
    }

    // Nav/IE weirdness
    if ( l.width.isFixed() ) {
	if ( table->style()->htmlHacks()
	     && (l.maxWidth > l.width.value()) && (fixedContributor != maxContributor)) {
	    l.width = Length();
	    fixedContributor = 0;
	}
    }

    l.maxWidth = qMax(l.maxWidth, int(l.minWidth));
#ifdef DEBUG_LAYOUT
    qDebug("col %d, final min=%d, max=%d, width=%d(%d)", effCol, l.minWidth, l.maxWidth, l.width.rawValue(),  l.width.type() );
#endif

    // ### we need to add col elements aswell
}


void AutoTableLayout::fullRecalc()
{
    hasPercent = false;
    effWidthDirty = true;

    int nEffCols = table->numEffCols();
    layoutStruct.resize( nEffCols );
    layoutStruct.fill( Layout() );
    spanCells.fill( 0 );

    RenderObject *child = table->firstChild();
    Length grpWidth;
    int cCol = 0;
    while ( child ) {
	if ( child->isTableCol() ) {
	    RenderTableCol *col = static_cast<RenderTableCol *>(child);
	    int span = col->span();
	    if ( col->firstChild() ) {
		grpWidth = col->style()->width();
	    } else {
		Length w = col->style()->width();
		if ( w.isAuto() )
		    w = grpWidth;
		if ( (w.isFixed() || w.isPercent()) && w.isZero() )
		    w = Length();
		int cEffCol = table->colToEffCol( cCol );
#ifdef DEBUG_LAYOUT
		qDebug("    col element %d (eff=%d): Length=%d(%d), span=%d, effColSpan=%d",  cCol, cEffCol, w.rawValue(), w.type(), span, table->spanOfEffCol(cEffCol ) );
#endif
		if ( !w.isAuto() && span == 1 && cEffCol < nEffCols ) {
		    if ( table->spanOfEffCol( cEffCol ) == 1 ) {
			layoutStruct[cEffCol].width = w;
                        if (w.isFixed() && layoutStruct[cEffCol].maxWidth < w.value())
                            layoutStruct[cEffCol].maxWidth = w.value();
                    }
		}
		cCol += span;
	    }
	} else {
	    break;
	}

	RenderObject *next = child->firstChild();
	if ( !next )
	    next = child->nextSibling();
	if ( !next && child->parent()->isTableCol() ) {
	    next = child->parent()->nextSibling();
	    grpWidth = Length();
	}
	child = next;
    }


    for ( int i = 0; i < nEffCols; i++ )
	recalcColumn( i );
}

static bool shouldScaleColumns(RenderTable* table)
{
    // A special case.  If this table is not fixed width and contained inside
    // a cell, then don't bloat the maxwidth by examining percentage growth.
    bool scale = true;
    while (table) {
        Length tw = table->style()->width();
        if ((tw.isAuto() || tw.isPercent()) && !table->isPositioned()) {
            RenderBlock* cb = table->containingBlock();
            while (cb && !cb->isCanvas() && !cb->isTableCell() &&
                cb->style()->width().isAuto() && !cb->isPositioned())
                cb = cb->containingBlock();

            table = 0;
            if (cb && cb->isTableCell() &&
                (cb->style()->width().isAuto() || cb->style()->width().isPercent())) {
                if (tw.isPercent())
                    scale = false;
                else {
                    RenderTableCell* cell = static_cast<RenderTableCell*>(cb);
                    if (cell->colSpan() > 1 || cell->table()->style()->width().isAuto())
                        scale = false;
                    else
                        table = cell->table();
                }
            }
        }
        else
            table = 0;
    }
    return scale;
}

void AutoTableLayout::calcMinMaxWidth()
{
#ifdef DEBUG_LAYOUT
    qDebug("AutoTableLayout::calcMinMaxWidth");
#endif
    fullRecalc();

    int spanMaxWidth = calcEffectiveWidth();
    int minWidth = 0;
    int maxWidth = 0;
    int maxPercent = 0;
    int maxNonPercent = 0;

    int remainingPercent = 100 * PERCENT_SCALE_FACTOR;
    for ( int i = 0; i < layoutStruct.size(); i++ ) {
	minWidth += layoutStruct[i].effMinWidth;
	maxWidth += layoutStruct[i].effMaxWidth;
	if ( layoutStruct[i].effWidth.isPercent() ) {
            int percent = qMin(layoutStruct[i].effWidth.rawValue(), remainingPercent);
            int pw = ( layoutStruct[i].effMaxWidth * 100 * PERCENT_SCALE_FACTOR) / qMax(percent, 1);
            remainingPercent -= percent;
	    maxPercent = qMax( pw,  maxPercent );
	} else {
	    maxNonPercent += layoutStruct[i].effMaxWidth;
	}
    }

    if (shouldScaleColumns(table)) {
        maxNonPercent = (maxNonPercent * 100 * PERCENT_SCALE_FACTOR) / qMax(remainingPercent, 1);
	maxWidth = qMax( maxNonPercent, maxWidth );
	maxWidth = qMax( maxWidth, maxPercent );
    }

    maxWidth = qMax( maxWidth, spanMaxWidth );

    int bs = table->bordersPaddingAndSpacing();
    minWidth += bs;
    maxWidth += bs;

    Length tw = table->style()->width();
    if ( tw.isFixed() && tw.isPositive() ) {
        int width = table->calcBoxWidth(tw.value());
        minWidth = qMax( minWidth, width );
	maxWidth = minWidth;
    }

    table->m_maxWidth = qMin(maxWidth, 0x7fff);
    table->m_minWidth = qMin(minWidth, 0x7fff);
#ifdef DEBUG_LAYOUT
    qDebug("    minWidth=%d, maxWidth=%d", table->m_minWidth, table->m_maxWidth );
#endif
}

/*
  This method takes care of colspans.
  effWidth is the same as width for cells without colspans. If we have colspans, they get modified.
 */
int AutoTableLayout::calcEffectiveWidth()
{
    int tMaxWidth = 0;

    unsigned int nEffCols = layoutStruct.size();
    int hspacing = table->borderHSpacing();
#ifdef DEBUG_LAYOUT
    qDebug("AutoTableLayout::calcEffectiveWidth for %d cols", nEffCols );
#endif
    for ( unsigned int i = 0; i < nEffCols; i++ ) {
	layoutStruct[i].effWidth = layoutStruct[i].width;
	layoutStruct[i].effMinWidth = layoutStruct[i].minWidth;
	layoutStruct[i].effMaxWidth = layoutStruct[i].maxWidth;
    }

    for ( int i = 0; i < spanCells.size(); i++ ) {
	RenderTableCell *cell = spanCells[i];
	if ( !cell || cell == (RenderTableCell *)-1 )
	    break;
	int span = cell->colSpan();

	Length w = cell->styleOrColWidth();
	if ( !w.isRelative() && w.isZero() )
	    w = Length(); // make it Auto

	int col = table->colToEffCol( cell->col() );
	unsigned int lastCol = col;
	int cMinWidth = cell->minWidth() + hspacing;
	int cMaxWidth = cell->maxWidth() + hspacing;
	int totalPercent = 0;
	int minWidth = 0;
	int maxWidth = 0;
	bool allColsArePercent = true;
	bool allColsAreFixed = true;
	bool haveAuto = false;
        bool spanHasEmptyCellsOnly = true;
	int fixedWidth = 0;
#ifdef DEBUG_LAYOUT
	int cSpan = span;
#endif
	while ( lastCol < nEffCols && span > 0 ) {
	    switch( layoutStruct[lastCol].width.type() ) {
	    case Percent:
		totalPercent += layoutStruct[lastCol].width.rawValue();
		allColsAreFixed = false;
		break;
	    case Fixed:
                if (layoutStruct[lastCol].width.isPositive()) {
                    fixedWidth += layoutStruct[lastCol].width.value();
                    allColsArePercent = false;
                    // IE resets effWidth to Auto here, but this breaks the konqueror about page and seems to be some bad
                    // legacy behavior anyway. mozilla doesn't do this so I decided we don't either.
                    break;
                }
                // fall through
	    case Auto:
		haveAuto = true;
		// fall through
	    default:
                // If the column is a percentage width, do not let the spanning cell overwrite the
                // width value.  This caused a mis-rendering on amazon.com.
                // Sample snippet:
                // <table border=2 width=100%><
                //   <tr><td>1</td><td colspan=2>2-3</tr>
                //   <tr><td>1</td><td colspan=2 width=100%>2-3</td></tr>
                // </table>
                if (!layoutStruct[lastCol].effWidth.isPercent()) {
                    layoutStruct[lastCol].effWidth = Length();
                    allColsArePercent = false;
                }
                else
                    totalPercent += layoutStruct[lastCol].effWidth.rawValue();
                allColsAreFixed = false;
            }
            if (!layoutStruct[lastCol].emptyCellsOnly)
                spanHasEmptyCellsOnly = false;
            span -= table->spanOfEffCol( lastCol );
	    minWidth += layoutStruct[lastCol].effMinWidth;
	    maxWidth += layoutStruct[lastCol].effMaxWidth;
	    lastCol++;
	    cMinWidth -= hspacing;
	    cMaxWidth -= hspacing;
	}
#ifdef DEBUG_LAYOUT
	qDebug("    colspan cell %p at effCol %d, span %d, type %d, value %d cmin=%d min=%d fixedwidth=%d", cell, col, cSpan, w.type(), w.rawValue(), cMinWidth, minWidth, fixedWidth );
#endif

	// adjust table max width if needed
	if ( w.isPercent() ) {
	    if ( totalPercent > w.rawValue() || allColsArePercent ) {
		// can't satify this condition, treat as variable
		w = Length();
	    } else {
		int spanMax = qMax( maxWidth, cMaxWidth );
#ifdef DEBUG_LAYOUT
		qDebug("    adjusting tMaxWidth (%d): spanMax=%d, value=%d, totalPercent=%d", tMaxWidth, spanMax, w.rawValue(), totalPercent );
#endif
		tMaxWidth = qMax( tMaxWidth, spanMax * 100 * PERCENT_SCALE_FACTOR / w.rawValue() );

		// all non percent columns in the span get percent values to sum up correctly.
		int percentMissing = w.rawValue() - totalPercent;
		int totalWidth = 0;
		for ( unsigned int pos = col; pos < lastCol; pos++ ) {
		    if ( !(layoutStruct[pos].width.isPercent() ) )
			totalWidth += layoutStruct[pos].effMaxWidth;
		}

		for ( unsigned int pos = col; pos < lastCol && totalWidth > 0; pos++ ) {
		    if ( !(layoutStruct[pos].width.isPercent() ) ) {
			int percent = percentMissing * layoutStruct[pos].effMaxWidth / totalWidth;
#ifdef DEBUG_LAYOUT
			qDebug("   col %d: setting percent value %d effMaxWidth=%d totalWidth=%d", pos, percent, layoutStruct[pos].effMaxWidth, totalWidth );
#endif
			totalWidth -= layoutStruct[pos].effMaxWidth;
			percentMissing -= percent;
			if ( percent > 0 )
			    layoutStruct[pos].effWidth.setRawValue( Percent, percent );
			else
			    layoutStruct[pos].effWidth = Length();
		    }
		}

	    }
	}

	// make sure minWidth and maxWidth of the spanning cell are honoured
	if ( cMinWidth > minWidth ) {
	    if ( allColsAreFixed ) {
#ifdef DEBUG_LAYOUT
		qDebug("extending minWidth of cols %d-%d to %dpx currentMin=%d accroding to fixed sum %d", col, lastCol-1, cMinWidth, minWidth, fixedWidth );
#endif
		for ( unsigned int pos = col; fixedWidth > 0 && pos < lastCol; pos++ ) {
		    int w = qMax( int( layoutStruct[pos].effMinWidth ), cMinWidth * layoutStruct[pos].width.value() / fixedWidth );
#ifdef DEBUG_LAYOUT
		    qDebug("   col %d: min=%d, effMin=%d, new=%d", pos, layoutStruct[pos].effMinWidth, layoutStruct[pos].effMinWidth, w );
#endif
		    fixedWidth -= layoutStruct[pos].width.value();
		    cMinWidth -= w;
		    layoutStruct[pos].effMinWidth = w;
		}

            } else if ( allColsArePercent ) {
                int maxw = maxWidth;
                int minw = minWidth;
                int cminw = cMinWidth;

                for ( unsigned int pos = col; maxw > 0 && pos < lastCol; pos++ ) {
                    if ( layoutStruct[pos].effWidth.isPercent() && layoutStruct[pos].effWidth.isPositive() && fixedWidth <= cMinWidth) {
                        int w = layoutStruct[pos].effMinWidth;
                        w = qMax( w, cminw*layoutStruct[pos].effWidth.rawValue()/totalPercent );
                        w = qMin(layoutStruct[pos].effMinWidth+(cMinWidth-minw), w);
#ifdef DEBUG_LAYOUT
                        qDebug("   col %d: min=%d, effMin=%d, new=%d", pos, layoutStruct[pos].effMinWidth, layoutStruct[pos].effMinWidth, w );
#endif
                        maxw -= layoutStruct[pos].effMaxWidth;
                        minw -= layoutStruct[pos].effMinWidth;
                        cMinWidth -= w;
                        layoutStruct[pos].effMinWidth = w;
                    }
                }
	    } else {
#ifdef DEBUG_LAYOUT
		qDebug("extending minWidth of cols %d-%d to %dpx currentMin=%d", col, lastCol-1, cMinWidth, minWidth );
#endif
		int maxw = maxWidth;
		int minw = minWidth;

                // Give min to variable first, to fixed second, and to others third.
		for ( unsigned int pos = col; maxw > 0 && pos < lastCol; pos++ ) {
		    if ( layoutStruct[pos].width.isFixed() && haveAuto && fixedWidth <= cMinWidth ) {
			int w = qMax( int( layoutStruct[pos].effMinWidth ), layoutStruct[pos].width.value() );
			fixedWidth -= layoutStruct[pos].width.value();
                        minw -= layoutStruct[pos].effMinWidth;
#ifdef DEBUG_LAYOUT
			qDebug("   col %d: min=%d, effMin=%d, new=%d", pos, layoutStruct[pos].effMinWidth, layoutStruct[pos].effMinWidth, w );
#endif
			maxw -= layoutStruct[pos].effMaxWidth;
			cMinWidth -= w;
			layoutStruct[pos].effMinWidth = w;
		    }
		}

		for ( unsigned int pos = col; maxw > 0 && pos < lastCol && minw < cMinWidth; pos++ ) {
		    if ( !(layoutStruct[pos].width.isFixed() && haveAuto && fixedWidth <= cMinWidth) ) {
			int w = qMax( int( layoutStruct[pos].effMinWidth ), cMinWidth * layoutStruct[pos].effMaxWidth / maxw );
                        w = qMin(layoutStruct[pos].effMinWidth+(cMinWidth-minw), w);

#ifdef DEBUG_LAYOUT
			qDebug("   col %d: min=%d, effMin=%d, new=%d", pos, layoutStruct[pos].effMinWidth, layoutStruct[pos].effMinWidth, w );
#endif
			maxw -= layoutStruct[pos].effMaxWidth;
                        minw -= layoutStruct[pos].effMinWidth;
			cMinWidth -= w;
			layoutStruct[pos].effMinWidth = w;
		    }
		}
	    }
	}
	if ( !w.isPercent() ) {
	    if ( cMaxWidth > maxWidth ) {
#ifdef DEBUG_LAYOUT
		qDebug("extending maxWidth of cols %d-%d to %dpx", col, lastCol-1, cMaxWidth );
#endif
		for ( unsigned int pos = col; maxWidth > 0 && pos < lastCol; pos++ ) {
		    int w = qMax( int( layoutStruct[pos].effMaxWidth ), cMaxWidth * layoutStruct[pos].effMaxWidth / maxWidth );
#ifdef DEBUG_LAYOUT
		    qDebug("   col %d: max=%d, effMax=%d, new=%d", pos, layoutStruct[pos].effMaxWidth, layoutStruct[pos].effMaxWidth, w );
#endif
		    maxWidth -= layoutStruct[pos].effMaxWidth;
		    cMaxWidth -= w;
		    layoutStruct[pos].effMaxWidth = w;
		}
	    }
	} else {
	    for ( unsigned int pos = col; pos < lastCol; pos++ )
		layoutStruct[pos].maxWidth = qMax(layoutStruct[pos].maxWidth, int(layoutStruct[pos].minWidth) );
	}
        // treat span ranges consisting of empty cells only as if they had content
        if (spanHasEmptyCellsOnly)
            for (unsigned int pos = col; pos < lastCol; pos++)
                layoutStruct[pos].emptyCellsOnly = false;
    }
    effWidthDirty = false;

//     qDebug("calcEffectiveWidth: tMaxWidth=%d",  tMaxWidth );
    return tMaxWidth;
}

/* gets all cells that originate in a column and have a cellspan > 1
   Sorts them by increasing cellspan
*/
void AutoTableLayout::insertSpanCell( RenderTableCell *cell )
{
    if ( !cell || cell == (RenderTableCell *)-1 || cell->colSpan() == 1 )
	return;

//     qDebug("inserting span cell %p with span %d", cell, cell->colSpan() );
    int size = spanCells.size();
    if ( !size || spanCells[size-1] != 0 ) {
	spanCells.resize( size + 10 );
	for ( int i = 0; i < 10; i++ )
	    spanCells[size+i] = 0;
	size += 10;
    }

    // add them in sort. This is a slow algorithm, and a binary search or a fast sorting after collection would be better
    int pos = 0;
    int span = cell->colSpan();
    while ( pos < spanCells.size() && spanCells[pos] && span > spanCells[pos]->colSpan() )
	pos++;
    memmove( spanCells.data()+pos+1, spanCells.data()+pos, (size-pos-1)*sizeof( RenderTableCell * ) );
    spanCells[pos] = cell;
}


void AutoTableLayout::layout()
{
    // table layout based on the values collected in the layout structure.
    int tableWidth = table->width() - table->bordersPaddingAndSpacing();
    int available = tableWidth;
    int nEffCols = table->numEffCols();

    if ( nEffCols != layoutStruct.size() ) {
	qWarning("WARNING: nEffCols is not equal to layoutstruct!" );
	fullRecalc();
	nEffCols = table->numEffCols();
    }
#ifdef DEBUG_LAYOUT
    qDebug("AutoTableLayout::layout()");
#endif

    if ( effWidthDirty )
	calcEffectiveWidth();

#ifdef DEBUG_LAYOUT
    qDebug("    tableWidth=%d,  nEffCols=%d", tableWidth,  nEffCols );
    for ( int i = 0; i < nEffCols; i++ ) {
	qDebug("    effcol %d is of type %d value %d, minWidth=%d, maxWidth=%d",
               i, layoutStruct[i].width.type(), layoutStruct[i].width.rawValue(),
	       layoutStruct[i].minWidth, layoutStruct[i].maxWidth );
	qDebug("        effective: type %d value %d, minWidth=%d, maxWidth=%d",
               layoutStruct[i].effWidth.type(), layoutStruct[i].effWidth.rawValue(),
	       layoutStruct[i].effMinWidth, layoutStruct[i].effMaxWidth );
    }
#endif

    bool havePercent = false;
    bool haveRelative = false;
    int totalRelative = 0;
    int numAuto = 0;
    int numFixed = 0;
    int totalAuto = 0;
    int totalFixed = 0;
    int totalPercent = 0;
    int allocAuto = 0;
    int numAutoEmptyCellsOnly = 0;

    // fill up every cell with it's minWidth
    for ( int i = 0; i < nEffCols; i++ ) {
	int w = layoutStruct[i].effMinWidth;
	layoutStruct[i].calcWidth = w;
	available -= w;
	Length& width = layoutStruct[i].effWidth;
        switch( width.type()) {
        case Percent:
	    havePercent = true;
	    totalPercent += width.rawValue();
            break;
        case Relative:
	    haveRelative = true;
	    totalRelative += width.value();
            break;
        case Fixed:
            numFixed++;
            totalFixed += layoutStruct[i].effMaxWidth;
            // fall through
            break;
        case Auto:
        case Static:
            if (layoutStruct[i].emptyCellsOnly) 
                numAutoEmptyCellsOnly++;            
            else {
                numAuto++;
                totalAuto += layoutStruct[i].effMaxWidth;
                allocAuto += w;
            }
            break;
        }
    }

    // allocate width to percent cols
    if ( available > 0 && havePercent ) {
	for ( int i = 0; i < nEffCols; i++ ) {
	    const Length &width = layoutStruct[i].effWidth;
	    if ( width.isPercent() ) {
                int w = qMax ( int( layoutStruct[i].effMinWidth ), width.minWidth( tableWidth ) );
		available += layoutStruct[i].calcWidth - w;
		layoutStruct[i].calcWidth = w;
	    }
	}
	if ( totalPercent > 100 * PERCENT_SCALE_FACTOR ) {
	    // remove overallocated space from the last columns
	    int excess = tableWidth*(totalPercent-(100*PERCENT_SCALE_FACTOR)) / (100*PERCENT_SCALE_FACTOR);
	    for ( int i = nEffCols-1; i >= 0; i-- ) {
		if ( layoutStruct[i].effWidth.isPercent() ) {
		    int w = layoutStruct[i].calcWidth;
		    int reduction = qMin( w,  excess );
		    // the lines below might look inconsistent, but that's the way it's handled in mozilla
		    excess -= reduction;
		    int newWidth = qMax( int (layoutStruct[i].effMinWidth), w - reduction );
		    available += w - newWidth;
		    layoutStruct[i].calcWidth = newWidth;
		    //qDebug("col %d: reducing to %d px (reduction=%d)", i, newWidth, reduction );
		}
	    }
	}
    }
#ifdef DEBUG_LAYOUT
    qDebug("percent satisfied: available is %d", available);
#endif

    // then allocate width to fixed cols
    if ( available > 0 && numFixed) {
	for ( int i = 0; i < nEffCols; ++i ) {
	    const Length &width = layoutStruct[i].effWidth;
	    if ( width.isFixed() && width.value() > layoutStruct[i].calcWidth ) {
		available += layoutStruct[i].calcWidth - width.value();
		layoutStruct[i].calcWidth = width.value();
            }
	}
    }
#ifdef DEBUG_LAYOUT
    qDebug("fixed satisfied: available is %d", available);
#endif

    // now satisfy relative
    if ( available > 0 && haveRelative) {
	for ( int i = 0; i < nEffCols; i++ ) {
	    const Length &width = layoutStruct[i].effWidth;
	    if ( width.isRelative() && width.value() ) {
		// width=0* gets effMinWidth.
		int w = width.value()*tableWidth/totalRelative;
		available += layoutStruct[i].calcWidth - w;
		layoutStruct[i].calcWidth = w;
	    }
	}
    }

    // now satisfy variable
    if ( available > 0 && numAuto ) {
	available += allocAuto; // this gets redistributed
 	//qDebug("redistributing %dpx to %d variable columns. totalAuto=%d",  available,  numAuto,  totalAuto );
	for ( int i = 0; i < nEffCols; i++ ) {
	    const Length &width = layoutStruct[i].effWidth;
	    if ( width.isAuto() && totalAuto != 0 && !layoutStruct[i].emptyCellsOnly) {
		int w = qMax( int ( layoutStruct[i].calcWidth ),
                              available * layoutStruct[i].effMaxWidth / totalAuto );
		available -= w;
		totalAuto -= layoutStruct[i].effMaxWidth;
		layoutStruct[i].calcWidth = w;
	    }
	}
    }
#ifdef DEBUG_LAYOUT
    qDebug("variable satisfied: available is %d",  available );
#endif

    // spread over fixed columns
    if ( available > 0 && numFixed) {
        // still have some width to spread, distribute to fixed columns
        for ( int i = 0; i < nEffCols; i++ ) {
            const Length &width = layoutStruct[i].effWidth;
            if ( width.isFixed() ) {
                int w = available * layoutStruct[i].effMaxWidth / totalFixed;
                available -= w;
                totalFixed -= layoutStruct[i].effMaxWidth;
                layoutStruct[i].calcWidth += w;
            }
        }
    }

#ifdef DEBUG_LAYOUT
    qDebug("after fixed distribution: available=%d",  available );
#endif

    // spread over percent columns
    if ( available > 0 && hasPercent && totalPercent < 100 * PERCENT_SCALE_FACTOR) {
        // still have some width to spread, distribute weighted to percent columns
        for ( int i = 0; i < nEffCols; i++ ) {
            const Length &width = layoutStruct[i].effWidth;
            if ( width.isPercent() ) {
                int w = available * width.rawValue() / totalPercent;
                available -= w;
                totalPercent -= width.rawValue();
                layoutStruct[i].calcWidth += w;
                if (!available || !totalPercent) break;
            }
        }
    }

#ifdef DEBUG_LAYOUT
    qDebug("after percent distribution: available=%d",  available );
#endif

    // spread over the rest
    if ( available > 0 && nEffCols > numAutoEmptyCellsOnly) {
        int total = nEffCols;
        // still have some width to spread
        int i = nEffCols;
        while (  i-- ) {
            // variable columns with empty cells only don't get any width
            if (layoutStruct[i].width.isAuto() && layoutStruct[i].emptyCellsOnly)
                continue;
            int w = available / total;
            available -= w;
            total--;
            layoutStruct[i].calcWidth += w;
        }
    }

#ifdef DEBUG_LAYOUT
    qDebug("after equal distribution: available=%d",  available );
#endif
    // if we have overallocated, reduce every cell according to the difference between desired width and minwidth
    // this seems to produce to the pixel exaxt results with IE. Wonder is some of this also holds for width distributing.
    if ( available < 0 ) {
        // Need to reduce cells with the following prioritization:
        // (1) Auto
        // (2) Relative
        // (3) Fixed
        // (4) Percent
        // This is basically the reverse of how we grew the cells.
        if (available < 0) {
            int mw = 0;
            for ( int i = nEffCols-1; i >= 0; i-- ) {
                Length &width = layoutStruct[i].effWidth;
                if (width.isAuto())
                    mw += layoutStruct[i].calcWidth - layoutStruct[i].effMinWidth;
            }

            for ( int i = nEffCols-1; i >= 0 && mw > 0; i-- ) {
                Length &width = layoutStruct[i].effWidth;
                if (width.isAuto()) {
                    int minMaxDiff = layoutStruct[i].calcWidth-layoutStruct[i].effMinWidth;
                    int reduce = available * minMaxDiff / mw;
                    layoutStruct[i].calcWidth += reduce;
                    available -= reduce;
                    mw -= minMaxDiff;
                    if ( available >= 0 )
                        break;
                }
            }
        }

        if (available < 0 && haveRelative) {
            int mw = 0;
            for ( int i = nEffCols-1; i >= 0; i-- ) {
                Length &width = layoutStruct[i].effWidth;
                if (width.isRelative())
                    mw += layoutStruct[i].calcWidth - layoutStruct[i].effMinWidth;
            }

            for ( int i = nEffCols-1; i >= 0 && mw > 0; i-- ) {
                Length &width = layoutStruct[i].effWidth;
                if (width.isRelative()) {
                    int minMaxDiff = layoutStruct[i].calcWidth-layoutStruct[i].effMinWidth;
                    int reduce = available * minMaxDiff / mw;
                    layoutStruct[i].calcWidth += reduce;
                    available -= reduce;
                    mw -= minMaxDiff;
                    if ( available >= 0 )
                        break;
                }
            }
        }

        if (available < 0 && numFixed) {
            int mw = 0;
            for ( int i = nEffCols-1; i >= 0; i-- ) {
                Length &width = layoutStruct[i].effWidth;
                if (width.isFixed())
                    mw += layoutStruct[i].calcWidth - layoutStruct[i].effMinWidth;
            }

            for ( int i = nEffCols-1; i >= 0 && mw > 0; i-- ) {
                Length &width = layoutStruct[i].effWidth;
                if (width.isFixed()) {
                    int minMaxDiff = layoutStruct[i].calcWidth-layoutStruct[i].effMinWidth;
                    int reduce = available * minMaxDiff / mw;
                    layoutStruct[i].calcWidth += reduce;
                    available -= reduce;
                    mw -= minMaxDiff;
                    if ( available >= 0 )
                        break;
                }
            }
        }

        if (available < 0 && havePercent) {
            int mw = 0;
            for ( int i = nEffCols-1; i >= 0; i-- ) {
                Length &width = layoutStruct[i].effWidth;
                if (width.isPercent())
                    mw += layoutStruct[i].calcWidth - layoutStruct[i].effMinWidth;
            }

            for ( int i = nEffCols-1; i >= 0 && mw > 0; i-- ) {
                Length &width = layoutStruct[i].effWidth;
                if (width.isPercent()) {
                    int minMaxDiff = layoutStruct[i].calcWidth-layoutStruct[i].effMinWidth;
                    int reduce = available * minMaxDiff / mw;
                    layoutStruct[i].calcWidth += reduce;
                    available -= reduce;
                    mw -= minMaxDiff;
                    if ( available >= 0 )
                        break;
                }
            }
        }
    }

    //qDebug( "    final available=%d", available );

    int pos = 0;
    for ( int i = 0; i < nEffCols; i++ ) {
#ifdef DEBUG_LAYOUT
	qDebug("col %d: %d (width %d)", i, pos, layoutStruct[i].calcWidth );
#endif
	table->columnPos[i] = pos;
	pos += layoutStruct[i].calcWidth + table->borderHSpacing();
    }
    table->columnPos[table->columnPos.size()-1] = pos;

}

#undef DEBUG_LAYOUT
