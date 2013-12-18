/**
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
#include "render_br.h"

using namespace khtml;
using DOM::Position;

RenderBR::RenderBR(DOM::NodeImpl* node)
    : RenderText(node, new DOM::DOMStringImpl(QChar('\n')))
{
    m_hasReturn = true;
}

RenderBR::~RenderBR()
{
}

long RenderBR::caretMinOffset() const 
{
    return 0;
}

long RenderBR::caretMaxOffset() const 
{
    return 1;
}

unsigned long RenderBR::caretMaxRenderedOffset() const
{
    return 1;
}

RenderPosition RenderBR::positionForCoordinates(int /*_x*/, int /*_y*/)
{
    return RenderPosition(element(), 0);
}

#if 0
void RenderBR::caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const
{
  RenderText::caretPos(offset,flags,_x,_y,width,height);
  return;
#if 0
    if (previousSibling() && !previousSibling()->isBR() && !previousSibling()->isFloating()) {
        int offset = 0;
        if (previousSibling()->isText())
            offset = static_cast<RenderText*>(previousSibling())->maxOffset();

	// FIXME: this won't return a big width in override mode (LS)
        previousSibling()->caretPos(offset,override,_x,_y,width,height);
        return;
    }

    int absx, absy;
    absolutePosition(absx,absy);
    if (absx == -1) {
        // we don't know out absolute position, and there is no point returning
        // just a relative one
        _x = _y = -1;
    }
    else {
        _x += absx;
        _y += absy;
    }
    height = RenderText::verticalPositionHint( false );
    width = override ? height / 2 : 1;
#endif
}
#endif

FindSelectionResult RenderBR::checkSelectionPoint(int _x, int _y, int _tx, int _ty, DOM::NodeImpl*& node, int &offset, SelPointState &state)
{
  // Simply take result of previous one
  RenderText *prev = static_cast<RenderText *>(previousSibling());
  if (!prev || !prev->isText() || !prev->firstTextBox() || prev->isBR())
    prev = this;

  //qDebug() << "delegated to " << prev->renderName() << "@" << prev;
  return prev->RenderText::checkSelectionPoint(_x, _y, _tx, _ty, node, offset, state);
}

InlineBox *RenderBR::inlineBox(long /*offset*/)
{
    return firstTextBox();
}
