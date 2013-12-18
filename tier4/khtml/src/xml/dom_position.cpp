/*
 * Copyright (C) 2004 Apple Computer, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dom_position.h"

#include "misc/helper.h"
#include "rendering/render_block.h"
#include "rendering/render_line.h"
#include "rendering/render_object.h"
#include "rendering/render_style.h"
#include "rendering/render_text.h"
#include "xml/dom_positioniterator.h"
#include "xml/dom_elementimpl.h"
#include "xml/dom_docimpl.h"
#include "xml/dom_nodeimpl.h"
#include "html/html_documentimpl.h"
#include "rendering/render_position.h"

#include "khtml_part.h"

#include <qstring.h>

#define DEBUG_CARET

// FIXME shouldn't use any rendereing related here except for RenderPosition
using khtml::InlineBox;
using khtml::InlineFlowBox;
using khtml::InlineTextBox;
using khtml::RenderBlock;
using khtml::RenderObject;
using khtml::RenderText;
using khtml::RootInlineBox;
using khtml::RenderPosition;

namespace DOM {

static NodeImpl *nextRenderedEditable(NodeImpl *node)
{
    while (1) {
        node = node->nextEditable();
        if (!node)
            return 0;
        if (!node->renderer())
            continue;
        if (node->renderer()->inlineBox(0))
            return node;
    }
    return 0;
}

static NodeImpl *previousRenderedEditable(NodeImpl *node)
{
    while (1) {
        node = node->previousEditable();
        if (!node)
            return 0;
        if (!node->renderer())
            continue;
        if (node->renderer()->inlineBox(0))
            return node;
    }
    return 0;
}

/*static*/ NodeImpl *rootNavigableElement(NodeImpl *node)
{
    DocumentImpl *doc = node->document();
    if (doc && doc->part()->isCaretMode()) {
        if (doc->isHTMLDocument())
            return static_cast<HTMLDocumentImpl *>(doc)->body();
        else
            return doc->documentElement();
    }
  
    return node->rootEditableElement();
}

/*inline*/ /*static*/ bool inSameRootNavigableElement(NodeImpl *n1, NodeImpl *n2)
{
    return n1 && n2 && rootNavigableElement(n1) == rootNavigableElement(n2);
}

static void printSubTree(NodeImpl *node, int indent = 0)
{
    QString temp;
    temp.fill(' ', indent);
    // qDebug() << temp << node << node->nodeName() << node->renderer()
    //    << (node->renderer() ? node->renderer()->renderName() : "")
    //    << (node->isTextNode() ? static_cast<TextImpl*>(node)->toString() : "") << endl;
    for (NodeImpl *subNode = node->firstChild(); subNode; subNode = subNode->nextSibling())
        printSubTree(subNode, indent + 1);
}

void printEnclosingBlockTree(NodeImpl *node)
{
    if (!node || !node->enclosingBlockFlowElement()) {
        // qDebug() << "[null node]" << node << endl;
        return;
    }
    printSubTree(node->enclosingBlockFlowElement());
}

void printRootEditableTree(NodeImpl *node)
{
    if (!node || !node->rootEditableElement()) {
        // qDebug() << "[null node]" << node << endl;
        return;
    }
    printSubTree(node->rootEditableElement());
}

Position::Position(NodeImpl *node, long offset)
    : m_node(0), m_offset(offset)
{
    if (node) {
        m_node = node;
        m_node->ref();
    }
}

Position::Position(const Position &o)
    : m_node(0), m_offset(o.offset())
{
    if (o.node()) {
        m_node = o.node();
        m_node->ref();
    }
}

Position::~Position() {
    if (m_node) {
        m_node->deref();
    }
}

Position &Position::operator=(const Position &o)
{
    if (m_node) {
        m_node->deref();
    }
    m_node = o.node();
    if (m_node) {
        m_node->ref();
    }

    m_offset = o.offset();

    return *this;
}

ElementImpl *Position::element() const
{
    if (isEmpty())
        return 0;

    NodeImpl *n = node();
    for (; n && !n->isElementNode(); n = n->parentNode()) {}; //loop

    return static_cast<ElementImpl *>(n);
}

long Position::renderedOffset() const
{
    return RenderPosition::fromDOMPosition(*this).renderedOffset();
}

Position Position::equivalentLeafPosition() const
{
#ifdef DEBUG_CARET
    // qDebug() << *this << endl;
#endif
    if (isEmpty())
        return Position();

    if (!node()->renderer() || !node()->renderer()->firstChild())
        return *this;

#ifdef DEBUG_CARET
    // qDebug() << "[Position]" << this << endl;
#endif
    NodeImpl *n = node();
    int count = 0;
    while (1) {
        n = n->nextLeafNode();
        if (!n || !n->inSameContainingBlockFlowElement(node()))
            return *this;
#ifdef DEBUG_CARET
        // qDebug() << "[iterate]" << n << count << n->maxOffset() << endl;
#endif
        if (count + n->maxOffset() >= offset()) {
            count = offset() - count;
            break;
        }
        count += n->maxOffset();
    }
    return Position(n, count);
}

Position Position::previousRenderedEditablePosition() const
{
#ifdef DEBUG_CARET
    // qDebug() << *this << endl;
#endif
    if (isEmpty())
        return Position();

    if ((node()->document()->part()->isCaretMode() || node()->isContentEditable()) && node()->hasChildNodes() == false && inRenderedContent())
        return *this;

    NodeImpl *n = node();
    while (1) {
        n = n->previousEditable();
        if (!n)
            return Position();
        if (n->renderer() && n->renderer()->style()->visibility() == khtml::VISIBLE)
            break;
    }

    return Position(n, 0);
}

Position Position::nextRenderedEditablePosition() const
{
#ifdef DEBUG_CARET
    // qDebug() << *this << endl;
#endif
    if (isEmpty())
        return Position();

    if ((node()->document()->part()->isCaretMode() || node()->isContentEditable()) && node()->hasChildNodes() == false && inRenderedContent())
        return *this;

    NodeImpl *n = node();
    while (1) {
        n = n->nextEditable();
        if (!n)
            return Position();
        if (n->renderer() && n->renderer()->style()->visibility() == khtml::VISIBLE)
            break;
    }

    return Position(n, 0);
}

Position Position::previousCharacterPosition() const
{
#ifdef DEBUG_CARET
    // qDebug() << *this << endl;
#endif
    if (isEmpty())
        return Position();

    NodeImpl *fromRootNavigableElement = rootNavigableElement(node());
#ifdef DEBUG_CARET
    // qDebug() << "RootElement" << fromRootNavigableElement << endl;
#endif
    PositionIterator it(*this);

    RenderPosition originalRPosition = RenderPosition::fromDOMPosition(*this);

    while (!it.atStart()) {
        Position pos = it.previous();
#ifdef DEBUG_CARET
        // qDebug() << "iterate" << pos << endl;
#endif

        if (rootNavigableElement(pos.node()) != fromRootNavigableElement) {
#ifdef DEBUG_CARET
            // qDebug() << "different root" << rootNavigableElement(pos.node()) << endl;
#endif
            return *this;
        }
        RenderPosition currentRPosition = RenderPosition::fromDOMPosition(pos);
        if (RenderPosition::rendersInDifferentPosition(originalRPosition, currentRPosition))
            return currentRPosition.position();
    }
#ifdef DEBUG_CARET
    // qDebug() << "no previous position" << endl;
#endif
    return *this;
}

Position Position::nextCharacterPosition() const
{
#ifdef DEBUG_CARET
    // qDebug() << *this << endl;
#endif
    if (isEmpty())
        return Position();

    NodeImpl *fromRootNavigableElement = rootNavigableElement(node());
    PositionIterator it(*this);

    RenderPosition originalRPosition = RenderPosition::fromDOMPosition(*this);

    while (!it.atEnd()) {
        Position pos = it.next();

        if (rootNavigableElement(pos.node()) != fromRootNavigableElement)
            return *this;
        RenderPosition currentRPosition = RenderPosition::fromDOMPosition(pos);
        if (RenderPosition::rendersInDifferentPosition(originalRPosition, currentRPosition))
            return currentRPosition.position();
    }

    return *this;
}

Position Position::previousWordPosition() const
{
    if (isEmpty())
        return Position();

    Position pos = *this;
    for (PositionIterator it(*this); !it.atStart(); it.previous()) {
        if (it.current().node()->nodeType() == Node::TEXT_NODE || it.current().node()->nodeType() == Node::CDATA_SECTION_NODE) {
            // use RenderPosition here
            DOMString t = it.current().node()->nodeValue();
            QChar *chars = t.unicode();
            uint len = t.length();
            int start, end;
            khtml::findWordBoundary(chars, len, it.current().offset(), &start, &end);
            pos = Position(it.current().node(), start);
        }
        else {
            pos = Position(it.current().node(), it.current().node()->caretMinOffset());
        }
        if (pos != *this)
            return pos;
        it.setPosition(pos);
    }

    return *this;
}

Position Position::nextWordPosition() const
{
    if (isEmpty())
        return Position();

    Position pos = *this;
    for (PositionIterator it(*this); !it.atEnd(); it.next()) {
        if (it.current().node()->nodeType() == Node::TEXT_NODE || it.current().node()->nodeType() == Node::CDATA_SECTION_NODE) {
            // use RenderPosition here
            DOMString t = it.current().node()->nodeValue();
            QChar *chars = t.unicode();
            uint len = t.length();
            int start, end;
            khtml::findWordBoundary(chars, len, it.current().offset(), &start, &end);
            pos = Position(it.current().node(), end);
        }
        else {
            pos = Position(it.current().node(), it.current().node()->caretMaxOffset());
        }
        if (pos != *this)
            return pos;
        it.setPosition(pos);
    }

    return *this;
}

Position Position::previousLinePosition(int x) const
{
    return RenderPosition::fromDOMPosition(*this).previousLinePosition(x).position();
}

Position Position::nextLinePosition(int x) const
{
    return RenderPosition::fromDOMPosition(*this).nextLinePosition(x).position();
}

Position Position::equivalentUpstreamPosition() const
{
#ifdef DEBUG_CARET
    // qDebug() << *this << endl;
#endif
    if (!node())
        return Position();

    NodeImpl *block = node()->enclosingBlockFlowElement();

    PositionIterator it(*this);
    for (; !it.atStart(); it.previous()) {
#ifdef DEBUG_CARET
        // qDebug() << "[iterate]" << it.current() << endl;
#endif
        NodeImpl *currentBlock = it.current().node()->enclosingBlockFlowElement();
        if (block != currentBlock)
            return it.next();

        RenderObject *renderer = it.current().node()->renderer();
        if (!renderer)
            continue;

        if (renderer->style()->visibility() != khtml::VISIBLE)
            continue;

        if (renderer->isBlockFlow() || renderer->isReplaced() || renderer->isBR()) {
            if (it.current().offset() >= renderer->caretMaxOffset())
                return Position(it.current().node(), renderer->caretMaxOffset());
            else
                continue;
        }

        if (renderer->isText() && static_cast<RenderText *>(renderer)->firstTextBox()) {
            if (it.current().node() != node()) {
                Position result(it.current().node(), renderer->caretMaxOffset());
                if (rendersInDifferentPosition(result))
                    return it.next();
                return result;
            }

            if (it.current().offset() < 0)
                continue;
            uint textOffset = it.current().offset();

            RenderText *textRenderer = static_cast<RenderText *>(renderer);
            textOffset = textRenderer->convertToRenderedPosition(textOffset);
            // textRenderer->firstTextBox()->parent()->printTree();
            for (InlineTextBox *box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
                if (textOffset > box->start() && textOffset <= box->start() + box->len())
                    return it.current();
            }
        }
    }

    if (it.current().node()->enclosingBlockFlowElement() != block)
        return it.next();

    return it.current();
}

Position Position::equivalentDownstreamPosition() const
{
    // qDebug() << *this << endl;
    if (!node())
        return Position();

    NodeImpl *block = node()->enclosingBlockFlowElement();

    PositionIterator it(*this);
    for (; !it.atEnd(); it.next()) {
        // qDebug() << "[iterate]" << it.current() << endl;
        NodeImpl *currentBlock = it.current().node()->enclosingBlockFlowElement();
        if (block != currentBlock)
            return it.previous();

        RenderObject *renderer = it.current().node()->renderer();
        if (!renderer)
            continue;

        if (renderer->style()->visibility() != khtml::VISIBLE)
            continue;

        if (renderer->isBlockFlow() || renderer->isReplaced() || renderer->isBR()) {
            if (it.current().offset() <= renderer->caretMinOffset())
                return Position(it.current().node(), renderer->caretMinOffset());
            else
                continue;
        }

        if (renderer->isText() && static_cast<RenderText *>(renderer)->firstTextBox()) {
            if (it.current().node() != node()) {
                Position result(it.current().node(), renderer->caretMinOffset());
                if (rendersInDifferentPosition(result))
                    return it.previous();
                return result;
            }

            if (it.current().offset() < 0)
                continue;
            uint textOffset = it.current().offset();

            RenderText *textRenderer = static_cast<RenderText *>(renderer);
            textOffset = textRenderer->convertToRenderedPosition(textOffset);
            // textRenderer->firstTextBox()->parent()->printTree();
            for (InlineTextBox *box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
                if (textOffset >= box->start() && textOffset <= box->end())
                    return it.current();
            }
        }
    }

    if (it.current().node()->enclosingBlockFlowElement() != block)
        return it.previous();

    return it.current();
}

Position Position::equivalentRangeCompliantPosition() const
{
    if (isEmpty())
        return *this;

    if (!node()->parentNode())
        return *this;

    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return *this;

    if (!renderer->isReplaced() && !renderer->isBR())
        return *this;

    int o = 0;
    const NodeImpl *n = node();
    while ((n = n->previousSibling()))
        o++;

    return Position(node()->parentNode(), o + offset());
}

Position Position::equivalentShallowPosition() const
{
    if (isEmpty())
        return *this;

    Position pos(*this);
    while (pos.offset() == pos.node()->caretMinOffset() && pos.node()->parentNode() && pos.node() == pos.node()->parentNode()->firstChild())
        pos = Position(pos.node()->parentNode(), 0);
    return pos;
}

bool Position::atStartOfContainingEditableBlock() const
{
    return renderedOffset() == 0 && inFirstEditableInContainingEditableBlock();
}

bool Position::atStartOfRootEditableElement() const
{
    return renderedOffset() == 0 && inFirstEditableInRootEditableElement();
}

bool Position::inRenderedContent() const
{
    return RenderPosition::fromDOMPosition(*this).inRenderedContent();
}

bool Position::inRenderedText() const
{
    return RenderPosition::fromDOMPosition(*this).inRenderedContent() && node() && node()->renderer() && node()->renderer()->isText();
}

bool Position::rendersOnSameLine(const Position &pos) const
{
    if (isEmpty() || pos.isEmpty())
        return false;

    if (*this == pos)
        return true;

    if (node()->enclosingBlockFlowElement() != pos.node()->enclosingBlockFlowElement())
        return false;

    RenderPosition self = RenderPosition::fromDOMPosition(*this);
    RenderPosition other = RenderPosition::fromDOMPosition(pos);
    return RenderPosition::rendersOnSameLine(self, other);
}

bool Position::rendersInDifferentPosition(const Position &pos) const
{
    return RenderPosition::rendersInDifferentPosition(RenderPosition::fromDOMPosition(*this), RenderPosition::fromDOMPosition(pos));
    /*// qDebug() << *this << pos << endl;
    if (isEmpty() || pos.isEmpty())
        return false;

    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return false;

    RenderObject *posRenderer = pos.node()->renderer();
    if (!posRenderer)
        return false;

    if (renderer->style()->visibility() != khtml::VISIBLE ||
        posRenderer->style()->visibility() != khtml::VISIBLE)
        return false;

    if (node() == pos.node()) {
        if (node()->id() == ID_BR)
            return false;

        if (offset() == pos.offset())
            return false;

        if (!node()->isTextNode() && !pos.node()->isTextNode()) {
            if (offset() != pos.offset())
                return true;
        }
    }

    if (node()->id() == ID_BR && pos.inRenderedContent())
        return true;

    if (pos.node()->id() == ID_BR && inRenderedContent())
        return true;

    if (node()->enclosingBlockFlowElement() != pos.node()->enclosingBlockFlowElement())
        return true;

    if (node()->isTextNode() && !inRenderedText())
        return false;

    if (pos.node()->isTextNode() && !pos.inRenderedText())
        return false;

    long thisRenderedOffset = renderedOffset();
    long posRenderedOffset = pos.renderedOffset();

    if (renderer == posRenderer && thisRenderedOffset == posRenderedOffset)
        return false;

    // qDebug() << "onDifferentLine:" << (renderersOnDifferentLine(renderer, offset(), posRenderer, pos.offset()) ? "YES" : "NO");
    // qDebug() << "renderer:" << renderer << "[" << (renderer ? renderer->inlineBox(offset()) : 0) << "]";
    // qDebug() << "thisRenderedOffset:" << thisRenderedOffset;
    // qDebug() << "posRenderer:" << posRenderer <<  "[" << (posRenderer ? posRenderer->inlineBox(offset()) : 0) << "]";
    // qDebug() << "posRenderedOffset:"<< posRenderedOffset;
    // qDebug() << "node min/max:"<< node()->caretMinOffset() << "/" << node()->caretMaxRenderedOffset();
    // qDebug() << "pos node min/max:"<< pos.node()->caretMinOffset() << "/" << pos.node()->caretMaxRenderedOffset();
    // qDebug() << "----------------------------------------------------------------------";

    InlineBox *b1 = renderer ? renderer->inlineBox(offset()) : 0;
    InlineBox *b2 = posRenderer ? posRenderer->inlineBox(pos.offset()) : 0;

    if (!b1 || !b2) {
        return false;
    }

    if (b1->root() != b2->root()) {
        return true;
    }

    if (nextRenderedEditable(node()) == pos.node() &&
        thisRenderedOffset == (long)node()->caretMaxRenderedOffset() && posRenderedOffset == 0) {
        return false;
    }

    if (previousRenderedEditable(node()) == pos.node() &&
        thisRenderedOffset == 0 && posRenderedOffset == (long)pos.node()->caretMaxRenderedOffset()) {
        return false;
    }

    return true;*/
}

bool Position::isFirstRenderedPositionOnLine() const
{
    // return RenderPosition::fromDOMPosition(*this).isFirst*/
    // qDebug() << *this << endl;
    if (isEmpty())
        return false;

    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return false;
    // qDebug() << "Renderer" << renderer << renderer->renderName() << endl;

    if (renderer->style()->visibility() != khtml::VISIBLE)
        return false;

    Position pos(node(), offset());
    PositionIterator it(pos);
    while (!it.atStart()) {
        it.previous();
        // qDebug() << "To previous" << it.current() << endl;
        if (it.current().inRenderedContent())
            return !rendersOnSameLine(it.current());
            // return renderersOnDifferentLine(renderer, offset(), it.current().node()->renderer(), it.current().offset());
    }

    return true;
}

bool Position::isLastRenderedPositionOnLine() const
{
    if (isEmpty())
        return false;

    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return false;

    if (renderer->style()->visibility() != khtml::VISIBLE)
        return false;

    if (node()->id() == ID_BR)
        return true;

    Position pos(node(), offset());
    PositionIterator it(pos);
    while (!it.atEnd()) {
        it.next();
        if (it.current().inRenderedContent())
            return !rendersOnSameLine(it.current());
            // return renderersOnDifferentLine(renderer, offset(), it.current().node()->renderer(), it.current().offset());
    }

    return true;
}

bool Position::isLastRenderedPositionInEditableBlock() const
{
    if (isEmpty())
        return false;

    RenderObject *renderer = node()->renderer();
    if (!renderer)
        return false;

    if (renderer->style()->visibility() != khtml::VISIBLE)
        return false;

    if (renderedOffset() != (long)node()->caretMaxRenderedOffset())
        return false;

    Position pos(node(), offset());
    PositionIterator it(pos);
    while (!it.atEnd()) {
        it.next();
        if (!it.current().node()->inSameContainingBlockFlowElement(node()))
            return true;
        if (it.current().inRenderedContent())
            return false;
    }
    return true;
}

bool Position::inFirstEditableInRootEditableElement() const
{
    if (isEmpty() || !inRenderedContent())
        return false;

    PositionIterator it(*this);
    while (!it.atStart()) {
        if (it.previous().inRenderedContent())
            return false;
    }

    return true;
}

bool Position::inLastEditableInRootEditableElement() const
{
    if (isEmpty() || !inRenderedContent())
        return false;

    PositionIterator it(*this);
    while (!it.atEnd()) {
        if (it.next().inRenderedContent())
            return false;
    }

    return true;
}

bool Position::inFirstEditableInContainingEditableBlock() const
{
    if (isEmpty() || !RenderPosition::inRenderedContent(*this))
        return false;

    NodeImpl *block = node()->enclosingBlockFlowElement();

    PositionIterator it(*this);
    while (!it.atStart()) {
        it.previous();
        if (!RenderPosition::inRenderedContent(it.current()))
            continue;
        return block != it.current().node()->enclosingBlockFlowElement();
    }

    return true;
}

bool Position::inLastEditableInContainingEditableBlock() const
{
    if (isEmpty() || !RenderPosition::inRenderedContent(*this))
        return false;

    NodeImpl *block = node()->enclosingBlockFlowElement();

    PositionIterator it(*this);
    while (!it.atEnd()) {
        it.next();
        if (!RenderPosition::inRenderedContent(it.current()))
            continue;
        return block != it.current().node()->enclosingBlockFlowElement();
    }

    return true;
}

QDebug operator<<(QDebug stream, const Position& position)
{
    const NodeImpl* node = position.node();
    stream << "Position(" << node << (node ? node->nodeName() : QString()) << ":" << position.offset() << ")";
    return stream;
}

} // namespace DOM
