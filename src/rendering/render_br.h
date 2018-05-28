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

    const char *renderName() const override
    {
        return "RenderBR";
    }

    void paint(PaintInfo &, int, int) override {}

    unsigned int width(unsigned int, unsigned int, const Font *) const override
    {
        return 0;
    }
    unsigned int width(unsigned int, unsigned int, bool) const override
    {
        return 0;
    }
    short width() const override
    {
        return RenderText::width();
    }

    int height() const override
    {
        return 0;
    }

    // overrides
    void calcMinMaxWidth() override {}
    short minWidth() const override
    {
        return 0;
    }
    int maxWidth() const override
    {
        return 0;
    }

    FindSelectionResult checkSelectionPoint(int _x, int _y, int _tx, int _ty,
            DOM::NodeImpl *&node, int &offset,
            SelPointState &) override;

    bool isBR() const override
    {
        return true;
    }

    long caretMinOffset() const override;
    long caretMaxOffset() const override;
    unsigned long caretMaxRenderedOffset() const override;

    RenderPosition positionForCoordinates(int _x, int _y) override;
#if 0
    virtual void caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const;
#endif

    InlineBox *inlineBox(long offset) override;

};

}
#endif
