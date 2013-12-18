/**
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999-2003 Antti Koivisto (koivisto@kde.org)
 *           (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 *           (C) 2003-2007 Apple Computer, Inc.
 *           (C) 2007 Germain Garand (germain@ebooksfrance.org)
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
// -------------------------------------------------------------------------

#include "render_flow.h"

#include <QDebug>
#include <assert.h>
#include <QPainter>

#include "render_text.h"
#include "render_table.h"
#include "render_canvas.h"
#include "render_inline.h"
#include "render_block.h"
#include "render_arena.h"
#include "render_line.h"
#include <xml/dom_nodeimpl.h>
#include <xml/dom_docimpl.h>
#include <html/html_formimpl.h>

#include <khtmlview.h>

using namespace DOM;
using namespace khtml;

RenderFlow* RenderFlow::createFlow(DOM::NodeImpl* node, RenderStyle* style, RenderArena* arena)
{
    RenderFlow* result;
    if (style->display() == INLINE)
        result = new (arena) RenderInline(node);
    else
        result = new (arena) RenderBlock(node);
    result->setStyle(style);
    return result;
}

RenderFlow* RenderFlow::continuationBefore(RenderObject* beforeChild)
{
    if (beforeChild && beforeChild->parent() == this)
        return this;

    RenderFlow* curr = continuation();
    RenderFlow* nextToLast = this;
    RenderFlow* last = this;
    while (curr) {
        if (beforeChild && beforeChild->parent() == curr) {
            if (curr->firstChild() == beforeChild)
                return last;
            return curr;
        }

        nextToLast = last;
        last = curr;
        curr = curr->continuation();
    }

    if (!beforeChild && !last->firstChild())
        return nextToLast;
    return last;
}

void RenderFlow::addChildWithContinuation(RenderObject* newChild, RenderObject* beforeChild)
{
    RenderFlow* flow = continuationBefore(beforeChild);
    while(beforeChild && beforeChild->parent() != flow && !beforeChild->parent()->isAnonymousBlock()) {
        // skip implicit containers around beforeChild
        beforeChild = beforeChild->parent();
    }
    RenderFlow* beforeChildParent = beforeChild ? static_cast<RenderFlow*>(beforeChild->parent()) :
                                    (flow->continuation() ? flow->continuation() : flow);

    if (newChild->isFloatingOrPositioned())
        return beforeChildParent->addChildToFlow(newChild, beforeChild);

    // A continuation always consists of two potential candidates: an inline or an anonymous
    // block box holding block children.
    bool childInline = newChild->isInline();
    bool bcpInline = beforeChildParent->isInline();
    bool flowInline = flow->isInline();

    if (flow == beforeChildParent)
        return flow->addChildToFlow(newChild, beforeChild);
    else {
        // The goal here is to match up if we can, so that we can coalesce and create the
        // minimal # of continuations needed for the inline.
        if (childInline == bcpInline)
            return beforeChildParent->addChildToFlow(newChild, beforeChild);
        else if (flowInline == childInline)
            return flow->addChildToFlow(newChild, 0); // Just treat like an append.
        else
            return beforeChildParent->addChildToFlow(newChild, beforeChild);
    }
}

void RenderFlow::addChild(RenderObject *newChild, RenderObject *beforeChild)
{
#ifdef DEBUG_LAYOUT
    // qDebug() << renderName() << "(RenderFlow)::addChild( " << newChild->renderName() <<
                       ", " << (beforeChild ? beforeChild->renderName() : "0") << " )" << endl;
    // qDebug() << "current height = " << m_height;
#endif

    if (continuation())
        return addChildWithContinuation(newChild, beforeChild);
    return addChildToFlow(newChild, beforeChild);
}

void RenderFlow::extractLineBox(InlineFlowBox* box)
{
    m_lastLineBox = box->prevFlowBox();
    if (box == m_firstLineBox)
        m_firstLineBox = 0;
    if (box->prevLineBox())
        box->prevLineBox()->setNextLineBox(0);
    box->setPreviousLineBox(0);
    for (InlineRunBox* curr = box; curr; curr = curr->nextLineBox())
        curr->setExtracted();
}

void RenderFlow::attachLineBox(InlineFlowBox* box)
{
    if (m_lastLineBox) {
        m_lastLineBox->setNextLineBox(box);
        box->setPreviousLineBox(m_lastLineBox);
    } else
        m_firstLineBox = box;
    InlineFlowBox* last = box;
    for (InlineFlowBox* curr = box; curr; curr = curr->nextFlowBox()) {
        curr->setExtracted(false);
        last = curr;
    }
    m_lastLineBox = last;
}

void RenderFlow::removeInlineBox(InlineBox* _box)
{
    if ( _box->isInlineFlowBox() ) {
        InlineFlowBox* box = static_cast<InlineFlowBox*>(_box);
        if (box == m_firstLineBox)
            m_firstLineBox = box->nextFlowBox();
        if (box == m_lastLineBox)
            m_lastLineBox = box->prevFlowBox();
        if (box->nextLineBox())
            box->nextLineBox()->setPreviousLineBox(box->prevLineBox());
        if (box->prevLineBox())
            box->prevLineBox()->setNextLineBox(box->nextLineBox());
    }
    RenderBox::removeInlineBox( _box );
}

void RenderFlow::deleteInlineBoxes(RenderArena* arena)
{
    if (m_firstLineBox) {
        if (!arena)
            arena = renderArena();
        InlineRunBox *curr=m_firstLineBox, *next=0;
        while (curr) {
            next = curr->nextLineBox();
            if (!curr->isPlaceHolderBox())
                curr->detach(arena, true /*noRemove*/);
            curr = next;
        }
        m_firstLineBox = 0;
        m_lastLineBox = 0;
    }
}

void RenderFlow::dirtyInlineBoxes(bool fullLayout, bool isRootLineBox)
{
    if (!isRootLineBox && (isReplaced() || isPositioned()))
        return RenderBox::dirtyInlineBoxes(fullLayout, isRootLineBox);

    if (fullLayout)
        deleteInlineBoxes();
    else {
        for (InlineRunBox* curr = firstLineBox(); curr; curr = curr->nextLineBox())
            curr->dirtyInlineBoxes();
    }
}

void RenderFlow::deleteLastLineBox(RenderArena* arena)
{
    if (m_lastLineBox) {
        if (!arena)
            arena = renderArena();
        InlineRunBox *curr=m_lastLineBox, *prev = m_lastLineBox;
        if (m_firstLineBox == m_lastLineBox)
            m_firstLineBox = m_lastLineBox = 0;
        else {
            prev = curr->prevLineBox();
            while (!prev->isInlineFlowBox()) {
                prev = prev->prevLineBox();
                prev->detach(arena);
            }
            m_lastLineBox = static_cast<InlineFlowBox*>(prev);
            prev->setNextLineBox(0);
        }
        if (curr->parent()) {
            curr->parent()->removeFromLine(curr);
        }
        curr->detach(arena);
    }
}

InlineBox* RenderFlow::createInlineBox(bool makePlaceHolderBox, bool isRootLineBox)
{
    if ( !isRootLineBox &&
         (isReplaced() || makePlaceHolderBox) )       // Inline tables and inline blocks
         return RenderBox::createInlineBox(false, false);    // (or positioned element placeholders).

    InlineFlowBox* flowBox = 0;
    if (isInlineFlow())
        flowBox = new (renderArena()) InlineFlowBox(this);
    else
        flowBox = new (renderArena()) RootInlineBox(this);

    if (!m_firstLineBox) {
        m_firstLineBox = m_lastLineBox = flowBox;
    } else {
        m_lastLineBox->setNextLineBox(flowBox);
        flowBox->setPreviousLineBox(m_lastLineBox);
        m_lastLineBox = flowBox;
    }

    return flowBox;
}

void RenderFlow::dirtyLinesFromChangedChild(RenderObject* child)
{
    if (!parent() || (selfNeedsLayout() && !isInlineFlow()) || isTable())
        return;

    // If we have no first line box, then just bail early.
    if (!firstLineBox()) {
        // For an empty inline, propagate the check up to our parent, unless the parent
        // is already dirty.
        if (isInline() && !parent()->selfNeedsLayout() && parent()->isInlineFlow())
            static_cast<RenderFlow*>(parent())->dirtyLinesFromChangedChild(this);
        return;
    }

    // Try to figure out which line box we belong in.  First try to find a previous
    // line box by examining our siblings.  If we didn't find a line box, then use our 
    // parent's first line box.
    RootInlineBox* box = 0;
    RenderObject* curr = 0;
    for (curr = child->previousSibling(); curr; curr = curr->previousSibling()) {
        if (curr->isFloatingOrPositioned())
            continue;

        if (curr->isReplaced() && curr->isBox()) {
            InlineBox* placeHolderBox = static_cast<RenderBox*>(curr)->placeHolderBox();
            if (placeHolderBox)
                box = placeHolderBox->root();
        } else if (curr->isText()) {
            InlineTextBox* textBox = static_cast<RenderText*>(curr)->lastTextBox();
            if (textBox)
                box = textBox->root();
        } else if (curr->isInlineFlow()) {
            InlineRunBox* runBox = static_cast<RenderFlow*>(curr)->lastLineBox();
            if (runBox)
                box = runBox->root();
        }

        if (box)
            break;
    }
    if (!box)
        box = firstLineBox()->root();

    // If we found a line box, then dirty it.
    if (box) {
        RootInlineBox* adjacentBox;
        box->markDirty();

        // dirty the adjacent lines that might be affected
        // NOTE: we dirty the previous line because RootInlineBox objects cache
        // the address of the first object on the next line after a BR, which we may be
        // invalidating here.  For more info, see how RenderBlock::layoutInlineChildren
        // calls setLineBreakInfo with the result of findNextLineBreak.  findNextLineBreak,
        // despite the name, actually returns the first RenderObject after the BR.

        adjacentBox = box->prevRootBox();
        if (adjacentBox)
            adjacentBox->markDirty();
        if (child->isBR() || (curr && curr->isBR())) {
            adjacentBox = box->nextRootBox();
            if (adjacentBox)
                adjacentBox->markDirty();
        }
    }
}

QList< QRectF > RenderFlow::getClientRects()
{
    if (isRenderInline() && isInlineFlow()) {
        QList<QRectF> list;
        for (InlineFlowBox *child = firstLineBox(); child; child = child->nextFlowBox()) {
            QRectF rect(parent()->offsetLeft() + child->xPos(),
                        parent()->offsetTop() + child->yPos(),
                        child->width(), child->height());

            list.append(clientRectToViewport(rect));
        }

        // In case our flow is splitted by blocks
        for (RenderObject *cont = continuation(); cont; cont = cont->continuation()) {
            list.append(cont->getClientRects());
        }

        // Empty Flow, return the Flow itself
        if (list.isEmpty()) {
            return RenderObject::getClientRects();
        }

        return list;
    } else {
        return RenderObject::getClientRects();
    }
}

void RenderFlow::detach()
{
    if (continuation())
        continuation()->detach();
    m_continuation = 0;

    // Make sure to destroy anonymous children first while they are still connected to the rest of the tree, so that they will
    // properly dirty line boxes that they are removed from.  Effects that do :before/:after only on hover could crash otherwise.
    detachRemainingChildren();

    if (!documentBeingDestroyed()) {
        if (m_firstLineBox) {
            // We can't wait for RenderContainer::destroy to clear the selection,
            // because by then we will have nuked the line boxes.
            if (isSelectionBorder())
                canvas()->clearSelection();

            // If line boxes are contained inside a root, that means we're an inline.
            // In that case, we need to remove all the line boxes so that the parent
            // lines aren't pointing to deleted children. If the first line box does
            // not have a parent that means they are either already disconnected or
            // root lines that can just be destroyed without disconnecting.
            if (m_firstLineBox->parent()) {
                for (InlineRunBox* box = m_firstLineBox; box; box = box->nextLineBox())
                    box->remove();
            }

            // If we are an anonymous block, then our line boxes might have children
            // that will outlast this block. In the non-anonymous block case those
            // children will be destroyed by the time we return from this function.
            if (isAnonymousBlock()) {
                for (InlineFlowBox* box = m_firstLineBox; box; box = box->nextFlowBox()) {
                    while (InlineBox* childBox = box->firstChild())
                        childBox->remove();
                }
            }
        } else if (isInline() && parent())
            // empty inlines propagate linebox dirtying to the parent
            parent()->dirtyLinesFromChangedChild(this);
    }

    deleteInlineBoxes();

    RenderBox::detach(); 
}

void RenderFlow::paintLines(PaintInfo& i, int _tx, int _ty)
{
    // Only paint during the foreground/selection phases.
    if (i.phase != PaintActionForeground && i.phase != PaintActionSelection && i.phase != PaintActionOutline)
        return;

    if (!firstLineBox())
        return;

    // We can check the first box and last box and avoid painting if we don't
    // intersect.  This is a quick short-circuit that we can take to avoid walking any lines.
    // FIXME: This check is flawed in two extremely obscure ways.
    // (1) If some line in the middle has a huge overflow, it might actually extend below the last line.
    // (2) The overflow from an inline block on a line is not reported to the line.
    int maxOutlineSize = maximalOutlineSize(i.phase);
    int yPos = firstLineBox()->root()->topOverflow() - maxOutlineSize;
    int h = maxOutlineSize + lastLineBox()->root()->bottomOverflow() - yPos;
    yPos += _ty;
    if ((yPos >= i.r.y() + i.r.height()) || (yPos + h <= i.r.y()))
        return;
    for (InlineFlowBox* curr = firstLineBox(); curr; curr = curr->nextFlowBox()) {
        yPos = curr->root()->topOverflow() - maxOutlineSize;
        h = curr->root()->bottomOverflow() + maxOutlineSize - yPos;
        yPos += _ty;
        if ((yPos < i.r.y() + i.r.height()) && (yPos + h > i.r.y()))
            curr->paint(i, _tx, _ty);
    }

    if (i.phase == PaintActionOutline && i.outlineObjects) {
          foreach (RenderFlow* oo, *i.outlineObjects)
              if (oo->isRenderInline())
                  static_cast<RenderInline*>(oo)->paintOutlines(i.p, _tx, _ty);
          i.outlineObjects->clear();
    }
}


bool RenderFlow::hitTestLines(NodeInfo& i, int x, int y, int tx, int ty, HitTestAction hitTestAction)
{
   (void) hitTestAction;
   /*
    if (hitTestAction != HitTestForeground) // ### port hitTest
        return false;
   */

    if (!firstLineBox())
        return false;

    // We can check the first box and last box and avoid hit testing if we don't
    // contain the point.  This is a quick short-circuit that we can take to avoid walking any lines.
    // FIXME: This check is flawed in two extremely obscure ways.
    // (1) If some line in the middle has a huge overflow, it might actually extend below the last line.
    // (2) The overflow from an inline block on a line is not reported to the line.
    if ((y >= ty + lastLineBox()->root()->bottomOverflow()) || (y < ty + firstLineBox()->root()->topOverflow()))
        return false;

    // See if our root lines contain the point.  If so, then we hit test
    // them further.  Note that boxes can easily overlap, so we can't make any assumptions
    // based off positions of our first line box or our last line box.
    for (InlineFlowBox* curr = lastLineBox(); curr; curr = curr->prevFlowBox()) {
        if (y >= ty + curr->root()->topOverflow() && y < ty + curr->root()->bottomOverflow()) {
            bool inside = curr->nodeAtPoint(i, x, y, tx, ty);
            if (inside) {
                setInnerNode(i);
                return true;
            }
        }
    }

    return false;
}


void RenderFlow::repaint(Priority prior)
{
    if (isInlineFlow()) {
        // Find our leftmost position.
        int left = 0;
        // root inline box not reliably availabe during relayout
        int top = firstLineBox() ? (
                needsLayout() ? firstLineBox()->xPos() : firstLineBox()->root()->topOverflow()
            ) : 0;
        for (InlineRunBox* curr = firstLineBox(); curr; curr = curr->nextLineBox())
            if (curr == firstLineBox() || curr->xPos() < left)
                left = curr->xPos();

        // Now invalidate a rectangle.
        int ow = style() ? style()->outlineSize() : 0;

        // We need to add in the relative position offsets of any inlines (including us) up to our
        // containing block.
        RenderBlock* cb = containingBlock();
        for (RenderObject* inlineFlow = this; inlineFlow && inlineFlow->isInlineFlow() && inlineFlow != cb;
             inlineFlow = inlineFlow->parent()) {
             if (inlineFlow->style() && inlineFlow->style()->position() == PRELATIVE && inlineFlow->layer()) {
                KHTMLAssert(inlineFlow->isBox());
                static_cast<RenderBox*>(inlineFlow)->relativePositionOffset(left, top);
             }
        }

        RootInlineBox *lastRoot = lastLineBox() && !needsLayout() ? lastLineBox()->root() : 0;
        containingBlock()->repaintRectangle(-ow+left, -ow+top,
                                            width()+ow*2,
					    (lastRoot ? lastRoot->bottomOverflow() - top : height())+ow*2, prior);
    }
    else {
        if (firstLineBox() && firstLineBox()->topOverflow() < 0) {
            int ow = style() ? style()->outlineSize() : 0;
            repaintRectangle(-ow, -ow+firstLineBox()->topOverflow(),
                             effectiveWidth()+ow*2, effectiveHeight()+ow*2, prior);
        }
        else
            return RenderBox::repaint(prior);
    }
}

int
RenderFlow::lowestPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int bottom = includeSelf && m_width > 0 ? m_height : 0;
    if (!includeOverflowInterior && hasOverflowClip())
        return bottom;

    // FIXME: Come up with a way to use the layer tree to avoid visiting all the kids.
    // For now, we have to descend into all the children, since we may have a huge abs div inside
    // a tiny rel div buried somewhere deep in our child tree.  In this case we have to get to
    // the abs div.
    for (RenderObject *c = firstChild(); c; c = c->nextSibling()) {
        if (!c->isFloatingOrPositioned() && !c->isText() && !c->isInlineFlow()) {
            int lp = c->yPos() + c->lowestPosition(false);
            bottom = qMax(bottom, lp);
        }
    }

    if (includeSelf && isRelPositioned()) {
        int x = 0;
        relativePositionOffset(x, bottom);
    }

    return bottom;
}

int RenderFlow::rightmostPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int right = includeSelf && m_height > 0 ? m_width : 0;
    if (!includeOverflowInterior && hasOverflowClip())
        return right;

    // FIXME: Come up with a way to use the layer tree to avoid visiting all the kids.
    // For now, we have to descend into all the children, since we may have a huge abs div inside
    // a tiny rel div buried somewhere deep in our child tree.  In this case we have to get to
    // the abs div.
    for (RenderObject *c = firstChild(); c; c = c->nextSibling()) {
        if (!c->isFloatingOrPositioned() && !c->isText() && !c->isInlineFlow()) {
            int rp = c->xPos() + c->rightmostPosition(false);
            right = qMax(right, rp);
        }
    }

    if (includeSelf && isRelPositioned()) {
        int y = 0;
        relativePositionOffset(right, y);
    }

    return right;
}

int RenderFlow::leftmostPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int left = includeSelf && m_height > 0 ? 0 : m_width;
    if (!includeOverflowInterior && hasOverflowClip())
        return left;

    // FIXME: Come up with a way to use the layer tree to avoid visiting all the kids.
    // For now, we have to descend into all the children, since we may have a huge abs div inside
    // a tiny rel div buried somewhere deep in our child tree.  In this case we have to get to
    // the abs div.
    for (RenderObject *c = firstChild(); c; c = c->nextSibling()) {
        if (!c->isFloatingOrPositioned() && !c->isText() && !c->isInlineFlow()) {
            int lp = c->xPos() + c->leftmostPosition(false);
            left = qMin(left, lp);
        }
    }

    if (includeSelf && isRelPositioned()) {
        int y = 0;
        relativePositionOffset(left, y);
    }

    return left;
}

int RenderFlow::highestPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int top = RenderBox::highestPosition(includeOverflowInterior, includeSelf);
    if (!includeOverflowInterior && hasOverflowClip())
        return top;

    // FIXME: Come up with a way to use the layer tree to avoid visiting all the kids.
    // For now, we have to descend into all the children, since we may have a huge abs div inside
    // a tiny rel div buried somewhere deep in our child tree.  In this case we have to get to
    // the abs div.
    for (RenderObject *c = firstChild(); c; c = c->nextSibling()) {
        if (!c->isFloatingOrPositioned() && !c->isText() && !c->isInlineFlow()) {
            int hp = c->yPos() + c->highestPosition(false);
            top = qMin(top, hp);
        }
    }

    if (includeSelf && isRelPositioned()) {
        int x = 0;
        relativePositionOffset(x, top);
    }

    return top;
}
