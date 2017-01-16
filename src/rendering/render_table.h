/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1997 Martin Jones (mjones@kde.org)
 *           (C) 1997 Torben Weis (weis@kde.org)
 *           (C) 1998 Waldo Bastian (bastian@kde.org)
 *           (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2003 Apple Computer, Inc.
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
 *
 */
#ifndef RENDER_TABLE_H
#define RENDER_TABLE_H

#include <QColor>
#include <QVector>

#include "rendering/render_box.h"
#include "rendering/render_block.h"
#include "rendering/render_style.h"

#include "misc/khtmllayout.h"

namespace khtml
{

class RenderTable;
class RenderTableSection;
class RenderTableRow;
class RenderTableCell;
class RenderTableCol;
class TableLayout;

class RenderTable : public RenderBlock
{
public:

    RenderTable(DOM::NodeImpl *node);
    ~RenderTable();

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderTable";
    }

    void setStyle(RenderStyle *style) Q_DECL_OVERRIDE;

    bool isTable() const Q_DECL_OVERRIDE
    {
        return true;
    }

    int getColumnPos(int col) const
    {
        return columnPos[col];
    }

    int borderHSpacing() const
    {
        return hspacing;
    }
    int borderVSpacing() const
    {
        return vspacing;
    }

    bool collapseBorders() const
    {
        return style()->borderCollapse();
    }
    int borderLeft() const Q_DECL_OVERRIDE;
    int borderRight() const Q_DECL_OVERRIDE;
    int borderTop() const Q_DECL_OVERRIDE;
    int borderBottom() const Q_DECL_OVERRIDE;
    int paddingLeft() const Q_DECL_OVERRIDE
    {
        return collapseBorders() ? 0 : RenderBlock::paddingLeft();
    }
    int paddingRight() const Q_DECL_OVERRIDE
    {
        return collapseBorders() ? 0 : RenderBlock::paddingRight();
    }
    int paddingTop() const Q_DECL_OVERRIDE
    {
        return collapseBorders() ? 0 : RenderBlock::paddingTop();
    }
    int paddingBottom() const Q_DECL_OVERRIDE
    {
        return collapseBorders() ? 0 : RenderBlock::paddingBottom();
    }

    const QColor &bgColor() const
    {
        return style()->backgroundColor();
    }

    uint cellPadding() const
    {
        return padding;
    }
    void setCellPadding(uint p)
    {
        padding = p;
    }

    // overrides
    void addChild(RenderObject *child, RenderObject *beforeChild = nullptr) Q_DECL_OVERRIDE;
    void paint(PaintInfo &, int tx, int ty) Q_DECL_OVERRIDE;
    void paintBoxDecorations(PaintInfo &, int _tx, int _ty) Q_DECL_OVERRIDE;
    void layout() Q_DECL_OVERRIDE;
    void calcMinMaxWidth() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;

    short lineHeight(bool b) const Q_DECL_OVERRIDE;
    short baselinePosition(bool b) const Q_DECL_OVERRIDE;

    virtual void setCellWidths();

    void calcWidth() Q_DECL_OVERRIDE;

    QList< QRectF > getClientRects() Q_DECL_OVERRIDE;

    virtual FindSelectionResult checkSelectionPoint(int _x, int _y, int _tx, int _ty,
            DOM::NodeImpl *&node, int &offset,
            SelPointState &);

#ifdef ENABLE_DUMP
    void dump(QTextStream &stream, const QString &ind) const Q_DECL_OVERRIDE;
#endif
    struct ColumnStruct {
        enum {
            WidthUndefined = 0xffff
        };
        ColumnStruct()
        {
            span = 1;
            width = WidthUndefined;
        }
        ushort span;
        ushort width; // the calculated position of the column
    };

    QVector<int> columnPos;
    QVector<ColumnStruct> columns;

    void splitColumn(int pos, int firstSpan);
    void appendColumn(int span);
    int numEffCols() const
    {
        return columns.size();
    }
    int spanOfEffCol(int effCol) const
    {
        return columns[effCol].span;
    }
    int colToEffCol(int col) const
    {
        int c = 0;
        int i = 0;
        while (c < col && i < (int)columns.size()) {
            c += columns[i].span;
            i++;
        }
        return i;
    }
    int effColToCol(int effCol) const
    {
        int c = 0;
        for (int i = 0; i < effCol; i++) {
            c += columns[i].span;
        }
        return c;
    }

    int bordersPaddingAndSpacing() const
    {
        return borderLeft() + borderRight() +
               (collapseBorders() ? 0 : (paddingLeft() + paddingRight() + (numEffCols() + 1) * borderHSpacing()));
    }

    RenderTableCol *colElement(int col, bool *startEdge = nullptr, bool *endEdge = nullptr) const;

    void setNeedSectionRecalc()
    {
        needSectionRecalc = true;
    }

    RenderObject *removeChildNode(RenderObject *child) Q_DECL_OVERRIDE;

    RenderTableSection *sectionAbove(const RenderTableSection *, bool skipEmptySections = false);
    RenderTableSection *sectionBelow(const RenderTableSection *, bool skipEmptySections = false);

    RenderTableCell *cellAbove(const RenderTableCell *cell);
    RenderTableCell *cellBelow(const RenderTableCell *cell);
    RenderTableCell *cellBefore(const RenderTableCell *cell);
    RenderTableCell *cellAfter(const RenderTableCell *cell);

    CollapsedBorderValue *currentBorderStyle()
    {
        return m_currentBorder;
    }

    RenderTableSection *firstBodySection() const
    {
        return firstBody;
    }
    RenderFlow         *caption() const
    {
        return tCaption;
    }

protected:

    void recalcSections();

    friend class AutoTableLayout;
    friend class FixedTableLayout;

    RenderFlow         *tCaption;
    RenderTableSection *head;
    RenderTableSection *foot;
    RenderTableSection *firstBody;

    TableLayout *tableLayout;

    CollapsedBorderValue *m_currentBorder;

    bool has_col_elems      : 1;
    uint needSectionRecalc  : 1;
    uint padding        : 22;

    ushort hspacing;
    ushort vspacing;

    friend class TableSectionIterator;
};

// -------------------------------------------------------------------------

class RenderTableSection : public RenderBox
{
public:
    RenderTableSection(DOM::NodeImpl *node);
    ~RenderTableSection();
    void detach() Q_DECL_OVERRIDE;

    void setStyle(RenderStyle *style) Q_DECL_OVERRIDE;

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderTableSection";
    }

    // overrides
    void addChild(RenderObject *child, RenderObject *beforeChild = nullptr) Q_DECL_OVERRIDE;
    bool isTableSection() const Q_DECL_OVERRIDE
    {
        return true;
    }

    short lineHeight(bool) const Q_DECL_OVERRIDE
    {
        return 0;
    }
    void position(InlineBox *, int, int, bool) Q_DECL_OVERRIDE {}

    virtual FindSelectionResult checkSelectionPoint(int _x, int _y, int _tx, int _ty,
            DOM::NodeImpl *&node, int &offset,
            SelPointState &);

#ifdef ENABLE_DUMP
    void dump(QTextStream &stream, const QString &ind) const Q_DECL_OVERRIDE;
#endif

    void addCell(RenderTableCell *cell, RenderTableRow *row);

    void setCellWidths();
    void calcRowHeight();
    int layoutRows(int height);

    RenderTable *table() const
    {
        return static_cast<RenderTable *>(parent());
    }

    typedef QVector<RenderTableCell *> Row;
    struct RowStruct {
        Row *row;
        RenderTableRow *rowRenderer;
        int baseLine;
        Length height;
        bool needFlex;
    };

    RenderTableCell *&cellAt(int row,  int col)
    {
        return (*(grid[row].row))[col];
    }
    RenderTableCell *cellAt(int row,  int col) const
    {
        return (*(grid[row].row))[col];
    }

    int lowestPosition(bool includeOverflowInterior, bool includeSelf) const Q_DECL_OVERRIDE;
    int rightmostPosition(bool includeOverflowInterior, bool includeSelf) const Q_DECL_OVERRIDE;
    int leftmostPosition(bool includeOverflowInterior, bool includeSelf) const Q_DECL_OVERRIDE;
    int highestPosition(bool includeOverflowInterior, bool includeSelf) const Q_DECL_OVERRIDE;

    int borderLeft() const Q_DECL_OVERRIDE
    {
        return table()->collapseBorders() ? 0 : RenderBox::borderLeft();
    }
    int borderRight() const Q_DECL_OVERRIDE
    {
        return table()->collapseBorders() ? 0 : RenderBox::borderRight();
    }
    int borderTop() const Q_DECL_OVERRIDE
    {
        return table()->collapseBorders() ? 0 : RenderBox::borderTop();
    }
    int borderBottom() const Q_DECL_OVERRIDE
    {
        return table()->collapseBorders() ? 0 : RenderBox::borderBottom();
    }

    void paint(PaintInfo &i, int tx, int ty) Q_DECL_OVERRIDE;

    int numRows() const
    {
        return grid.size();
    }
    int getBaseline(int row)
    {
        return grid[row].baseLine;
    }

    void setNeedCellRecalc()
    {
        needCellRecalc = true;
        table()->setNeedSectionRecalc();
    }

    RenderObject *removeChildNode(RenderObject *child) Q_DECL_OVERRIDE;

    bool canClear(RenderObject *child, PageBreakLevel level) Q_DECL_OVERRIDE;
    void addSpaceAt(int pos, int dy);

    bool nodeAtPoint(NodeInfo &info, int x, int y, int tx, int ty, HitTestAction action, bool inside) Q_DECL_OVERRIDE;

    // this gets a cell grid data structure. changing the number of
    // columns is done by the table
    QVector<RowStruct> grid;
    QVector<int> rowPos;

    int numColumns() const;

    //QMap< lastInsertedColumn, cell >
    QMap< int, RenderTableCell * > cellsWithColSpanZero;
    //QMap< nextRowToInsert, cell >
    QMap< int, RenderTableCell * > cellsWithRowSpanZero;

    int cRow;
    int cCol;
    bool needCellRecalc;

    void recalcCells();
protected:
    void ensureRows(int numRows);
    void clearGrid();
    bool emptyRow(int rowNum);
    bool flexCellChildren(RenderObject *p) const;

    friend class TableSectionIterator;
};

// -------------------------------------------------------------------------

class RenderTableRow : public RenderBox
{
public:
    RenderTableRow(DOM::NodeImpl *node);

    void detach() Q_DECL_OVERRIDE;

    void setStyle(RenderStyle *) Q_DECL_OVERRIDE;
    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderTableRow";
    }
    bool isTableRow() const Q_DECL_OVERRIDE
    {
        return true;
    }
    void addChild(RenderObject *child, RenderObject *beforeChild = nullptr) Q_DECL_OVERRIDE;

    short offsetWidth() const Q_DECL_OVERRIDE;
    int offsetHeight() const Q_DECL_OVERRIDE;
    int offsetLeft() const Q_DECL_OVERRIDE;
    int offsetTop() const Q_DECL_OVERRIDE;

    short lineHeight(bool) const Q_DECL_OVERRIDE
    {
        return 0;
    }
    void position(InlineBox *, int, int, bool) Q_DECL_OVERRIDE {}

    bool nodeAtPoint(NodeInfo &info, int x, int y, int tx, int ty, HitTestAction action, bool inside) Q_DECL_OVERRIDE;

    void layout() Q_DECL_OVERRIDE;

    RenderObject *removeChildNode(RenderObject *child) Q_DECL_OVERRIDE;

    // The only time rows get a layer is when they have transparency.
    bool requiresLayer() const Q_DECL_OVERRIDE
    {
        return style()->opacity() < 1.0f;
    }
    void paint(PaintInfo &i, int tx, int ty) Q_DECL_OVERRIDE;

    void paintRow(PaintInfo &i, int tx, int ty, int w, int h);

    RenderTable *table() const
    {
        return static_cast<RenderTable *>(parent()->parent());
    }
    RenderTableSection *section() const
    {
        return static_cast<RenderTableSection *>(parent());
    }
};

// -------------------------------------------------------------------------

class RenderTableCell : public RenderBlock
{
public:
    RenderTableCell(DOM::NodeImpl *node);

    void layout() Q_DECL_OVERRIDE;
    void detach() Q_DECL_OVERRIDE;

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderTableCell";
    }
    bool isTableCell() const Q_DECL_OVERRIDE
    {
        return true;
    }

    // ### FIX these two...
    long cellIndex() const
    {
        return 0;
    }
    void setCellIndex(long) { }

    unsigned short colSpan() const
    {
        return cSpan;
    }
    void setColSpan(unsigned short c)
    {
        cSpan = c;
    }

    unsigned short rowSpan() const
    {
        return rSpan;
    }
    void setRowSpan(unsigned short r)
    {
        rSpan = r;
    }

    int col() const
    {
        return _col;
    }
    void setCol(int col)
    {
        _col = col;
    }
    int row() const
    {
        return _row;
    }
    void setRow(int r)
    {
        _row = r;
    }

    Length styleOrColWidth();

    // overrides
    void calcMinMaxWidth() Q_DECL_OVERRIDE;
    void calcWidth() Q_DECL_OVERRIDE;
    void setWidth(int width) Q_DECL_OVERRIDE;
    void setStyle(RenderStyle *style) Q_DECL_OVERRIDE;
    bool requiresLayer() const Q_DECL_OVERRIDE;

    int borderLeft() const Q_DECL_OVERRIDE;
    int borderRight() const Q_DECL_OVERRIDE;
    int borderTop() const Q_DECL_OVERRIDE;
    int borderBottom() const Q_DECL_OVERRIDE;

    CollapsedBorderValue collapsedLeftBorder(bool rtl) const;
    CollapsedBorderValue collapsedRightBorder(bool rtl) const;
    CollapsedBorderValue collapsedTopBorder() const;
    CollapsedBorderValue collapsedBottomBorder() const;
    void collectBorders(QList<CollapsedBorderValue> &borderStyles) Q_DECL_OVERRIDE;

    void updateFromElement() Q_DECL_OVERRIDE;

    void setCellTopExtra(int p)
    {
        _topExtra = p;
    }
    void setCellBottomExtra(int p)
    {
        _bottomExtra = p;
    }
    int cellTopExtra() const
    {
        return _topExtra;
    }
    int cellBottomExtra() const
    {
        return _bottomExtra;
    }

    int pageTopAfter(int x) const Q_DECL_OVERRIDE;

    void paint(PaintInfo &i, int tx, int ty) Q_DECL_OVERRIDE;

    void paintCollapsedBorder(QPainter *p, int x, int y, int w, int h);
    void paintBackgroundsBehindCell(PaintInfo &i, int _tx, int _ty, RenderObject *backgroundObject);

    void close() Q_DECL_OVERRIDE;

    // lie position to outside observers
    int yPos() const Q_DECL_OVERRIDE
    {
        return m_y + _topExtra;
    }

    void repaintRectangle(int x, int y, int w, int h, Priority p = NormalPriority, bool f = false) Q_DECL_OVERRIDE;

    short baselinePosition(bool = false) const Q_DECL_OVERRIDE;

    bool nodeAtPoint(NodeInfo &info, int _x, int _y, int _tx, int _ty, HitTestAction hitTestAction, bool inside) Q_DECL_OVERRIDE;

    RenderTable *table() const
    {
        return static_cast<RenderTable *>(parent()->parent()->parent());
    }
    RenderTableSection *section() const
    {
        return static_cast<RenderTableSection *>(parent()->parent());
    }

#ifdef ENABLE_DUMP
    void dump(QTextStream &stream, const QString &ind) const Q_DECL_OVERRIDE;
#endif

    bool widthChanged()
    {
        bool retval = m_widthChanged;
        m_widthChanged = false;
        return retval;
    }

    int cellPercentageHeight() const
    {
        return m_percentageHeight;
    }
    void setCellPercentageHeight(int h)
    {
        m_percentageHeight = h;
    }
    bool hasFlexedAnonymous() const
    {
        return m_hasFlexedAnonymous;
    }
    void setHasFlexedAnonymous(bool b = true)
    {
        m_hasFlexedAnonymous = b;
    }

protected:
    void paintBoxDecorations(PaintInfo &p, int _tx, int _ty) Q_DECL_OVERRIDE;
    int borderTopExtra() const Q_DECL_OVERRIDE
    {
        return _topExtra;
    }
    int borderBottomExtra() const Q_DECL_OVERRIDE
    {
        return _bottomExtra;
    }

    int _row;
    int _col;
    ushort rSpan;
    ushort cSpan;
    int _topExtra;
    signed int _bottomExtra : 30;
    bool m_widthChanged : 1;
    bool m_hasFlexedAnonymous : 1;
    int m_percentageHeight;
};

// -------------------------------------------------------------------------

class RenderTableCol : public RenderBox
{
public:
    RenderTableCol(DOM::NodeImpl *node);

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderTableCol";
    }

    bool isTableCol() const Q_DECL_OVERRIDE
    {
        return true;
    }

    short lineHeight(bool) const Q_DECL_OVERRIDE
    {
        return 0;
    }
    void position(InlineBox *, int, int, bool) Q_DECL_OVERRIDE {}
    void layout() Q_DECL_OVERRIDE {}
    bool requiresLayer() const Q_DECL_OVERRIDE
    {
        return false;
    }

    void updateFromElement() Q_DECL_OVERRIDE;

#ifdef ENABLE_DUMP
    void dump(QTextStream &stream, const QString &ind) const Q_DECL_OVERRIDE;
#endif

    int span() const
    {
        return m_span;
    }
    void setSpan(int s)
    {
        m_span = s;
    }

private:
    int m_span;
};

// -------------------------------------------------------------------------

/** This class provides an iterator to iterate through the sections of a
 * render table in their visual order.
 *
 * In HTML, sections are specified in the order of THEAD, TFOOT, and TBODY.
 * Visually, TBODY sections appear between THEAD and TFOOT, which this iterator
 * takes into regard.
 * @author Leo Savernik
 * @internal
 */
class TableSectionIterator
{
public:

    /**
     * Initializes a new iterator
     * @param table table whose sections to iterate
     * @param fromEnd @p true, begin with last section, @p false, begin with
     *    first section.
     */
    TableSectionIterator(RenderTable *table, bool fromEnd = false);

    /**
     * Initializes a new iterator
     * @param start table section to start with.
     */
    TableSectionIterator(RenderTableSection *start) : sec(start) {}

    /**
     * Uninitialized iterator.
     */
    TableSectionIterator() {}

    /** Returns the current section, or @p 0 if the end has been reached.
     */
    RenderTableSection *operator *() const
    {
        return sec;
    }

    /** Moves to the next section in visual order. */
    TableSectionIterator &operator ++();

    /** Moves to the previous section in visual order. */
    TableSectionIterator &operator --();

private:
    RenderTableSection *sec;
};

}
#endif

