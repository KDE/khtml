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

namespace khtml
{

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
    RenderBox(DOM::NodeImpl *node);
    virtual ~RenderBox();

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderBox";
    }
    bool isBox() const Q_DECL_OVERRIDE
    {
        return true;
    }

    void setStyle(RenderStyle *style) Q_DECL_OVERRIDE;
    void paint(PaintInfo &i, int _tx, int _ty) Q_DECL_OVERRIDE;

    void close() Q_DECL_OVERRIDE;

    InlineBox *createInlineBox(bool makePlaceHolderBox, bool isRootLineBox) Q_DECL_OVERRIDE;
    void deleteInlineBoxes(RenderArena *arena = 0) Q_DECL_OVERRIDE;
    void dirtyInlineBoxes(bool fullLayout, bool isRootLineBox = false) Q_DECL_OVERRIDE;
    void removeInlineBox(InlineBox *_box) Q_DECL_OVERRIDE
    {
        if (m_placeHolderBox == _box) {
            m_placeHolderBox = 0;
        }
    }

    void removeChild(RenderObject *) Q_DECL_OVERRIDE;

    void detach() Q_DECL_OVERRIDE;

    short minWidth() const Q_DECL_OVERRIDE
    {
        return m_minWidth;
    }
    int maxWidth() const Q_DECL_OVERRIDE
    {
        return m_maxWidth;
    }

    short contentWidth() const Q_DECL_OVERRIDE;
    int contentHeight() const Q_DECL_OVERRIDE;

    bool absolutePosition(int &xPos, int &yPos, bool f = false) const Q_DECL_OVERRIDE;

    void setPos(int xPos, int yPos) Q_DECL_OVERRIDE;

    int xPos() const Q_DECL_OVERRIDE
    {
        return m_x;
    }
    int yPos() const Q_DECL_OVERRIDE
    {
        return m_y;
    }
    short width() const Q_DECL_OVERRIDE;
    int height() const Q_DECL_OVERRIDE;

    short marginTop() const Q_DECL_OVERRIDE
    {
        return m_marginTop;
    }
    short marginBottom() const Q_DECL_OVERRIDE
    {
        return m_marginBottom;
    }
    short marginLeft() const Q_DECL_OVERRIDE
    {
        return m_marginLeft;
    }
    short marginRight() const Q_DECL_OVERRIDE
    {
        return m_marginRight;
    }

    void setWidth(int width) Q_DECL_OVERRIDE;
    void setHeight(int height) Q_DECL_OVERRIDE;

    void position(InlineBox *box, int from, int len, bool reverse) Q_DECL_OVERRIDE;

    int highestPosition(bool includeOverflowInterior = true, bool includeSelf = true) const Q_DECL_OVERRIDE;
    int lowestPosition(bool includeOverflowInterior = true, bool includeSelf = true) const Q_DECL_OVERRIDE;
    int rightmostPosition(bool includeOverflowInterior = true, bool includeSelf = true) const Q_DECL_OVERRIDE;
    int leftmostPosition(bool includeOverflowInterior = true, bool includeSelf = true) const Q_DECL_OVERRIDE;

    void repaint(Priority p = NormalPriority) Q_DECL_OVERRIDE;

    void repaintRectangle(int x, int y, int w, int h, Priority p = NormalPriority, bool f = false) Q_DECL_OVERRIDE;

    short containingBlockWidth(RenderObject *providedCB = 0) const Q_DECL_OVERRIDE;
    void relativePositionOffset(int &tx, int &ty) const;

    void calcWidth() Q_DECL_OVERRIDE;
    void calcHeight() Q_DECL_OVERRIDE;

    virtual short calcReplacedWidth() const;
    virtual int   calcReplacedHeight() const;

    int availableHeight() const Q_DECL_OVERRIDE;
    virtual int availableWidth() const;

    void calcVerticalMargins() Q_DECL_OVERRIDE;

    RenderLayer *layer() const Q_DECL_OVERRIDE
    {
        return m_layer;
    }

    void setStaticX(short staticX);
    void setStaticY(int staticY);
    int staticX() const
    {
        return m_staticX;
    }
    int staticY() const
    {
        return m_staticY;
    }

    void caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const Q_DECL_OVERRIDE;

    void calcHorizontalMargins(const Length &ml, const Length &mr, int cw);
    RenderBlock *createAnonymousBlock();

    int pageTopAfter(int y) const Q_DECL_OVERRIDE;
    int crossesPageBreak(int t, int b) const Q_DECL_OVERRIDE;

    bool handleEvent(const DOM::EventImpl &ev) Q_DECL_OVERRIDE;

    int calcBoxWidth(int w) const;
    int calcBoxHeight(int h) const;
    virtual int calcContentWidth(int w) const;
    virtual int calcContentHeight(int h) const;

    InlineBox *placeHolderBox()
    {
        return m_placeHolderBox;
    }
    void setPlaceHolderBox(InlineBox *placeHolder)
    {
        m_placeHolderBox = placeHolder; /* assert !m_placeHolderBox */
    }
    QRect getFixedBackgroundImageRect(const BackgroundLayer *bglayer, int &sx, int &sy, int &scaledImageWidth, int &scaledImageHeight);

protected:
    int calcWidthUsing(WidthType widthType, int cw, LengthType &lengthType);
    int calcHeightUsing(const Length &height);
    int calcReplacedWidthUsing(WidthType widthType) const;
    int calcReplacedHeightUsing(HeightType heightType) const;
    int calcPercentageHeight(const Length &height) const;
    int availableHeightUsing(const Length &h) const;
    int availableWidthUsing(const Length &w) const;
    int calcImplicitContentHeight() const;
    bool hasImplicitHeight() const
    {
        return isPositioned() && !style()->top().isAuto() && !style()->bottom().isAuto();
    }

protected:
    void paintBoxDecorations(PaintInfo &paintInfo, int _tx, int _ty) Q_DECL_OVERRIDE;
    void paintRootBoxDecorations(PaintInfo &paintInfo, int _tx, int _ty);

    void paintAllBackgrounds(QPainter *p, const QColor &c, const BackgroundLayer *bgLayer, QRect clipr, int _tx, int _ty, int w, int h);
    virtual void paintOneBackground(QPainter *p, const QColor &c, const BackgroundLayer *bgLayer, QRect clipr, int _tx, int _ty, int w, int h);
    virtual void paintBackgroundExtended(QPainter * /*p*/, const QColor & /*c*/, const BackgroundLayer * /*bgLayer*/,
                                         QRect clipr, int /*_tx*/, int /*_ty*/,
                                         int /*w*/, int /*height*/, int /*bleft*/, int /*bright*/, int /*pleft*/, int /*pright*/,
                                         int /*btop*/, int /*bbottom*/, int /*ptop*/, int /*pbottom*/) Q_DECL_OVERRIDE;
    void outlineBox(QPainter *p, int _tx, int _ty, const char *color = "red");

    void calcAbsoluteHorizontal();
    void calcAbsoluteVertical();
    void calcAbsoluteHorizontalValues(Length width, const RenderObject *cb, EDirection containerDirection,
                                      const int containerWidth, const int bordersPlusPadding,
                                      const Length left, const Length right, const Length marginLeft, const Length marginRight,
                                      short &widthValue, short &marginLeftValue, short &marginRightValue, short &xPos);
    void calcAbsoluteVerticalValues(Length height, const RenderObject *cb,
                                    const int containerHeight, const int bordersPlusPadding,
                                    const Length top, const Length bottom, const Length marginTop, const Length marginBottom,
                                    int &heightValue, short &marginTopValue, short &marginBottomValue, int &yPos);

    void calcAbsoluteVerticalReplaced();
    void calcAbsoluteHorizontalReplaced();

    QPainterPath borderRadiusClipPath(const BackgroundLayer *bgLayer, int _tx, int _ty, int w, int h,
                                      int bleft, int bright, int btop, int bbottom) const;
    QRect overflowClipRect(int tx, int ty) Q_DECL_OVERRIDE;
    QRect clipRect(int tx, int ty) Q_DECL_OVERRIDE;

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
