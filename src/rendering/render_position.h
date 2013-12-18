/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2009 Vyacheslav Tokarev (tsjoker@gmail.com)
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
 */

#ifndef Render_Position_H
#define Render_Position_H

#include "xml/dom_position.h"
#include "rendering/render_object.h"

using DOM::Position;

namespace DOM
{
    class NodeImpl;
}

using DOM::NodeImpl;

namespace khtml
{
class InlineBox;

class RenderPosition
{
public:
    // constructs empty position
    RenderPosition() {}
    RenderPosition(const Position& position) : m_position(position) {}
    RenderPosition(NodeImpl* node, int offset);

    static RenderPosition fromDOMPosition(const Position& position);

    Position position() const { return m_position; }
    const RenderObject* renderer() const { return m_position.isEmpty() ? 0 : m_position.node()->renderer(); }
    NodeImpl* node() const { return m_position.node(); }
    int domOffset() const { return m_position.offset(); }

    inline bool isEmpty() const { return m_position.isEmpty(); }
    inline bool notEmpty() const { return m_position.notEmpty(); }

    // QRect caretRegion() const;

    // int renderedOffset() const; // convert to rendered offset, though we need to eliminate other code using it
    int renderedOffset() {
        int result;
        getInlineBoxAndOffset(result);
        return result;
    }

    bool inRenderedContent() { return renderer(); }
    static bool inRenderedContent(const Position& position) { return fromDOMPosition(position).inRenderedContent(); }

    // implementation: same RootInlineBox
    static bool rendersOnSameLine(const RenderPosition& self, const RenderPosition& other);
    // implementation: either renderers are different, or inline boxes or offsets inside
    static bool rendersInDifferentPosition(const RenderPosition& self, const RenderPosition& other);

    // static bool rendersOnDifferentLine(); // compare inline boxes and its roots

    // bool isFirstOnRenderedLine() const; // either iterate backwards over DOM tree or check if we're at the beginning of the RootInlineBox?
    // bool isLastOnRenderedLine() const; // -||-

    RenderPosition previousLinePosition(int);
    RenderPosition nextLinePosition(int);

protected:
    // returns rendered offset
    InlineBox* getInlineBoxAndOffset(int& offset) const;

private:
    Position m_position;
    // affinity thing, so we'll have where to put caret at the end of inline box or at the start of next one
};

inline bool operator==(const RenderPosition& a, const RenderPosition& b)
{
    return a.position() == b.position();
}

inline bool operator!=(const RenderPosition& a, const RenderPosition& b)
{
    return !(a == b);
}

QDebug operator<<(QDebug stream, const RenderPosition& renderPosition);

} // namespace

#endif
