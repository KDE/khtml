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
#ifndef TABLE_LAYOUT_H
#define TABLE_LAYOUT_H


#include <misc/khtmllayout.h>
#include <QVector>
namespace khtml {

class RenderTable;
class RenderTableCell;

// -------------------------------------------------------------------------

class TableLayout
{
public:
    TableLayout( RenderTable *t ) : table( t ) {}
    virtual ~TableLayout() {}

    virtual void calcMinMaxWidth() = 0;
    virtual void layout() = 0;

protected:
    RenderTable *table;
};

// -------------------------------------------------------------------------

class FixedTableLayout : public TableLayout
{
public:
    FixedTableLayout( RenderTable *table );
    ~FixedTableLayout();

    void calcMinMaxWidth();
    void layout();

protected:
    int calcWidthArray();

    QVector<Length> width;
};

// -------------------------------------------------------------------------

class AutoTableLayout : public TableLayout
{
public:
    AutoTableLayout( RenderTable *table );
    ~AutoTableLayout();

    void calcMinMaxWidth();
    void layout();


protected:
    void fullRecalc();
    void recalcColumn( int effCol );
    int calcEffectiveWidth();
    void insertSpanCell( RenderTableCell *cell );

    struct Layout {
	Layout() : minWidth( 1 ), maxWidth( 1 ),
		   effMinWidth( 0 ), effMaxWidth( 0 ),
		   calcWidth( 0 ), emptyCellsOnly(true) {}
	Length width;
	Length effWidth;
	short minWidth;
	int maxWidth;
	short effMinWidth;
	int effMaxWidth;
	int calcWidth;
	bool emptyCellsOnly;
    };

    QVector<Layout> layoutStruct;
    QVector<RenderTableCell *>spanCells;
    bool hasPercent : 1;
    mutable bool effWidthDirty : 1;
};

}
#endif
