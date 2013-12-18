/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2002-2003 Apple Computer, Inc.
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
#ifndef RENDER_BOX_H
#define RENDER_BOX_H

#include "render_container.h"

namespace khtml {

enum WidthType { Width, MinWidth, MaxWidth };
enum HeightType { Height, MinHeight, MaxHeight };

class RenderBlock;
class RenderTableCell;

class RenderBox : public RenderContainer
{
    friend class RenderTableCell;

// combines ElemImpl & PosElImpl (all rendering objects are positioned)
// should contain all border and padding handling

public:
    RenderBox(DOM::NodeImpl* node);
    virtual ~RenderBox();

    virtual const char *renderName() const { return "RenderBox"; }
    virtual bool isBox() const { return true; }

    virtual void setStyle(RenderStyle *style);
    virtual void paint(PaintInfo& i, int _tx, int _ty);

    virtual void close();

    virtual InlineBox* createInlineBox(bool makePlaceHolderBox, bool isRootLineBox);
    virtual void deleteInlineBoxes(RenderArena* arena=0);
    virtual void dirtyInlineBoxes(bool fullLayout, bool isRootLineBox = false);
    virtual void removeInlineBox(InlineBox* _box) { if (m_placeHolderBox == _box) m_placeHolderBox = 0; }
    
    virtual void removeChild(RenderObject*);

    virtual void detach();

    virtual short minWidth() const { return m_minWidth; }
    virtual int maxWidth() const { return m_maxWidth; }

    virtual short contentWidth() const;
    virtual int contentHeight() const;

    virtual bool absolutePosition(int &xPos, int &yPos, bool f = false) const;

    virtual void setPos( int xPos, int yPos );

    virtual int xPos() const { return m_x; }
    virtual int yPos() const { return m_y; }
    virtual short width() const;
    virtual int height() const;

    virtual short marginTop() const { return m_marginTop; }
    virtual short marginBottom() const { return m_marginBottom; }
    virtual short marginLeft() const { return m_marginLeft; }
    virtual short marginRight() const { return m_marginRight; }

    virtual void setWidth( int width );
    virtual void setHeight( int height );

    virtual void position(InlineBox* box, int from, int len, bool reverse);

    virtual int highestPosition(bool includeOverflowInterior=true, bool includeSelf=true) const;
    virtual int lowestPosition(bool includeOverflowInterior=true, bool includeSelf=true) const;
    virtual int rightmostPosition(bool includeOverflowInterior=true, bool includeSelf=true) const;
    virtual int leftmostPosition(bool includeOverflowInterior=true, bool includeSelf=true) const;

    virtual void repaint(Priority p=NormalPriority);

    virtual void repaintRectangle(int x, int y, int w, int h, Priority p=NormalPriority, bool f=false);

    virtual short containingBlockWidth(RenderObject* providedCB=0) const;
    void relativePositionOffset(int &tx, int &ty) const;

    virtual void calcWidth();
    virtual void calcHeight();

    virtual short calcReplacedWidth() const;
    virtual int   calcReplacedHeight() const;

    virtual int availableHeight() const;
    virtual int availableWidth() const;

    void calcVerticalMargins();

    virtual RenderLayer* layer() const { return m_layer; }

    void setStaticX(short staticX);
    void setStaticY(int staticY);
    int staticX() const { return m_staticX; }
    int staticY() const { return m_staticY; }

    virtual void caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const;

    void calcHorizontalMargins(const Length& ml, const Length& mr, int cw);
    RenderBlock* createAnonymousBlock();

    virtual int pageTopAfter(int y) const;
    virtual int crossesPageBreak(int t, int b) const;
    
    virtual bool handleEvent(const DOM::EventImpl& ev);

    int calcBoxWidth(int w) const;
    int calcBoxHeight(int h) const;
    virtual int calcContentWidth(int w) const;
    virtual int calcContentHeight(int h) const;

    InlineBox *placeHolderBox() { return m_placeHolderBox; }
    void setPlaceHolderBox(InlineBox* placeHolder) { m_placeHolderBox = placeHolder; /* assert !m_placeHolderBox */ }
    QRect getFixedBackgroundImageRect( const BackgroundLayer* bglayer, int& sx, int& sy, int& scaledImageWidth, int& scaledImageHeight );

protected:
    int calcWidthUsing(WidthType widthType, int cw, LengthType& lengthType);
    int calcHeightUsing(const Length& height);
    int calcReplacedWidthUsing(WidthType widthType) const;
    int calcReplacedHeightUsing(HeightType heightType) const;
    int calcPercentageHeight(const Length& height) const;
    int availableHeightUsing(const Length& h) const;
    int availableWidthUsing(const Length& w) const;
    int calcImplicitContentHeight() const;
    bool hasImplicitHeight() const {
        return isPositioned() && !style()->top().isAuto() && !style()->bottom().isAuto();
    }

protected:
    virtual void paintBoxDecorations(PaintInfo& paintInfo, int _tx, int _ty);
    void paintRootBoxDecorations( PaintInfo& paintInfo, int _tx, int _ty);

    void paintAllBackgrounds(QPainter *p, const QColor& c, const BackgroundLayer* bgLayer, QRect clipr, int _tx, int _ty, int w, int h);
    virtual void paintOneBackground(QPainter *p, const QColor& c, const BackgroundLayer* bgLayer, QRect clipr, int _tx, int _ty, int w, int h);
    virtual void paintBackgroundExtended(QPainter* /*p*/, const QColor& /*c*/, const BackgroundLayer* /*bgLayer*/,
                                         QRect clipr, int /*_tx*/, int /*_ty*/,
                                         int /*w*/, int /*height*/, int /*bleft*/, int /*bright*/, int /*pleft*/, int /*pright*/,
                                         int /*btop*/, int /*bbottom*/, int /*ptop*/, int /*pbottom*/ );
    void outlineBox(QPainter *p, int _tx, int _ty, const char *color = "red");

    void calcAbsoluteHorizontal();
    void calcAbsoluteVertical();
    void calcAbsoluteHorizontalValues(Length width, const RenderObject* cb, EDirection containerDirection,
                                      const int containerWidth, const int bordersPlusPadding,
                                      const Length left, const Length right, const Length marginLeft, const Length marginRight,
                                      short& widthValue, short& marginLeftValue, short& marginRightValue, short& xPos);
    void calcAbsoluteVerticalValues(Length height, const RenderObject* cb,
                                    const int containerHeight, const int bordersPlusPadding,
                                    const Length top, const Length bottom, const Length marginTop, const Length marginBottom,
                                    int& heightValue, short& marginTopValue, short& marginBottomValue, int& yPos);

    void calcAbsoluteVerticalReplaced();
    void calcAbsoluteHorizontalReplaced();

    QPainterPath borderRadiusClipPath(const BackgroundLayer *bgLayer, int _tx, int _ty, int w, int h,
                                      int bleft, int bright, int btop, int bbottom) const;
    QRect overflowClipRect(int tx, int ty);
    QRect clipRect(int tx, int ty);

    void restructureParentFlow();
    void detachRemainingChildren();


    // the actual height of the contents + borders + padding (border-box)
    int m_height;
    int m_y;

    short m_width;
    short m_x;

    short m_marginTop;
    short m_marginBottom;

    short m_marginLeft;
    short m_marginRight;

    /*
     * the minimum width the element needs, to be able to render
     * its content without clipping
     */
    short m_minWidth;
    /* The maximum width the element can fill horizontally
     * ( = the width of the element with line breaking disabled)
     */
    int m_maxWidth;

    // Cached normal flow values for absolute positioned elements with static left/top values.
    short m_staticX;
    int m_staticY;

    RenderLayer *m_layer;

    /* A box used to represent this object on a line
     * when its inner content isn't contextually relevant
     * (e.g replaced or positioned elements)
     */
    InlineBox *m_placeHolderBox;
};


} //namespace

#endif
