/**
 * This file is part of the html renderer for KDE.
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

#include "rendering/render_position.h"
#include "rendering/render_text.h"
#include "rendering/render_line.h"
#include "rendering/render_block.h"

using namespace DOM;
using namespace khtml;

RenderPosition::RenderPosition(NodeImpl *node, int offset)
{
    if (node && node->renderer() && node->renderer()->isText()) {
        offset = static_cast<RenderText *>(node->renderer())->convertToDOMPosition(offset);
    }
    m_position = Position(node, offset);
}

RenderPosition RenderPosition::fromDOMPosition(const Position &position)
{
    if (position.isEmpty()) {
        return RenderPosition();
    }

    NodeImpl *node = position.node();
    RenderObject *renderObject = node->renderer();
    // if no renderer -> no position in the rendering is possible
    if (!renderObject) {
        return RenderPosition();
    }

    if (!renderObject->isText()) {
        return RenderPosition(position);
    }

    if (renderObject->isBR()) {
        return (!position.offset() && renderObject->inlineBox(0)) ? RenderPosition(Position(node, 0)) : RenderPosition();
    }
    // return renderObject->inlineBox(0) ? RenderPosition(position) : RenderPosition();

    // qCDebug(KHTML_LOG) << "[text position]" << position;
    const RenderText *renderText = static_cast<const RenderText *>(renderObject);
    int domOffset = position.offset();
    int renderOffset = renderText->convertToRenderedPosition(domOffset);
    domOffset = renderText->convertToDOMPosition(renderOffset);
    // now we need to modify original position
    // qCDebug(KHTML_LOG) << "[equivalent offset]" << domOffset;
    RenderPosition result(Position(node, domOffset));
    if (!result.getInlineBoxAndOffset(renderOffset)) {
        return RenderPosition();
    }
    return result;
}

InlineBox *RenderPosition::getInlineBoxAndOffset(int &offset) const
{
    // default value
    offset = 0;
    if (!renderer()) {
        // qCDebug(KHTML_LOG) << "[EMPTY POSITION]";
        return nullptr;
    }
    // qCDebug(KHTML_LOG) << "[find inline box]" << m_position;

    const NodeImpl *node = m_position.node();
    /*const*/ RenderObject *renderObject = node->renderer();
    if (!renderObject->isText()) {
        offset = m_position.offset();
        return renderObject->inlineBox(offset);
    }
    if (renderObject->isBR()) {
        offset = m_position.offset();
        return renderObject->inlineBox(0);
    }
    int domOffset = m_position.offset();

    /*const*/ RenderText *renderText = static_cast<RenderText *>(renderObject);
    int renderOffset = renderText->convertToRenderedPosition(domOffset);
    InlineTextBox *textBox;
    for (textBox = renderText->firstTextBox(); textBox; textBox = textBox->nextTextBox()) {
        if (renderOffset >= textBox->start() && renderOffset <= textBox->end()) {
            offset = renderOffset; // - textBox->start();
            // qCDebug(KHTML_LOG) << "[result]" << offset << textBox;
            return textBox;
        } else if (renderOffset < textBox->start()) {
            offset = textBox->start();
            // qCDebug(KHTML_LOG) << "[result]" << offset << textBox;
            return textBox;
        } else if (!textBox->nextTextBox()) {
            offset = textBox->start() + textBox->len();
            // qCDebug(KHTML_LOG) << "[result]" << offset << textBox;
            return textBox;
        }
        // choose right box we're at
        // if we're not we should probably return 0, but set offset properly
    }
    return nullptr;
}

bool RenderPosition::rendersInDifferentPosition(const RenderPosition &self, const RenderPosition &other)
{
    // qCDebug(KHTML_LOG) << "[compare]" << self.position() << other.position();
    if (self == other) {
        return false;
    }
    if (self.isEmpty() || other.isEmpty()) {
        return false;
    }
    // if (self.renderer() != other.renderer()) return true;
    if (!self.renderer() || !other.renderer()) {
        return false;
    }
    int selfOffset;
    const InlineBox *selfBox = self.getInlineBoxAndOffset(selfOffset);
    int otherOffset;
    const InlineBox *otherBox = other.getInlineBoxAndOffset(otherOffset);
    if (selfBox == otherBox && selfOffset == otherOffset) {
        return false;
    }

    // FIXME remove caret rects comparing - it's slow, or leave as rare fall back
    int x1, y1, x2, y2, w1, h1, w2, h2;
    self.renderer()->caretPos(const_cast<RenderPosition &>(self).renderedOffset(), 0, x1, y1, w1, h1);
    other.renderer()->caretPos(const_cast<RenderPosition &>(other).renderedOffset(), 0, x2, y2, w2, h2);
    if (x1 == x2 && y1 == y2 && w1 == w2 && h1 == h2) {
        return false;
    }

    // compare containing blocks
    // flow block elements (DOM)
    // bound positions etc
    return true;
}

bool RenderPosition::rendersOnSameLine(const RenderPosition &self, const RenderPosition &other)
{
    if (self == other) {
        return true;
    }
    if (self.isEmpty() || other.isEmpty()) {
        return false;
    }
    if (self.renderer() != other.renderer()) {
        return false;
    }
    int tempOffset;
    /*const */InlineBox *selfBox  = self.getInlineBoxAndOffset(tempOffset);
    /*const */InlineBox *otherBox = other.getInlineBoxAndOffset(tempOffset);
    return selfBox == otherBox || (selfBox && otherBox && selfBox->root() == otherBox->root());
}

RenderPosition RenderPosition::previousLinePosition(int x)
{
    // qCDebug(KHTML_LOG) << "[Previous line at x]" << x;
    if (!renderer()) {
        return *this;
    }

    int rOffset;
    NodeImpl *node = m_position.node();
    InlineBox *box = getInlineBoxAndOffset(rOffset);
    // qCDebug(KHTML_LOG) << "[box;offset]" << box << rOffset;

    RenderBlock *containingBlock = nullptr;
    RootInlineBox *root = nullptr;
    if (box) {
        root = box->root()->prevRootBox();
    }
    // qCDebug(KHTML_LOG) << "[root]" << root;
    if (root) {
        containingBlock = node->renderer()->containingBlock();
    } else {
        // This containing editable block does not have a previous line.
        // Need to move back to previous containing editable block in this root editable
        // block and find the last root line box in that block.
        NodeImpl *startBlock = node->enclosingBlockFlowElement();
        NodeImpl *n = node->previousEditable();
        // qCDebug(KHTML_LOG) << "[StartBlock]" << startBlock << (startBlock ? startBlock->renderer() : 0);
        while (n && startBlock == n->enclosingBlockFlowElement()) {
            n = n->previousEditable();
        }
        // qCDebug(KHTML_LOG) << "[n]" << n << (n ? n->renderer() : 0) << (n ? n->nodeName() : "");
        printEnclosingBlockTree(n);
        if (n) {
            while (n && !Position(n, n->caretMaxOffset()).inRenderedContent()) {
                // qCDebug(KHTML_LOG) << "[previous]" << n;
                n = n->previousEditable();
            }
            // qCDebug(KHTML_LOG) << "[n]" << n << (n ? n->renderer() : 0);
            if (n && inSameRootNavigableElement(n, node)) {
                assert(n->renderer());
                // box = n->renderer()->inlineBox(n->caretMaxOffset());
                int offset;
                box = RenderPosition::fromDOMPosition(Position(n, n->caretMaxOffset())).getInlineBoxAndOffset(offset);
                // qCDebug(KHTML_LOG) << "[box]" << box << offset;
                // previous root line box found
                if (box) {
                    root = box->root();
                    containingBlock = n->renderer()->containingBlock();
                    // qCDebug(KHTML_LOG) << "[root,block]" << root << containingBlock;
                }
                return RenderPosition::fromDOMPosition(Position(n, n->caretMaxOffset())).position();
            }
        }
    }

    if (root) {
        int absx, absy;
        containingBlock->absolutePosition(absx, absy);
        // qCDebug(KHTML_LOG) << "[cb]" << containingBlock << absx << absy;
        RenderObject *renderer = root->closestLeafChildForXPos(x, absx)->object();
        // qCDebug(KHTML_LOG) << "[renderer]" << renderer;
        return renderer->positionForCoordinates(x, absy + root->topOverflow());
    }

    return *this;
}

RenderPosition RenderPosition::nextLinePosition(int x)
{
    // qCDebug(KHTML_LOG) << "[Next line at x]" << x;
    if (!renderer()) {
        return *this;
    }

    int rOffset;
    NodeImpl *node = m_position.node();
    InlineBox *box = getInlineBoxAndOffset(rOffset);
    // qCDebug(KHTML_LOG) << "[box;offset]" << box << rOffset;

    RenderBlock *containingBlock = nullptr;
    RootInlineBox *root = nullptr;
    if (box) {
        root = box->root()->nextRootBox();
    }
    if (root) {
        containingBlock = node->renderer()->containingBlock();
    } else {
        // This containing editable block does not have a next line.
        // Need to move forward to next containing editable block in this root editable
        // block and find the first root line box in that block.
        NodeImpl *startBlock = node->enclosingBlockFlowElement();
        NodeImpl *n = node->nextEditable();
        while (n && startBlock == n->enclosingBlockFlowElement()) {
            n = n->nextEditable();
        }
        if (n) {
            while (n && !Position(n, n->caretMinOffset()).inRenderedContent()) {
                n = n->nextEditable();
            }
            if (n && inSameRootNavigableElement(n, node)) {
                assert(n->renderer());
                box = n->renderer()->inlineBox(n->caretMinOffset());
                // previous root line box found
                if (box) {
                    root = box->root();
                    containingBlock = n->renderer()->containingBlock();
                }
                return Position(n, n->caretMinOffset());
            }
        }
    }

    if (root) {
        int absx, absy;
        containingBlock->absolutePosition(absx, absy);
        RenderObject *renderer = root->closestLeafChildForXPos(x, absx)->object();
        return renderer->positionForCoordinates(x, absy + root->topOverflow());
    }

    return *this;
}

/*bool RenderPosition::haveRenderPosition()
{
    // qCDebug(KHTML_LOG) << *this;
    if (isEmpty())
        return false;

    RenderObject *renderer = node()->renderer();
    if (!renderer || !(node()->document()->part()->isCaretMode() || renderer->isEditable()))
        return false;

    if (renderer->style()->visibility() != khtml::VISIBLE)
        return false;

    if (renderer->isBR() && static_cast<RenderText *>(renderer)->firstTextBox()) {
        return offset() == 0;
    }
    else if (renderer->isText()) {
        RenderText *textRenderer = static_cast<RenderText *>(renderer);
        // qCDebug(KHTML_LOG) << "text" << textRenderer;
        unsigned rOffset = textRenderer->convertToRenderedPosition(offset());
        for (InlineTextBox *box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
            // qCDebug(KHTML_LOG) << "box" << box << box->m_start << box->m_start + box->m_len;
            if (rOffset >= box->m_start && rOffset <= box->m_start + box->m_len) {
                return true;
            }
            else if (rOffset < box->m_start) {
                // The offset we're looking for is before this node
                // this means the offset must be in content that is
                // not rendered. Return false.
                return false;
            }
        }
    }
    else if (offset() >= renderer->caretMinOffset() && offset() <= renderer->caretMaxOffset()) {
        // don't return containing editable blocks unless they are empty
        if (node()->enclosingBlockFlowElement() == node() && node()->firstChild())
            return false;
        return true;
    }

    return false;
}*/

/*bool RenderPosition::isFirstOnRenderedLine() const
{
}

bool RenderPosition::isLastOnRenderedLine() const
{
}*/

