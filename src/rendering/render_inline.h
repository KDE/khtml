/*
 * This file is part of the render object implementation for KHTML.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999-2003 Antti Koivisto (koivisto@kde.org)
 *           (C) 2002-2003 Dirk Mueller (mueller@kde.org)
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

#ifndef RENDER_INLINE_H
#define RENDER_INLINE_H

#include "render_flow.h"

namespace khtml
{

class RenderInline : public RenderFlow
{
public:
    RenderInline(DOM::NodeImpl *node) : RenderFlow(node), m_isContinuation(false) {}

    const char *renderName() const override;

    bool isRenderInline() const override
    {
        return true;
    }
    bool isInlineFlow() const override
    {
        return true;
    }
    bool childrenInline() const override
    {
        return true;
    }

    bool isInlineContinuation() const override;

    void addChildToFlow(RenderObject *newChild, RenderObject *beforeChild) override;

    void splitInlines(RenderBlock *fromBlock, RenderBlock *toBlock, RenderBlock *middleBlock,
                      RenderObject *beforeChild, RenderFlow *oldCont);

    void splitFlow(RenderObject *beforeChild, RenderBlock *newBlockBox,
                   RenderObject *newChild, RenderFlow *oldCont);

    void setStyle(RenderStyle *_style) override;
    void attach() override;

    void layout() override
    {
        setNeedsLayout(false);    // Do nothing for layout()
    }

    void paint(PaintInfo &, int tx, int ty) override;

    bool nodeAtPoint(NodeInfo &info, int _x, int _y, int _tx, int _ty, HitTestAction hitTestAction, bool inside) override;

    void calcMinMaxWidth() override;

    // overrides RenderObject
    bool requiresLayer() const override
    {
        return isRelPositioned();
    }

    short width() const override;
    int height() const override;

    int inlineXPos() const override;
    int inlineYPos() const override;

    // used to calculate offsetWidth/Height.  Overridden by inlines (render_flow) to return
    // the remaining width on a given line (and the height of a single line).
    int offsetLeft() const override;
    int offsetTop() const override;

    RenderPosition positionForCoordinates(int x, int y) override;

    void caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const override;
    void paintOutlines(QPainter *p, int tx, int ty);

protected:
    static RenderInline *cloneInline(RenderFlow *src);
    void paintOutlinePath(QPainter *p, int tx, int ty, const QPoint *begin, const QPoint *end, BorderSide startingBS, int initialDirection, BorderSide endingBS);

private:
    bool m_isContinuation : 1; // Whether or not we're a continuation of an inline.

};

} // namespace

#endif // RENDER_BLOCK_H

