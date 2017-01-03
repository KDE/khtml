/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
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
#ifndef RENDER_BR_H
#define RENDER_BR_H

#include "render_text.h"

/*
 * The whole class here is a hack to get <br> working, as long as we don't have support for
 * CSS2 :before and :after pseudo elements
 */
namespace khtml
{

class RenderBR : public RenderText
{
public:
    RenderBR(DOM::NodeImpl *node);
    virtual ~RenderBR();

    const char *renderName() const Q_DECL_OVERRIDE
    {
        return "RenderBR";
    }

    void paint(PaintInfo &, int, int) Q_DECL_OVERRIDE {}

    unsigned int width(unsigned int, unsigned int, const Font *) const Q_DECL_OVERRIDE
    {
        return 0;
    }
    unsigned int width(unsigned int, unsigned int, bool) const Q_DECL_OVERRIDE
    {
        return 0;
    }
    short width() const Q_DECL_OVERRIDE
    {
        return RenderText::width();
    }

    int height() const Q_DECL_OVERRIDE
    {
        return 0;
    }

    // overrides
    void calcMinMaxWidth() Q_DECL_OVERRIDE {}
    short minWidth() const Q_DECL_OVERRIDE
    {
        return 0;
    }
    int maxWidth() const Q_DECL_OVERRIDE
    {
        return 0;
    }

    FindSelectionResult checkSelectionPoint(int _x, int _y, int _tx, int _ty,
            DOM::NodeImpl *&node, int &offset,
            SelPointState &) Q_DECL_OVERRIDE;

    bool isBR() const Q_DECL_OVERRIDE
    {
        return true;
    }

    long caretMinOffset() const Q_DECL_OVERRIDE;
    long caretMaxOffset() const Q_DECL_OVERRIDE;
    unsigned long caretMaxRenderedOffset() const Q_DECL_OVERRIDE;

    RenderPosition positionForCoordinates(int _x, int _y) Q_DECL_OVERRIDE;
#if 0
    virtual void caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const;
#endif

    InlineBox *inlineBox(long offset) Q_DECL_OVERRIDE;

};

}
#endif
