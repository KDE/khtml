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

#include "dom_selection.h"

#include "khtml_part.h"
#include "khtmlpart_p.h"
#include "khtmlview.h"
#include "dom/dom2_range.h"
#include "dom/dom_node.h"
#include "dom/dom_string.h"
#include "rendering/render_object.h"
#include "rendering/render_style.h"
#include "rendering/render_text.h"
#include "xml/dom_docimpl.h"
#include "xml/dom_positioniterator.h"
#include "xml/dom_elementimpl.h"
#include "xml/dom_nodeimpl.h"
#include "xml/dom_textimpl.h"

#include <QEvent>
#include <QPainter>
#include <QPaintEngine>
#include <QRect>

#define EDIT_DEBUG 0
#define DEBUG_CARET

using khtml::EditorContext;
using khtml::findWordBoundary;
using khtml::InlineTextBox;
using khtml::RenderObject;
using khtml::RenderText;
using khtml::RenderPosition;

namespace DOM {

static bool firstRunAt(RenderObject *renderNode, int y, NodeImpl *&startNode, long &startOffset);
static bool lastRunAt(RenderObject *renderNode, int y, NodeImpl *&endNode, long &endOffset);
static bool startAndEndLineNodesIncludingNode(NodeImpl *node, int offset, Selection &selection);

static inline Position &emptyPosition()
{
    static Position EmptyPosition = Position();
    return EmptyPosition;
}

Selection::Selection()
{
    init();
}

Selection::Selection(const Position &pos)
{
    init();
    assignBaseAndExtent(pos, pos);
    validate();
}

Selection::Selection(const Range &r)
{
    const Position start(r.startContainer().handle(), r.startOffset());
    const Position end(r.endContainer().handle(), r.endOffset());

    init();
    assignBaseAndExtent(start, end);
    validate();
}

Selection::Selection(const Position &base, const Position &extent)
{
    init();
    assignBaseAndExtent(base, extent);
    validate();
}

Selection::Selection(const Selection &o)
{
    init();

    assignBaseAndExtent(o.base(), o.extent());
    assignStartAndEnd(o.start(), o.end());

    m_state = o.m_state;
    m_affinity = o.m_affinity;

    m_baseIsStart = o.m_baseIsStart;
    m_needsCaretLayout = o.m_needsCaretLayout;
    m_modifyBiasSet = o.m_modifyBiasSet;

    // Only copy the coordinates over if the other object
    // has had a layout, otherwise keep the current
    // coordinates. This prevents drawing artifacts from
    // remaining when the caret is painted and then moves,
    // and the old rectangle needs to be repainted.
    if (!m_needsCaretLayout) {
        m_caretX = o.m_caretX;
        m_caretY = o.m_caretY;
        m_caretSize = o.m_caretSize;
    }
}

void Selection::init()
{
    m_base = m_extent = m_start = m_end = emptyPosition();
    m_state = NONE;
    m_caretX = 0;
    m_caretY = 0;
    m_caretSize = 0;
    m_baseIsStart = true;
    m_needsCaretLayout = true;
    m_modifyBiasSet = false;
    m_affinity = DOWNSTREAM;
}

Selection &Selection::operator=(const Selection &o)
{
    assignBaseAndExtent(o.base(), o.extent());
    assignStartAndEnd(o.start(), o.end());

    m_state = o.m_state;
    m_affinity = o.m_affinity;

    m_baseIsStart = o.m_baseIsStart;
    m_needsCaretLayout = o.m_needsCaretLayout;
    m_modifyBiasSet = o.m_modifyBiasSet;

    // Only copy the coordinates over if the other object
    // has had a layout, otherwise keep the current
    // coordinates. This prevents drawing artifacts from
    // remaining when the caret is painted and then moves,
    // and the old rectangle needs to be repainted.
    if (!m_needsCaretLayout) {
        m_caretX = o.m_caretX;
        m_caretY = o.m_caretY;
        m_caretSize = o.m_caretSize;
    }

    return *this;
}

void Selection::setAffinity(EAffinity affinity)
{
    if (affinity == m_affinity)
        return;

    m_affinity = affinity;
    setNeedsLayout();
}

void Selection::moveTo(const Range &r)
{
    Position start(r.startContainer().handle(), r.startOffset());
    Position end(r.endContainer().handle(), r.endOffset());
    moveTo(start, end);
}

void Selection::moveTo(const Selection &o)
{
    moveTo(o.start(), o.end());
}

void Selection::moveTo(const Position &pos)
{
    moveTo(pos, pos);
}

void Selection::moveTo(const Position &base, const Position &extent)
{
//   kdDebug(6200) << "Selection::moveTo: base(" << base.node() << "," << base.offset() << "), extent(" << extent.node() << "," << extent.offset() << ")" << endl;
#ifdef DEBUG_CARET
    qDebug() << *this << base << extent << endl;
#endif
    assignBaseAndExtent(base, extent);
    validate();
}

bool Selection::modify(EAlter alter, EDirection dir, ETextGranularity granularity)
{
    Position pos;

    switch (dir) {
        // EDIT FIXME: This needs to handle bidi
        case RIGHT:
        case FORWARD:
            if (alter == EXTEND) {
                if (!m_modifyBiasSet) {
                    m_modifyBiasSet = true;
                    assignBaseAndExtent(start(), end());
                }
                switch (granularity) {
                    case CHARACTER:
                        pos = extent().nextCharacterPosition();
                        break;
                    case WORD:
                        pos = extent().nextWordPosition();
                        break;
                    case LINE:
                        pos = extent().nextLinePosition(xPosForVerticalArrowNavigation(EXTENT));
                        break;
                    case PARAGRAPH:
                        // not implemented
                        break;
                }
            }
            else {
                m_modifyBiasSet = false;
                switch (granularity) {
                    case CHARACTER:
                        pos = (state() == RANGE) ? end() : extent().nextCharacterPosition();
                        break;
                    case WORD:
                        pos = extent().nextWordPosition();
                        break;
                    case LINE:
                        pos = end().nextLinePosition(xPosForVerticalArrowNavigation(END, state() == RANGE));
                        break;
                    case PARAGRAPH:
                        // not implemented
                        break;
                }
            }
            break;
        // EDIT FIXME: This needs to handle bidi
        case LEFT:
        case BACKWARD:
            if (alter == EXTEND) {
                if (!m_modifyBiasSet) {
                    m_modifyBiasSet = true;
                    assignBaseAndExtent(end(), start());
                }
                switch (granularity) {
                    case CHARACTER:
                        pos = extent().previousCharacterPosition();
                        break;
                    case WORD:
                        pos = extent().previousWordPosition();
                        break;
                    case LINE:
                        pos = extent().previousLinePosition(xPosForVerticalArrowNavigation(EXTENT));
                        break;
                    case PARAGRAPH:
                        // not implemented
                        break;
                }
            }
            else {
                m_modifyBiasSet = false;
                switch (granularity) {
                    case CHARACTER:
                        pos = (state() == RANGE) ? start() : extent().previousCharacterPosition();
                        break;
                    case WORD:
                        pos = extent().previousWordPosition();
                        break;
                    case LINE:
                        pos = start().previousLinePosition(xPosForVerticalArrowNavigation(START, state() == RANGE));
                        break;
                    case PARAGRAPH:
                        // not implemented
                        break;
                }
            }
            break;
    }

    if (pos.isEmpty())
        return false;

    if (alter == MOVE)
        moveTo(pos);
    else // alter == EXTEND
        setExtent(pos);

    return true;
}

bool Selection::expandUsingGranularity(ETextGranularity granularity)
{
    if (state() == NONE)
        return false;

    validate(granularity);
    return true;
}

int Selection::xPosForVerticalArrowNavigation(EPositionType type, bool recalc) const
{
    int x = 0;

    if (state() == NONE)
        return x;

    Position pos;
    switch (type) {
        case START:
            pos = start();
            break;
        case END:
            pos = end();
            break;
        case BASE:
            pos = base();
            break;
        case EXTENT:
            pos = extent();
            break;
        case CARETPOS:
            pos = caretPos();
            break;
    }

    KHTMLPart *part = pos.node()->document()->part();
    if (!part)
        return x;

    if (recalc || part->d->editor_context.m_xPosForVerticalArrowNavigation == EditorContext::NoXPosForVerticalArrowNavigation
       )
    {
        int y, w, h;
        if (pos.node()->renderer())
            pos.node()->renderer()->caretPos(pos.renderedOffset(), 0, x, y, w, h);
        part->d->editor_context.m_xPosForVerticalArrowNavigation = x;
    }
    else {
        x = part->d->editor_context.m_xPosForVerticalArrowNavigation;
    }

    return x;
}

void Selection::clear()
{
    assignBaseAndExtent(emptyPosition(), emptyPosition());
    validate();
}

void Selection::collapse()
{
    moveTo(caretPos());
}

void Selection::setBase(const Position &pos)
{
    assignBase(pos);
    validate();
}

void Selection::setExtent(const Position &pos)
{
    assignExtent(pos);
    validate();
}

void Selection::setBaseAndExtent(const Position &base, const Position &extent)
{
    assignBaseAndExtent(base, extent);
    validate();
}

void Selection::setStart(const Position &pos)
{
    assignStart(pos);
    validate();
}

void Selection::setEnd(const Position &pos)
{
    assignEnd(pos);
    validate();
}

void Selection::setStartAndEnd(const Position &start, const Position &end)
{
    assignStartAndEnd(start, end);
    validate();
}

void Selection::setNeedsLayout(bool flag)
{
    m_needsCaretLayout = flag;
}

void Selection::getRange(NodeImpl *&st, long &so, NodeImpl *&en, long &eo) const
{
    if (isEmpty()) {
        st = en = 0; so = eo = 0;
        return;
    }

    // Make sure we have an updated layout since this function is called
    // in the course of running edit commands which modify the DOM.
    // Failing to call this can result in equivalentXXXPosition calls returning
    // incorrect results.
    start().node()->document()->updateLayout();

    Position s, e;
    if (state() == CARET) {
        // If the selection is a caret, move the range start upstream. This helps us match
        // the conventions of text editors tested, which make style determinations based
        // on the character before the caret, if any.
        s = start().equivalentUpstreamPosition().equivalentRangeCompliantPosition();
        e = s;
    }
    else {
        // If the selection is a range, select the minimum range that encompasses the selection.
        // Again, this is to match the conventions of text editors tested, which make style
        // determinations based on the first character of the selection.
        // For instance, this operation helps to make sure that the "X" selected below is the
        // only thing selected. The range should not be allowed to "leak" out to the end of the
        // previous text node, or to the beginning of the next text node, each of which has a
        // different style.
        //
        // On a treasure map, <b>X</b> marks the spot.
        //                       ^ selected
        //
        assert(state() == RANGE);
        s = start().equivalentDownstreamPosition();
        e = end().equivalentUpstreamPosition();
        if ((s.node() == e.node() && s.offset() > e.offset()) || !nodeIsBeforeNode(s.node(), e.node())) {
            // Make sure the start is before the end.
            // The end can wind up before the start if collapsed whitespace is the only thing selected.
            Position tmp = s;
            s = e;
            e = tmp;
        }
        s = s.equivalentRangeCompliantPosition();
        e = e.equivalentRangeCompliantPosition();
    }

    st = s.node();
    so = s.offset();
    en = e.node();
    eo = e.offset();
}

Range Selection::toRange() const
{
    if (isEmpty())
        return Range();

    NodeImpl *start, *end;
    long so, eo;
    getRange(start, so, end, eo);
    return Range(Node(start), so, Node(end), eo);
}

void Selection::layoutCaret()
{
    if (isEmpty() || !caretPos().node()->renderer()) {
        m_caretX = m_caretY = m_caretSize = 0;
    }
    else {
        // EDIT FIXME: Enhance call to pass along selection
        // upstream/downstream affinity to get the right position.
        int w;
        int offset = RenderPosition::fromDOMPosition(caretPos()).renderedOffset();
#ifdef DEBUG_CARET
        qDebug() << "[before caretPos()]" << m_caretX << endl;
#endif
        caretPos().node()->renderer()->caretPos(offset, true, m_caretX, m_caretY, w, m_caretSize);
#ifdef DEBUG_CARET
        qDebug() << "[after caretPos()]" << m_caretX << endl;
#endif
    }

    m_needsCaretLayout = false;
}

QRect Selection::getRepaintRect() const
{
    if (m_needsCaretLayout) {
        const_cast<Selection *>(this)->layoutCaret();
    }

    // EDIT FIXME: fudge a bit to make sure we don't leave behind artifacts
    return QRect(m_caretX - 1, m_caretY - 1, 3, m_caretSize + 2);
}

void Selection::needsCaretRepaint()
{
    if (isEmpty())
        return;

    if (!start().node())
        return;

    if (!start().node()->document())
        return;

    KHTMLView *v = caretPos().node()->document()->view();
    if (!v)
        return;

    // qDebug() << "[NeedsCaretLayout]" << m_needsCaretLayout << endl;
    if (m_needsCaretLayout) {
        // repaint old position and calculate new position
        v->updateContents(getRepaintRect());
        layoutCaret();

        // EDIT FIXME: This is an unfortunate hack.
        // Basically, we can't trust this layout position since we
        // can't guarantee that the check to see if we are in unrendered
        // content will work at this point. We may have to wait for
        // a layout and re-render of the document to happen. So, resetting this
        // flag will cause another caret layout to happen the first time
        // that we try to paint the caret after this call. That one will work since
        // it happens after the document has accounted for any editing
        // changes which may have been done.
        // And, we need to leave this layout here so the caret moves right
        // away after clicking.
        m_needsCaretLayout = true;
    }
    v->updateContents(getRepaintRect());

}

void Selection::paintCaret(QPainter *p, const QRect &rect)
{
    if (isEmpty())
        return;

    if (m_state == NONE)
        return;

    if (m_needsCaretLayout) {
        Position pos = caretPos();
        if (!pos.inRenderedContent()) {
            // ### wrong wrong wrong wrong wrong. this will break quanta vpl
            moveToRenderedContent();
        }
        layoutCaret();
    }

    QRect caretRect(m_caretX, m_caretY, 1, m_caretSize);
    if (caretRect.intersects(rect)) {
        QPainter::CompositionMode oldop = p->compositionMode();
        QColor c = Qt::black;
        if (p->paintEngine() && p->paintEngine()->hasFeature(QPaintEngine::BlendModes)) {
            p->setCompositionMode(QPainter::CompositionMode_Difference);
            c =  Qt::white;
        } else {
            p->setCompositionMode(QPainter::CompositionMode_Xor);
        }
        p->fillRect(caretRect.left(), caretRect.top(), 1, caretRect.height(), c);
        p->setCompositionMode(oldop);
    }
}

void Selection::validate(ETextGranularity granularity)
{
#ifdef DEBUG_CARET
    qDebug() << *this << granularity << endl;
#endif
    // move the base and extent nodes to their equivalent leaf positions
    bool baseAndExtentEqual = base() == extent();
    if (base().notEmpty()) {
#ifdef DEBUG_CARET
        qDebug() << "[base not empty]" << endl;
#endif
        Position pos = base().equivalentLeafPosition();
        assignBase(pos);
        if (baseAndExtentEqual)
            assignExtent(pos);
    }
    if (extent().notEmpty() && !baseAndExtentEqual) {
        assignExtent(extent().equivalentLeafPosition());
    }

    // make sure we do not have a dangling start or end. In particular, if one
    // of base or extent is empty, we use the other one (which may be empty as
    // well) for everything, before getting into the code that computes
    // start + end from base + extent based on granularity.
    if (base().isEmpty()) {
        assignBaseAndExtent(extent(), extent());
        m_baseIsStart = true;
    }
    else if (extent().isEmpty()) {
        assignBaseAndExtent(base(), base());
        m_baseIsStart = true;
    }
    else {
        // adjust m_baseIsStart as needed
        if (base().node() == extent().node()) {
            if (base().offset() > extent().offset())
                m_baseIsStart = false;
            else
                m_baseIsStart = true;
        }
        else if (nodeIsBeforeNode(base().node(), extent().node()))
            m_baseIsStart = true;
        else
            m_baseIsStart = false;
    }

    // calculate the correct start and end positions
    if (granularity == CHARACTER) {
#ifdef DEBUG_CARET
        qDebug() << "[character:baseIsStart]" << m_baseIsStart << base() << extent() << endl;
#endif
        if (m_baseIsStart)
            assignStartAndEnd(base(), extent());
        else
            assignStartAndEnd(extent(), base());
    }
    else if (granularity == WORD) {
        int baseStartOffset = base().offset();
        int baseEndOffset = base().offset();
        int extentStartOffset = extent().offset();
        int extentEndOffset = extent().offset();
#ifdef DEBUG_CARET
        qDebug() << "WORD GRANULARITY:" << baseStartOffset << baseEndOffset << extentStartOffset << extentEndOffset << endl;
#endif
        if (base().notEmpty() && (base().node()->nodeType() == Node::TEXT_NODE || base().node()->nodeType() == Node::CDATA_SECTION_NODE)) {
            DOMString t = base().node()->nodeValue();
            QChar *chars = t.unicode();
            uint len = t.length();
#ifdef DEBUG_CARET
            qDebug() << "text:" << QString::fromRawData(chars, len) << endl;
#endif
            findWordBoundary(chars, len, base().offset(), &baseStartOffset, &baseEndOffset);
#ifdef DEBUG_CARET
            qDebug() << "after find word boundary" << baseStartOffset << baseEndOffset << endl;
#endif
        }
        if (extent().notEmpty() && (extent().node()->nodeType() == Node::TEXT_NODE || extent().node()->nodeType() == Node::CDATA_SECTION_NODE)) {
            DOMString t = extent().node()->nodeValue();
            QChar *chars = t.unicode();
            uint len = t.length();
#ifdef DEBUG_CARET
            qDebug() << "text:" << QString::fromRawData(chars, len) << endl;
#endif
            findWordBoundary(chars, len, extent().offset(), &extentStartOffset, &extentEndOffset);
#ifdef DEBUG_CARET
            qDebug() << "after find word boundary" << baseStartOffset << baseEndOffset << endl;
#endif
        }
#ifdef DEBUG_CARET
        qDebug() << "is start:" << m_baseIsStart << endl;
#endif
        if (m_baseIsStart) {
            assignStart(Position(base().node(), baseStartOffset));
            assignEnd(Position(extent().node(), extentEndOffset));
        }
        else {
            assignStart(Position(extent().node(), extentStartOffset));
            assignEnd(Position(base().node(), baseEndOffset));
        }
    }
    else {  // granularity == LINE
        Selection baseSelection = *this;
        Selection extentSelection = *this;
        if (base().notEmpty() && (base().node()->nodeType() == Node::TEXT_NODE || base().node()->nodeType() == Node::CDATA_SECTION_NODE)) {
            if (startAndEndLineNodesIncludingNode(base().node(), base().offset(), baseSelection)) {
                assignStart(Position(baseSelection.base().node(), baseSelection.base().offset()));
                assignEnd(Position(baseSelection.extent().node(), baseSelection.extent().offset()));
            }
        }
        if (extent().notEmpty() && (extent().node()->nodeType() == Node::TEXT_NODE || extent().node()->nodeType() == Node::CDATA_SECTION_NODE)) {
            if (startAndEndLineNodesIncludingNode(extent().node(), extent().offset(), extentSelection)) {
                assignStart(Position(extentSelection.base().node(), extentSelection.base().offset()));
                assignEnd(Position(extentSelection.extent().node(), extentSelection.extent().offset()));
            }
        }
        if (m_baseIsStart) {
            assignStart(baseSelection.start());
            assignEnd(extentSelection.end());
        }
        else {
            assignStart(extentSelection.start());
            assignEnd(baseSelection.end());
        }
    }

    // adjust the state
    if (start().isEmpty() && end().isEmpty())
        m_state = NONE;
    else if (start() == end())
        m_state = CARET;
    else
        m_state = RANGE;

     if (start().isEmpty())
         assert (m_state == NONE);

    m_needsCaretLayout = true;

#if EDIT_DEBUG
    debugPosition();
#endif
}

bool Selection::moveToRenderedContent()
{
    if (isEmpty())
        return false;

    if (m_state != CARET)
        return false;

    Position pos = start();
    if (pos.inRenderedContent())
        return true;

    // not currently rendered, try moving to prev
    Position prev = pos.previousCharacterPosition();
    if (prev != pos && prev.node()->inSameContainingBlockFlowElement(pos.node())) {
        moveTo(prev);
        return true;
    }

    // could not be moved to prev, try next
    Position next = pos.nextCharacterPosition();
    if (next != pos && next.node()->inSameContainingBlockFlowElement(pos.node())) {
        moveTo(next);
        return true;
    }

    return false;
}

bool Selection::nodeIsBeforeNode(NodeImpl *n1, NodeImpl *n2) const
{
    if (!n1 || !n2)
        return true;

    if (n1 == n2)
        return true;

    bool result = false;
    int n1Depth = 0;
    int n2Depth = 0;

    // First we find the depths of the two nodes in the tree (n1Depth, n2Depth)
    NodeImpl *n = n1;
    while (n->parentNode()) {
        n = n->parentNode();
        n1Depth++;
    }
    n = n2;
    while (n->parentNode()) {
        n = n->parentNode();
        n2Depth++;
    }
    // Climb up the tree with the deeper node, until both nodes have equal depth
    while (n2Depth > n1Depth) {
        n2 = n2->parentNode();
        n2Depth--;
    }
    while (n1Depth > n2Depth) {
        n1 = n1->parentNode();
        n1Depth--;
    }
    // Climb the tree with both n1 and n2 until they have the same parent
    while (n1->parentNode() != n2->parentNode()) {
        n1 = n1->parentNode();
        n2 = n2->parentNode();
    }
    // Iterate through the parent's children until n1 or n2 is found
    n = n1->parentNode() ? n1->parentNode()->firstChild() : n1->firstChild();
    while (n) {
        if (n == n1) {
            result = true;
            break;
        }
        else if (n == n2) {
            result = false;
            break;
        }
        n = n->nextSibling();
    }
    return result;
}

static bool firstRunAt(RenderObject *renderNode, int y, NodeImpl *&startNode, long &startOffset)
{
    for (RenderObject *n = renderNode; n; n = n->nextSibling()) {
        if (n->isText()) {
            RenderText *textRenderer = static_cast<khtml::RenderText *>(n);
            for (InlineTextBox *box = textRenderer->firstTextBox(); box; box = box->nextTextBox()) {
                if (box->m_y == y) {
                    startNode = textRenderer->element();
                    startOffset = box->m_start;
                    return true;
                }
            }
        }

        if (firstRunAt(n->firstChild(), y, startNode, startOffset)) {
            return true;
        }
    }

    return false;
}

static bool lastRunAt(RenderObject *renderNode, int y, NodeImpl *&endNode, long &endOffset)
{
    RenderObject *n = renderNode;
    if (!n) {
        return false;
    }
    RenderObject *next;
    while ((next = n->nextSibling())) {
        n = next;
    }

    while (1) {
        if (lastRunAt(n->firstChild(), y, endNode, endOffset)) {
            return true;
        }

        if (n->isText()) {
            RenderText *textRenderer =  static_cast<khtml::RenderText *>(n);
            for (InlineTextBox *box = textRenderer->lastTextBox(); box; box = box->prevTextBox()) {
                if (box->m_y == y) {
                    endNode = textRenderer->element();
                    endOffset = box->m_start + box->m_len;
                    return true;
                }
            }
        }

        if (n == renderNode) {
            return false;
        }

        n = n->previousSibling();
    }
}

static bool startAndEndLineNodesIncludingNode(NodeImpl *node, int offset, Selection &selection)
{
    if (node && node->renderer() && (node->nodeType() == Node::TEXT_NODE || node->nodeType() == Node::CDATA_SECTION_NODE)) {
        int pos;
        int selectionPointY;
        RenderPosition rp = RenderPosition::fromDOMPosition(Position(node, offset));
        pos = rp.renderedOffset();
        // RenderText *renderer = static_cast<RenderText *>(node->renderer());
        // const InlineTextBox * run = renderer->findInlineTextBox( offset, pos );
        const InlineTextBox* run = static_cast<InlineTextBox*>(node->renderer()->inlineBox(pos));
        DOMString t = node->nodeValue();

        if (!run)
            return false;

        selectionPointY = run->m_y;

        // Go up to first non-inline element.
        khtml::RenderObject *renderNode = node->renderer();
        while (renderNode && renderNode->isInline())
            renderNode = renderNode->parent();

        if (renderNode)
            renderNode = renderNode->firstChild();

        NodeImpl *startNode = 0;
        NodeImpl *endNode = 0;
        long startOffset;
        long endOffset;

        // Look for all the first child in the block that is on the same line
        // as the selection point.
        if (!firstRunAt (renderNode, selectionPointY, startNode, startOffset))
            return false;

        // Look for all the last child in the block that is on the same line
        // as the selection point.
        if (!lastRunAt (renderNode, selectionPointY, endNode, endOffset))
            return false;

        selection.moveTo(RenderPosition(startNode, startOffset).position(), RenderPosition(endNode, endOffset).position());

        return true;
    }
    return false;
}

void Selection::debugRenderer(RenderObject *r, bool selected) const
{
    if (r->node()->isElementNode()) {
        ElementImpl *element = static_cast<ElementImpl *>(r->node());
        fprintf(stderr, "%s%s\n", selected ? "==> " : "    ", element->tagName().string().toLatin1().data());
    }
    else if (r->isText()) {
        RenderText *textRenderer = static_cast<RenderText *>(r);
        if (textRenderer->stringLength() == 0 || !textRenderer->firstTextBox()) {
            fprintf(stderr, "%s#text (empty)\n", selected ? "==> " : "    ");
            return;
        }

        static const int max = 36;
        QString text = DOMString(textRenderer->string()).string();
        int textLength = text.length();
        if (selected) {
            int offset = 0;
            if (r->node() == start().node())
                offset = start().offset();
            else if (r->node() == end().node())
                offset = end().offset();

            int pos;
            const InlineTextBox *box = textRenderer->findInlineTextBox(offset, pos);
            text = text.mid(box->m_start, box->m_len);

            QString show;
            int mid = max / 2;
            int caret = 0;

            // text is shorter than max
            if (textLength < max) {
                show = text;
                caret = pos;
            }

            // too few characters to left
            else if (pos - mid < 0) {
                show = text.left(max - 3) + "...";
                caret = pos;
            }

            // enough characters on each side
            else if (pos - mid >= 0 && pos + mid <= textLength) {
                show = "..." + text.mid(pos - mid + 3, max - 6) + "...";
                caret = mid;
            }

            // too few characters on right
            else {
                show = "..." + text.right(max - 3);
                caret = pos - (textLength - show.length());
            }

            show = show.replace("\n", " ");
            show = show.replace("\r", " ");
            fprintf(stderr, "==> #text : \"%s\" at offset %d\n", show.toLatin1().data(), pos);
            fprintf(stderr, "           ");
            for (int i = 0; i < caret; i++)
                fprintf(stderr, " ");
            fprintf(stderr, "^\n");
        }
        else {
            if ((int)text.length() > max)
                text = text.left(max - 3) + "...";
            else
                text = text.left(max);
            fprintf(stderr, "    #text : \"%s\"\n", text.toLatin1().data());
        }
    }
}

void Selection::debugPosition() const
{
    if (!start().node())
        return;

    //static int context = 5;

    //RenderObject *r = 0;

    fprintf(stderr, "Selection =================\n");

    if (start() == end()) {
        Position pos = start();
        Position upstream = pos.equivalentUpstreamPosition();
        Position downstream = pos.equivalentDownstreamPosition();
        /*FIXME:use qDebug() fprintf(stderr, "upstream:   %s %p:%ld\n", getTagName(upstream.node()->id())
            , upstream.node(), upstream.offset());
        fprintf(stderr, "pos:        %s %p:%ld\n", getTagName(pos.node()->id())
            , pos.node(), pos.offset());
        fprintf(stderr, "downstream: %s %p:%ld\n", getTagName(downstream.node()->id())
            , downstream.node(), downstream.offset());*/
    }
    else {
        Position pos = start();
        Position upstream = pos.equivalentUpstreamPosition();
        Position downstream = pos.equivalentDownstreamPosition();
        /*FIXME: use qDebug() fprintf(stderr, "upstream:   %s %p:%ld\n", getTagName(upstream.node()->id())
            , upstream.node(), upstream.offset());
        fprintf(stderr, "start:      %s %p:%ld\n", getTagName(pos.node()->id())
            , pos.node(), pos.offset());
        fprintf(stderr, "downstream: %s %p:%ld\n", getTagName(downstream.node()->id())
            , downstream.node(), downstream.offset());
        fprintf(stderr, "-----------------------------------\n");*/
        pos = end();
        upstream = pos.equivalentUpstreamPosition();
        downstream = pos.equivalentDownstreamPosition();
        /*FIXME: use qDebug() fprintf(stderr, "upstream:   %s %p:%ld\n", getTagName(upstream.node()->id())
            , upstream.node(), upstream.offset());
        fprintf(stderr, "end:        %s %p:%ld\n", getTagName(pos.node()->id())
            , pos.node(), pos.offset());
        fprintf(stderr, "downstream: %s %p:%ld\n", getTagName(downstream.node()->id())
            , downstream.node(), downstream.offset());
        fprintf(stderr, "-----------------------------------\n");*/
    }

#if 0
    int back = 0;
    r = start().node()->renderer();
    for (int i = 0; i < context; i++, back++) {
        if (r->previousRenderer())
            r = r->previousRenderer();
        else
            break;
    }
    for (int i = 0; i < back; i++) {
        debugRenderer(r, false);
        r = r->nextRenderer();
    }


    fprintf(stderr, "\n");

    if (start().node() == end().node())
        debugRenderer(start().node()->renderer(), true);
    else
        for (r = start().node()->renderer(); r && r != end().node()->renderer(); r = r->nextRenderer())
            debugRenderer(r, true);

    fprintf(stderr, "\n");

    r = end().node()->renderer();
    for (int i = 0; i < context; i++) {
        if (r->nextRenderer()) {
            r = r->nextRenderer();
            debugRenderer(r, false);
        }
        else
            break;
    }
#endif

    fprintf(stderr, "================================\n");
}

QDebug operator<<(QDebug stream, const Selection& selection)
{
    stream << "Selection["
        << selection.base()
        << selection.extent()
        << selection.start()
        << selection.end()
        << selection.affinity() << "]";
    return stream;
}

} // namespace DOM
