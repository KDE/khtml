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

#include "render_inline.h"

#include "render_arena.h"
#include "render_block.h"
#include "rendering/render_position.h"

#include <xml/dom_docimpl.h>

using namespace khtml;

void RenderInline::setStyle(RenderStyle* _style)
{
    RenderFlow::setStyle(_style);
    setInline(true);

    // Ensure that all of the split inlines pick up the new style. We
    // only do this if we're an inline, since we don't want to propagate
    // a block's style to the other inlines.
    // e.g., <font>foo <h4>goo</h4> moo</font>.  The <font> inlines before
    // and after the block share the same style, but the block doesn't
    // need to pass its style on to anyone else.
    RenderFlow* currCont = continuation();
    while (currCont) {
        if (currCont->isInline()) {
            RenderFlow* nextCont = currCont->continuation();
            currCont->setContinuation(0);
            currCont->setStyle(style());
            currCont->setContinuation(nextCont);
        }
        currCont = currCont->continuation();
    }

    if (attached()) {
        // Update replaced content
        updateReplacedContent();
        // Update pseudos for ::before and ::after
        updatePseudoChildren();
    }
}

// Attach handles initial setStyle that requires parent nodes
void RenderInline::attach()
{
    RenderFlow::attach();

    updateReplacedContent();
    updatePseudoChildren();
}

bool RenderInline::isInlineContinuation() const
{
    return m_isContinuation;
}

void RenderInline::addChildToFlow(RenderObject* newChild, RenderObject* beforeChild)
{
    // Make sure we don't append things after :after-generated content if we have it.
    if (!beforeChild && lastChild() && lastChild()->style()->styleType() == RenderStyle::AFTER)
        beforeChild = lastChild();

    if (!newChild->isInline() && !newChild->isFloatingOrPositioned() )
    {
        // We are placing a block inside an inline. We have to perform a split of this
        // inline into continuations.  This involves creating an anonymous block box to hold
        // |newChild|.  We then make that block box a continuation of this inline.  We take all of
        // the children after |beforeChild| and put them in a clone of this object.

        RenderBlock *newBox = createAnonymousBlock();
        RenderFlow* oldContinuation = continuation();
        setContinuation(newBox);

        splitFlow(beforeChild, newBox, newChild, oldContinuation);
        return;
    }

    RenderBox::addChild(newChild,beforeChild);

    newChild->setNeedsLayoutAndMinMaxRecalc();
}

RenderInline* RenderInline::cloneInline(RenderFlow* src)
{
    RenderInline *o = new (src->renderArena()) RenderInline(src->element());
    o->m_isContinuation = true;
    o->setStyle(src->style());
    return o;
}

void RenderInline::splitInlines(RenderBlock* fromBlock, RenderBlock* toBlock,
                                RenderBlock* middleBlock,
                                RenderObject* beforeChild, RenderFlow* oldCont)
{
    // Create a clone of this inline.
    RenderInline* clone = cloneInline(this);
    clone->setContinuation(oldCont);

    // Now take all of the children from beforeChild to the end and remove
    // then from |this| and place them in the clone.
    RenderObject* o = beforeChild;
    while (o) {
        RenderObject* tmp = o;
        o = tmp->nextSibling();
        clone->addChildToFlow(removeChildNode(tmp), 0);
        tmp->setNeedsLayoutAndMinMaxRecalc();
    }

    // Hook |clone| up as the continuation of the middle block.
    middleBlock->setContinuation(clone);

    // We have been reparented and are now under the fromBlock.  We need
    // to walk up our inline parent chain until we hit the containing block.
    // Once we hit the containing block we're done.
    RenderFlow* curr = static_cast<RenderFlow*>(parent());
    RenderFlow* currChild = this;
    while (curr && curr != fromBlock) {
        // Create a new clone.
        RenderInline* cloneChild = clone;
        clone = cloneInline(curr);

        // Insert our child clone as the first child.
        clone->addChildToFlow(cloneChild, 0);

        // Hook the clone up as a continuation of |curr|.
        RenderFlow* oldCont = curr->continuation();
        curr->setContinuation(clone);
        clone->setContinuation(oldCont);

        // Now we need to take all of the children starting from the first child
        // *after* currChild and append them all to the clone.
        o = currChild->nextSibling();
        while (o) {
            RenderObject* tmp = o;
            o = tmp->nextSibling();
            clone->appendChildNode(curr->removeChildNode(tmp));
            tmp->setNeedsLayoutAndMinMaxRecalc();
        }

        // Keep walking up the chain.
        currChild = curr;
        curr = static_cast<RenderFlow*>(curr->parent());
    }

    // Now we are at the block level. We need to put the clone into the toBlock.
    toBlock->appendChildNode(clone);

    // Now take all the children after currChild and remove them from the fromBlock
    // and put them in the toBlock.
    o = currChild->nextSibling();
    while (o) {
        RenderObject* tmp = o;
        o = tmp->nextSibling();
        toBlock->appendChildNode(fromBlock->removeChildNode(tmp));
    }
}

void RenderInline::splitFlow(RenderObject* beforeChild, RenderBlock* newBlockBox,
                             RenderObject* newChild, RenderFlow* oldCont)
{
    RenderBlock* pre = 0;
    RenderBlock* block = containingBlock();
    bool madeNewBeforeBlock = false;

    // Delete our line boxes before we do the inline split into continuations.
    block->deleteLineBoxTree();

    if (block->isAnonymousBlock()) {
        // We can reuse this block and make it the preBlock of the next continuation.
        pre = block;
        block = block->containingBlock();
    }
    else {
        // No anonymous block available for use.  Make one.
        pre = block->createAnonymousBlock();
        madeNewBeforeBlock = true;
    }

    RenderBlock* post = block->createAnonymousBlock();

    RenderObject* boxFirst = madeNewBeforeBlock ? block->firstChild() : pre->nextSibling();
    if (madeNewBeforeBlock)
        block->insertChildNode(pre, boxFirst);
    block->insertChildNode(newBlockBox, boxFirst);
    block->insertChildNode(post, boxFirst);
    block->setChildrenInline(false);

    if (madeNewBeforeBlock) {
        RenderObject* o = boxFirst;
        while (o)
        {
            RenderObject* no = o;
            o = no->nextSibling();
            pre->appendChildNode(block->removeChildNode(no));
            no->setNeedsLayoutAndMinMaxRecalc();
        }
    }

    splitInlines(pre, post, newBlockBox, beforeChild, oldCont);

    // We already know the newBlockBox isn't going to contain inline kids, so avoid wasting
    // time in makeChildrenNonInline by just setting this explicitly up front.
    newBlockBox->setChildrenInline(false);

    // We don't just call addChild, since it would pass things off to the
    // continuation, so we call addChildToFlow explicitly instead.  We delayed
    // adding the newChild until now so that the |newBlockBox| would be fully
    // connected, thus allowing newChild access to a renderArena should it need
    // to wrap itself in additional boxes (e.g., table construction).
    newBlockBox->addChildToFlow(newChild, 0);

    // XXXdwh is any of this even necessary? I don't think it is.
    pre->close();
    pre->setPos(0, -500000);
    pre->setNeedsLayoutAndMinMaxRecalc();
    newBlockBox->close();
    newBlockBox->setPos(0, -500000);
    newBlockBox->setNeedsLayout(true);
    post->close();
    post->setPos(0, -500000);
    post->setNeedsLayoutAndMinMaxRecalc();

    updatePseudoChildren();

    block->setNeedsLayoutAndMinMaxRecalc();
}

void RenderInline::paint(PaintInfo& i, int _tx, int _ty)
{
    paintLines(i, _tx, _ty);
}

/**
 * Appends the given coordinate-pair to the point-array if it is not
 * equal to the last element.
 * @param pointArray point-array
 * @param pnt point to append
 * @return \c true if \c pnt has actually been appended
 */
inline static bool appendIfNew(QVector<QPoint> &pointArray, const QPoint &pnt)
{
//   if (!pointArray.isEmpty()) qDebug() << "appifnew: " << pointArray.back() << " == " << pnt << ": " << (pointArray.back() == pnt);
//   else qDebug() << "appifnew: " << pnt << " (unconditional)";
    if (!pointArray.isEmpty() && pointArray.back() == pnt) return false;
    pointArray.append(pnt);
    return true;
}

/**
 * Does spike-reduction on the given point-array's stack-top.
 *
 * Spikes are path segments of which one goes forward, and the sucessor
 * goes backward on the predecessor's segment:
 *
 * 2      0      1
 * x------x<-----x
 * (0 is stack-top in point-array)
 *
 * This will be reduced to
 * 1      0
 * x------x
 *
 * Preconditions:
 * - No other spikes exist in the whole point-array except at most
 *   one at the end
 * - No two succeeding points are ever equal
 * - For each two succeeding points either p1.x == p2.x or p1.y == p2.y holds
 *   true
 * - No such spike exists where 2 is situated between 0 and 1.
 *
 * Postcondition:
 * - No spikes exist in the whole point-array
 *
 * If no spike is found, the point-array is left unchanged.
 * @return \c true if an actual reduction was done
 */
inline static bool reduceSpike(QVector<QPoint> &pointArray)
{
    if (pointArray.size() < 3) return false;
    QVector<QPoint>::Iterator it = pointArray.end();
    QPoint p0 = *--it;
    QPoint p1 = *--it;
    QPoint p2 = *--it;

    bool elide = false;

    if ((p0.x() == p1.x() && p1.x() == p2.x()
         && ((p1.y() < p0.y() && p0.y() < p2.y())
             || (p2.y() < p0.y() && p0.y() < p1.y())
             || (p1.y() < p2.y() && p2.y() < p0.y())
             || (p0.y() < p2.y() && p2.y() < p1.y())
             || (elide = p2.y() == p0.y() && p0.y() < p1.y())
             || (elide = p1.y() < p0.y() && p0.y() == p2.y())))
        || (p0.y() == p1.y() && p1.y() == p2.y()
            && ((p1.x() < p0.x() && p0.x() < p2.x())
                || (p2.x() < p0.x() && p0.x() < p1.x())
                || (p1.x() < p2.x() && p2.x() < p0.x())
                || (p0.x() < p2.x() && p2.x() < p1.x())
                || (elide = p2.x() == p0.x() && p0.x() < p1.x())
                || (elide = p1.x() < p0.x() && p0.x() == p2.x()))))
    {
//     qDebug() << "spikered p2" << (elide ? " (elide)" : "") << ": " << p2 << " p1: " << p1 << " p0: " << p0;
        pointArray.pop_back(); pointArray.pop_back();
        if (!elide)
            pointArray.push_back(p0);
        return true;
    }
    return false;
}

/**
 * Reduces segment separators.
 *
 * A segment separator separates a segment into two segments, thus causing
 * two adjacent segment with the same orientation.
 *
 * 2       1     0
 * x-------x---->x
 * (0 means stack-top)
 *
 * Here, 1 is a segment separator. As segment separators not only make
 * the line drawing algorithm inefficient, but also make the spike-reduction
 * fail, they must be eliminated:
 *
 * 1             0
 * x------------>x
 *
 * Preconditions:
 * - No other segment separators exist in the whole point-array except
 *   at most one at the end
 * - No two succeeding points are ever equal
 * - For each two succeeding points either p1.x == p2.x or p1.y == p2.y holds
 *   true
 * - No such spike exists where 2 is situated between 0 and 1.
 *
 * Postcondition:
 * - No segment separators exist in the whole point-array
 *
 * If no segment separator is found at the end of the point-array, it is
 * left unchanged.
 * @return \c true if a segment separator was actually reduced.
 */
inline static bool reduceSegmentSeparator(QVector<QPoint> &pointArray)
{
    if (pointArray.size() < 3) return false;
    QVector<QPoint>::Iterator it = pointArray.end();
    QPoint p0 = *--it;
    QPoint p1 = *--it;
    QPoint p2 = *--it;
//     qDebug() << "checking p2: " << p2 << " p1: " << p1 << " p0: " << p0;

    if ((p0.x() == p1.x() && p1.x() == p2.x()
         && ((p2.y() < p1.y() && p1.y() < p0.y())
             || (p0.y() < p1.y() && p1.y() < p2.y())))
        || (p0.y() == p1.y() && p1.y() == p2.y()
            && ((p2.x() < p1.x() && p1.x() < p0.x())
                || (p0.x() < p1.x() && p1.x() < p2.x()))))
    {
//     qDebug() << "segred p2: " << p2 << " p1: " << p1 << " p0: " << p0;
        pointArray.pop_back(); pointArray.pop_back();
        pointArray.push_back(p0);
        return true;
    }
    return false;
}

/**
 * Appends the given point to the point-array, doing necessary reductions to
 * produce a path without spikes and segment separators.
 */
static void appendPoint(QVector<QPoint> &pointArray, const QPoint &pnt)
{
  if (!appendIfNew(pointArray, pnt)) return;
//   qDebug() << "appendPoint: appended " << pnt;
  reduceSegmentSeparator(pointArray)
  || reduceSpike(pointArray);
}

/**
 * Traverses the horizontal inline boxes and appends the point coordinates to
 * the given array.
 * @param box inline box
 * @param pointArray array collecting coordinates
 * @param bottom \c true, collect bottom coordinates, \c false, collect top
 * 	coordinates.
 * @param limit lower limit that an y-coordinate must at least reach. Note
 *	that limit designates the highest y-coordinate for \c bottom, and
 *	the lowest for !\c bottom.
 */
static void collectHorizontalBoxCoordinates(InlineBox *box,
                                            QVector<QPoint> &pointArray,
                                            bool bottom, int offset, int limit = -500000)
{
//   qDebug() << "collectHorizontalBoxCoordinates: ";
    offset = bottom ? offset:-offset;
    int y = box->yPos() + bottom*box->height() + offset;
    if (limit != -500000 && (bottom ? y < limit : y > limit))
        y = limit;
    int x = box->xPos() + bottom*box->width() + offset;
    QPoint newPnt(x, y);
    // Add intersection point if point-array not empty.
    if (!pointArray.isEmpty()) {
        QPoint lastPnt = pointArray.back();
        QPoint insPnt(newPnt.x(), lastPnt.y());

        if (offset && ((bottom && lastPnt.y() > y) || (!bottom && lastPnt.y() < y))) {
            insPnt.rx() = lastPnt.x();
            insPnt.ry() = y;
        }
//         qDebug() << "left: " << lastPnt << " == " << insPnt << ": " << (insPnt == lastPnt);
        appendPoint(pointArray, insPnt);
    }
    // Insert starting point of box
    appendPoint(pointArray, newPnt);

    newPnt.rx() += (bottom ? -box->width() : box->width()) - 2*offset;

    if (box->isInlineFlowBox()) {
        InlineFlowBox *flowBox = static_cast<InlineFlowBox *>(box);
        for (InlineBox *b = bottom ? flowBox->lastChild() : flowBox->firstChild(); b; b = bottom ? b->prevOnLine() : b->nextOnLine()) {
            // Don't let boxes smaller than this flow box' height influence
            // the vertical position of the outline if they have a different
            // x-coordinate
            int l2;
            if (b->xPos() != box->xPos() && b->xPos() + b->width() != box->xPos() + box->width())
              l2 = y;
            else
              l2 = limit;
            collectHorizontalBoxCoordinates(b, pointArray, bottom, qAbs(offset), l2);
        }

        // Add intersection point if flow box contained any children
        if (flowBox->firstChild()) {
            QPoint lastPnt = pointArray.back();
            QPoint insPnt(lastPnt.x(), newPnt.y());
//             qDebug() << "right: " << lastPnt << " == " << insPnt << ": " << (insPnt == lastPnt);
            appendPoint(pointArray, insPnt);
        }
    }

    // Insert ending point of box
    appendPoint(pointArray, newPnt);

//     qDebug() << "collectHorizontalBoxCoordinates: " << "ende";
}

/**
 * Checks whether the given line box' extents and the following line box'
 * extents are disjount (i. e. do not share the same x-coordinate range).
 * @param line line box
 * @param toBegin \c true, compare with preceding line box, \c false, with
 *	succeeding
 * @return \c true if this and the next box are disjoint
 */
inline static bool lineBoxesDisjoint(InlineRunBox *line, int offset, bool toBegin)
{
  InlineRunBox *next = toBegin ? line->prevLineBox() : line->nextLineBox();
  return !next || next->xPos() + next->width() + 2*offset < line->xPos()
               || next->xPos() > line->xPos() + line->width() + 2*offset;
}

/**
 * Traverses the vertical outer borders of the given render flow's line
 * boxes and appends the point coordinates to the given point array.
 * @param line line box to begin traversal
 * @param pointArray point array
 * @param left \c true, traverse the left vertical coordinates,
 *	\c false, traverse the right vertical coordinates.
 * @param lastline if not 0, returns the pointer to the last line box traversed
 */
static void collectVerticalBoxCoordinates(InlineRunBox *line,
                                          QVector<QPoint> &pointArray,
                                          bool left, int offset, InlineRunBox **lastline = 0)
{
    InlineRunBox *last = 0;
    offset = left ? -offset:offset;
    for (InlineRunBox* curr = line; curr && !last; curr = left ? curr->prevLineBox() : curr->nextLineBox()) {
        InlineBox *root = curr;

        bool isLast = lineBoxesDisjoint(curr, qAbs(offset), left);
        if (isLast) last = curr;

        if (root != line && !isLast)
            while (root->parent()) root = root->parent();
        QPoint newPnt(curr->xPos() + !left*curr->width() + offset,
                      (left ? root->topOverflow() : root->bottomOverflow()) + offset);
        if (!pointArray.isEmpty()) {
            QPoint lastPnt = pointArray.back();
            if (newPnt.x()>lastPnt.x() && !left)
                pointArray.back().setY( qMin(lastPnt.y(), root->topOverflow()-offset) );
            else if (newPnt.x()<lastPnt.x() && left)
                pointArray.back().setY( qMax(lastPnt.y(), root->bottomOverflow()+offset) );
            QPoint insPnt(newPnt.x(), pointArray.back().y());
//         qDebug() << "left: " << lastPnt << " == " << insPnt << ": " << (insPnt == lastPnt);
            appendPoint(pointArray, insPnt);
        }
        appendPoint(pointArray, newPnt);
    }
    if (lastline) *lastline = last;
}

/**
 * Links up the end of the given point-array such that the starting point
 * is not a segment separator.
 *
 * To achieve this, improper points are removed from the beginning of
 * the point-array (by changing the array's starting iterator), and
 * proper ones appended to the point-array's back.
 *
 *   X---------------+               X------------------+
 *   ^               |               ^                  |
 *   |               |      ==>      |                  |
 *   +.....       ...+               +.....          ...+
 *
 *   +----->X--------+               +----------------->X
 *   |               |      ==>      |                  |
 *   +.....       ...+               +.....          ...+
 *
 *         ^X
 *         ||
 *   +-----++--------+               +----------------->X
 *   |               |      ==>      |                  |
 *   +.....       ...+               +.....          ...+
 *
 * @param pointArray point-array
 * @return actual begin of point array
 */
static QPoint *linkEndToBegin(QVector<QPoint> &pointArray)
{
    uint index = 0;
    // ### BUG: outlines with zero width aren't treated correctly
    // this is not the right fix
    if (pointArray.size() < 3) return pointArray.data();

    // if first and last points match, ignore the last one.
    bool linkup = false; QPoint linkupPnt;
    if (pointArray.front() == pointArray.back()) {
        linkupPnt = pointArray.back();
        pointArray.pop_back();
        linkup = true;
    }

    const QPoint *it = pointArray.data() + index;
    QPoint pfirst = *it;
    QPoint pnext = *++it;
    QPoint plast = pointArray.back();
//     qDebug() << "linkcheck plast: " << plast << " pfirst: " << pfirst << " pnext: " << pnext;

    if ((plast.x() == pfirst.x() && pfirst.x() == pnext.x())
        || (plast.y() == pfirst.y() && pfirst.y() == pnext.y())) {

        ++index;
        appendPoint(pointArray, pfirst); // ### do we really need this point?
        appendPoint(pointArray, pnext);
        // ended up at a segment separator? move one point forward
        if (plast == pnext) {
            ++index;
            appendPoint(pointArray, *++it);
        }
    } else if (linkup)
      pointArray.push_back(linkupPnt);
    return pointArray.data() + index;
}

// assumes clock-wise orientation
static RenderObject::BorderSide borderSide( const QPoint &first,
                                            const QPoint &second )
{
    if (second.x() > first.x())
        return RenderObject::BSTop;
    else if (second.x() < first.x())
        return RenderObject::BSBottom;
    else if (second.y() > first.y())
        return RenderObject::BSRight;
    else // second.y() < first.y()
        return RenderObject::BSLeft;
}

void RenderInline::paintOutlines(QPainter *p, int _tx, int _ty)
{
    if (style()->outlineWidth() == 0 || style()->outlineStyle() <= BHIDDEN)
        return;
    int offset = style()->outlineOffset();

    // We may have to draw more than one outline path as they may be
    // disjoint.
    for (InlineRunBox *curr = firstLineBox(); curr; curr = curr->nextLineBox()) {
        QVector<QPoint> path;

        // collect topmost outline
        collectHorizontalBoxCoordinates(curr, path, false, offset);
        // collect right outline
        collectVerticalBoxCoordinates(curr, path, false, offset, &curr);
        // collect bottommost outline
        collectHorizontalBoxCoordinates(curr, path, true, offset);
        // collect left outline
        collectVerticalBoxCoordinates(curr, path, true, offset);

        if (path.size() < 3) continue;

        const QPoint *begin = linkEndToBegin(path);

        // initial borderside and direction values
        QPoint pstart = *begin;
        QPoint pprev = *(path.end() - 2);
        RenderObject::BorderSide bs = borderSide(pprev, pstart);
        QPoint diff = pstart - pprev;
        int direction = diff.x() + diff.y();
        RenderObject::BorderSide endingBS = borderSide(*begin, *(begin + 1));

        // paint the outline
        paintOutlinePath(p, _tx, _ty, begin, path.data() + path.size(),
                         bs, direction, endingBS);
    }
}

template<class T> inline void kSwap(T &a1, T &a2)
{
    T tmp = a2;
    a2 = a1;
    a1 = tmp;
}

enum BSOrientation { BSHorizontal, BSVertical };

/**
 * Returns the orientation of the given border side.
 */
inline BSOrientation bsOrientation(RenderObject::BorderSide bs)
{
    switch (bs) {
    case RenderObject::BSTop:
    case RenderObject::BSBottom:
        return BSHorizontal;
    case RenderObject::BSLeft:
    case RenderObject::BSRight:
        return BSVertical;
    }
    return BSHorizontal;	// make gcc happy (sigh)
}

/**
 * Determines the new border side by evaluating the new direction as determined
 * by the given coordinates, the old border side, and the relative direction.
 *
 * The relative direction specifies whether the old border side meets with the
 * straight given by the coordinates from below/right (negative), or
 * above/left (positive).
 */
inline RenderObject::BorderSide newBorderSide(RenderObject::BorderSide oldBS, int direction, const QPoint &last, const QPoint &cur)
{
    bool below = direction < 0;
    if (last.x() == cur.x()) {	// new segment is vertical
        bool t = oldBS == RenderObject::BSTop;
        bool b = oldBS == RenderObject::BSBottom;
        if ((t || b) && last.y() != cur.y())
            return (cur.y() < last.y()) ^ ((t && below) || (b && !below))
                    ? RenderObject::BSLeft : RenderObject::BSRight;
    } else /*if (last.y() == cur.y())*/ {	// new segment is horizontal
        bool l = oldBS == RenderObject::BSLeft;
        bool r = oldBS == RenderObject::BSRight;
        if ((l || r) && last.x() != cur.x())
            return (cur.x() < last.x()) ^ ((l && below) || (r && !below))
                    ? RenderObject::BSTop : RenderObject::BSBottom;
    }
    return oldBS;			// same direction
}

/**
 * Draws an outline segment between the given two points.
 * @param o render object
 * @param p painter
 * @param tx absolute x-coordinate of containing block
 * @param ty absolute y-coordinate of containing block
 * @param p1 starting point
 * @param p2 end point
 * @param prevBS border side of previous segment
 * @param curBS border side of this segment
 * @param nextBS border side of next segment
 */
static void paintOutlineSegment(RenderObject *o, QPainter *p, int tx, int ty,
                                const QPoint &p1, const QPoint &p2,
                                RenderObject::BorderSide prevBS,
                                RenderObject::BorderSide curBS,
                                RenderObject::BorderSide nextBS)
{
    int ow = o->style()->outlineWidth();
    EBorderStyle os = o->style()->outlineStyle();
    QColor oc = o->style()->outlineColor();

    int x1 = tx + p1.x();
    int y1 = ty + p1.y();
    int x2 = tx + p2.x();
    int y2 = ty + p2.y();
    if (x1 > x2) {
        kSwap(x1, x2);
        if (bsOrientation(curBS) == BSHorizontal) kSwap(prevBS, nextBS);
    }
    if (y1 > y2) {
        kSwap(y1, y2);
        if (bsOrientation(curBS) == BSVertical) kSwap(prevBS, nextBS);
    }

//     qDebug() << "segment(" << x1 << "," << y1 << ") - (" << x2 << "," << y2 << ")";
/*    p->setPen(Qt::gray);
    p->drawLine(x1,y1,x2,y2);*/
    switch (curBS) {
    case RenderObject::BSLeft:
    case RenderObject::BSRight:
/*        p->setPen(QColor("#ffe4dd"));
        p->drawLine(
                      x1 - (curBS == RenderObject::BSLeft ? ow : 0),
                      y1 - (prevBS == RenderObject::BSTop ? ow : 0),
                      x2 + (curBS == RenderObject::BSRight ? ow : 0),
                      y2 + (nextBS == RenderObject::BSBottom ? ow : 0)
                      );*/
        o->drawBorder(p,
                      x1 - (curBS == RenderObject::BSLeft ? ow : 0),
                      y1 - (prevBS == RenderObject::BSTop ? ow : 0),
                      x2 + (curBS == RenderObject::BSRight ? ow : 0),
                      y2 + (nextBS == RenderObject::BSBottom ? ow : 0),
                      curBS, oc, o->style()->color(), os,
                      prevBS == RenderObject::BSTop ? ow
                              : prevBS == RenderObject::BSBottom ? -ow : 0,
                      nextBS == RenderObject::BSTop ? -ow
                              : nextBS == RenderObject::BSBottom ? ow : 0,
                      true);
        break;
    case RenderObject::BSBottom:
    case RenderObject::BSTop:
//       qDebug() << "BSTop/BSBottom: prevBS " << prevBS << " curBS " << curBS << " nextBS " << nextBS;
        o->drawBorder(p,
                      x1 - (prevBS == RenderObject::BSLeft ? ow : 0),
                      y1 - (curBS == RenderObject::BSTop ? ow : 0),
                      x2 + (nextBS == RenderObject::BSRight ? ow : 0),
                      y2 + (curBS == RenderObject::BSBottom ? ow : 0),
                      curBS, oc, o->style()->color(), os,
                      prevBS == RenderObject::BSLeft ? ow
                              : prevBS == RenderObject::BSRight ? -ow : 0,
                      nextBS == RenderObject::BSLeft ? -ow
                              : nextBS == RenderObject::BSRight ? ow : 0,
                      true);
        break;
    }
}

void RenderInline::paintOutlinePath(QPainter *p, int tx, int ty, const QPoint *begin, const QPoint *end, BorderSide bs, int direction, BorderSide endingBS)
{
    int ow = style()->outlineWidth();
    if (ow == 0 || m_isContinuation) // Continuations get painted by the original inline.
        return;

    QPoint last = *begin;
    BorderSide lastBS = bs;
    Q_ASSERT(begin != end);
    ++begin;

//     qDebug() << "last: " << last;

    bs = newBorderSide(bs, direction, last, *begin);
//     qDebug() << "newBorderSide: " << lastBS << " " << direction << "d " << last << " - " << *begin << " => " << bs;

    for (const QPoint *it = begin; it != end; ++it) {
        QPoint cur = *it;
//         qDebug() << "cur: " << cur;
        BorderSide nextBS;
        if (it + 1 != end) {
            QPoint diff = cur - last;
            direction = diff.x() + diff.y();
            nextBS = newBorderSide(bs, direction, cur, *(it + 1));
//             qDebug() << "newBorderSide*: " << bs << " " << direction << "d " << cur << " - " << *(it + 1) << " => " << nextBS;
        } else
            nextBS = endingBS;

        Q_ASSERT(bsOrientation(bs) != bsOrientation(nextBS));
        paintOutlineSegment(this, p, tx, ty, last, cur,
                            lastBS, bs, nextBS);
        lastBS = bs;
        last = cur;
        bs = nextBS;
    }

}

void RenderInline::calcMinMaxWidth()
{
    KHTMLAssert( !minMaxKnown() );

#ifdef DEBUG_LAYOUT
    // qDebug() << renderName() << "(RenderInline)::calcMinMaxWidth() this=" << this;
#endif

    // Irrelevant, since some enclosing block will actually measure us and our children.
    m_minWidth = 0;
    m_maxWidth = 0;

    setMinMaxKnown();
}

short RenderInline::width() const
{
    // Return the width of the minimal left side and the maximal right side.
    short leftSide = 0;
    short rightSide = 0;
    for (InlineRunBox* curr = firstLineBox(); curr; curr = curr->nextLineBox()) {
        if (curr == firstLineBox() || curr->xPos() < leftSide)
            leftSide = curr->xPos();
        if (curr == firstLineBox() || curr->xPos() + curr->width() > rightSide)
            rightSide = curr->xPos() + curr->width();
    }

    return rightSide - leftSide;
}

int RenderInline::height() const
{
    int h = 0;
    if (firstLineBox())
        h = lastLineBox()->yPos() + lastLineBox()->height() - firstLineBox()->yPos();
    return h;
}

int RenderInline::offsetLeft() const
{
    int x = RenderFlow::offsetLeft();
    if (firstLineBox())
        x += firstLineBox()->xPos();
    return x;
}

int RenderInline::offsetTop() const
{
    int y = RenderFlow::offsetTop();
    if (firstLineBox())
        y += firstLineBox()->yPos();
    return y;
}

const char *RenderInline::renderName() const
{
    if (isRelPositioned())
        return "RenderInline (relative positioned)";
    if (isAnonymous())
        return "RenderInline (anonymous)";
    return "RenderInline";
}

bool RenderInline::nodeAtPoint(NodeInfo& info, int _x, int _y, int _tx, int _ty, HitTestAction hitTestAction, bool inside)
{

    // Check our line boxes if we're still not inside.
    if (!inside) {
        // See if we're inside one of our line boxes.
        inside = hitTestLines(info, _x, _y, _tx, _ty, hitTestAction);
    }

    if (inside && element() && style()->visibility() != HIDDEN) {
        if (info.innerNode() && info.innerNode()->renderer() &&
            !info.innerNode()->renderer()->isInline()) {
            // Within the same layer, inlines are ALWAYS fully above blocks.  Change inner node.
            info.setInnerNode(element());

            // Clear everything else.
            info.setInnerNonSharedNode(0);
            info.setURLElement(0);
        }

        if (!info.innerNode())
            info.setInnerNode(element());

        if(!info.innerNonSharedNode())
            info.setInnerNonSharedNode(element());
    }

    return inside;
}

RenderPosition RenderInline::positionForCoordinates(int x, int y)
{
    for (RenderObject *c = continuation(); c; c = c->continuation()) {
        if (c->isInline() || c->firstChild())
            return c->positionForCoordinates(x, y);
    }

    return RenderFlow::positionForCoordinates(x, y);
}

void RenderInline::caretPos(int offset, int flags, int &_x, int &_y, int &width, int &height) const
{
    _x = -1;

    RenderBlock *cb = containingBlock();
    bool rtl = cb->style()->direction() == RTL;
    bool outsideEnd = flags & CFOutsideEnd;
    // I need to explain that: outsideEnd contains a meaningful value if
    // and only if flags & CFOutside is set. If it is not, then randomly
    // either the first or the last line box is returned.
    // This doesn't matter because the only case this can happen is on an
    // empty inline element, whose first and last line boxes are actually
    // the same.
    InlineFlowBox *line = !outsideEnd ^ rtl ? firstLineBox() : lastLineBox();

    if (!line) {		// umpf, handle "gracefully"
        RenderFlow::caretPos(offset, flags, _x, _y, width, height);
        return;
    }

    _x = line->xPos();
    width = 1;		// ### regard CFOverride

    // Place caret outside the border
    if (flags & CFOutside) {
        RenderStyle *s = element() && element()->parent()
			&& element()->parent()->renderer()
			? element()->parent()->renderer()->style()
			: style();
        const QFontMetrics &fm = s->fontMetrics();
        _y = line->yPos() + line->baseline() - fm.ascent();
        height = fm.height();

	if (!outsideEnd ^ rtl) {
	    _x -= line->marginBorderPaddingLeft();
	} else {
	    _x += line->width() + line->marginBorderPaddingRight();
	}

    } else {
        const QFontMetrics &fm = style()->fontMetrics();
        _y = line->yPos() + line->baseline() - fm.ascent();
        height = fm.height();
    }

    int absx, absy;
    if (cb && cb->absolutePosition(absx,absy)) {
        //qDebug() << "absx=" << absx << " absy=" << absy;
        _x += absx;
        _y += absy;
    } else {
        // we don't know our absolute position, and there is no point returning
        // just a relative one
        _x = _y = -1;
    }
}

inline int minXPos(const RenderInline *o)
{
    int retval=6666666;
    if (!o->firstLineBox()) return 0;
    for (InlineRunBox* curr = o->firstLineBox(); curr; curr = curr->nextLineBox())
        retval = qMin( retval, int( curr->m_x ));
    return retval;
}

int RenderInline::inlineXPos() const
{
    return minXPos(this);
}

int RenderInline::inlineYPos() const
{
    return firstLineBox() ? firstLineBox()->yPos() : 0;
}

