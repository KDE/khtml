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
namespace khtml {

class RenderBR : public RenderText
{
public:
    RenderBR(DOM::NodeImpl* node);
    virtual ~RenderBR();

    virtual const char *renderName() const { return "RenderBR"; }

    virtual void paint( PaintInfo&, int, int) {}

    virtual unsigned int width(unsigned int, unsigned int, const Font *) const { return 0; }
    virtual unsigned int width( unsigned int, unsigned int, bool) const { return 0; }
    virtual short width() const { return RenderText::width(); }

    virtual int height() const { return 0; }

    // overrides
    virtual void calcMinMaxWidth() {}
    virtual short minWidth() const { return 0; }
    virtual int maxWidth() const { return 0; }

    virtual FindSelectionResult checkSelectionPoint( int _x, int _y, int _tx, int _ty,
                                                     DOM::NodeImpl*& node, int & offset,
						     SelPointState & );

    virtual bool isBR() const { return true; }
    
    virtual long caretMinOffset() const;
    virtual long caretMaxOffset() const;
    virtual unsigned long caretMaxRenderedOffset() const;
    
    virtual RenderPosition positionForCoordinates(int _x, int _y);
#if 0
    virtual void caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const;
#endif

    virtual InlineBox *inlineBox(long offset);
    
};

}
#endif
