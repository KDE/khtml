/**
* This file is part of the html renderer for KDE.
 *
 * Copyright (C) 2003-2007 Apple Computer, Inc.
 *           (C) 2006-2007 Germain Garand (germain@ebooksfrance.org)
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

#include "render_line.h"

#include <QDebug>
#include <assert.h>
#include <QPainter>

#include "render_flow.h"
#include "render_text.h"
#include "render_table.h"
#include "render_inline.h"
#include "render_block.h"
#include "render_arena.h"
#include <xml/dom_nodeimpl.h>
#include <xml/dom_docimpl.h>
#include <html/html_formimpl.h>
#include <khtmlview.h>

using namespace DOM;
using namespace khtml;

#ifndef NDEBUG
static bool inInlineBoxDetach;
#endif

class khtml::EllipsisBox : public InlineBox
{
public:
    EllipsisBox(RenderObject* obj, const DOM::DOMString& ellipsisStr, InlineFlowBox* p,
                int w, int y, int h, int b, bool firstLine, InlineBox* markupBox)
    :InlineBox(obj), m_str(ellipsisStr) {
        m_parent = p;
        m_width = w;
        m_y = y;
        m_height = h;
        m_baseline = b;
        m_firstLine = firstLine;
        m_constructed = true;
        m_markupBox = markupBox;
    }

    void paint(RenderObject::PaintInfo& i, int _tx, int _ty);
    bool nodeAtPoint(RenderObject::NodeInfo& info, int _x, int _y, int _tx, int _ty);

private:
    DOM::DOMString m_str;
    InlineBox* m_markupBox;
};

void InlineBox::remove()
{
    if (m_parent) m_parent->removeFromLine(this);
}

void InlineBox::detach(RenderArena* renderArena, bool noRemove)
{
    if (!noRemove) remove();

#ifndef NDEBUG
    inInlineBoxDetach = true;
#endif
    delete this;
#ifndef NDEBUG
    inInlineBoxDetach = false;
#endif

    // Recover the size left there for us by operator delete and free the memory.
    renderArena->free(*(size_t *)this, this);
}

void* InlineBox::operator new(size_t sz, RenderArena* renderArena) throw()
{
    return renderArena->allocate(sz);
}

void InlineBox::operator delete(void* ptr, size_t sz)
{
    assert(inInlineBoxDetach);
#ifdef KHTML_USE_ARENA_ALLOCATOR
    // Stash size where detach can find it.
    *(size_t *)ptr = sz;
#endif
}

static bool needsOutlinePhaseRepaint(RenderObject* o, RenderObject::PaintInfo& i, int tx, int ty) {
    if (o->style()->outlineWidth() <= 0)
        return false;
    QRect r(tx+o->xPos(),ty+o->yPos(),o->width(),o->height());
    if (r.intersects(i.r))
        return false;
    r.adjust(-o->style()->outlineSize(),
                -o->style()->outlineSize(),
                 o->style()->outlineSize(),
                 o->style()->outlineSize());
    if (!r.intersects(i.r))
        return false;
    return true;
}

void InlineBox::paint(RenderObject::PaintInfo& i, int tx, int ty)
{
    if ( i.phase == PaintActionOutline && !needsOutlinePhaseRepaint(object(), i, tx, ty) )
        return;

    // Paint all phases of replaced elements atomically, as though the replaced element established its
    // own stacking context.  (See Appendix E.2, section 6.4 on inline block/table elements in the CSS2.1
    // specification.)
    bool paintSelectionOnly = i.phase == PaintActionSelection;
    RenderObject::PaintInfo info(i.p, i.r, paintSelectionOnly ? i.phase : PaintActionElementBackground);
    object()->paint(info, tx, ty);
    if (!paintSelectionOnly) {
        info.phase = PaintActionChildBackgrounds;
        object()->paint(info, tx, ty);
        info.phase = PaintActionFloat;
        object()->paint(info, tx, ty);
        info.phase = PaintActionForeground;
        object()->paint(info, tx, ty);
        info.phase = PaintActionOutline;
        object()->paint(info, tx, ty);
    }
}

bool InlineBox::nodeAtPoint(RenderObject::NodeInfo& i, int x, int y, int tx, int ty)
{
    // Hit test all phases of replaced elements atomically, as though the replaced element established its
    // own stacking context.  (See Appendix E.2, section 6.4 on inline block/table elements in the CSS2.1
    // specification.)
    bool inside = false;
    return object()->nodeAtPoint(i, x, y, tx, ty, HitTestAll, inside); // ### port hitTest
}

long InlineBox::caretMinOffset() const
{
    return 0;
}

long InlineBox::caretMaxOffset() const
{
    return 1;
}

unsigned long InlineBox::caretMaxRenderedOffset() const
{
    return 1;
}

RootInlineBox* InlineBox::root()
{
    if (m_parent)
        return m_parent->root();
    assert( isRootInlineBox() );
    return static_cast<RootInlineBox*>(this);
}

InlineFlowBox::~InlineFlowBox()
{
}

void InlineFlowBox::extractLine()
{
    if (!m_extracted)
        static_cast<RenderFlow*>(m_object)->extractLineBox(this);
    for (InlineBox* child = firstChild(); child; child = child->nextOnLine())
        child->extractLine();
}

void InlineFlowBox::attachLine()
{
    if (m_extracted)
        static_cast<RenderFlow*>(m_object)->attachLineBox(this);
    for (InlineBox* child = firstChild(); child; child = child->nextOnLine())
        child->attachLine();
}

void InlineFlowBox::deleteLine(RenderArena* arena)
{
    InlineBox* child = firstChild();
    InlineBox* next = 0;
    while (child) {
        assert(this == child->parent());
        next = child->nextOnLine();
#ifndef NDEBUG
        child->setParent(0);
#endif
        child->deleteLine(arena);
        child = next;
    }
#ifndef NDEBUG
    m_firstChild = 0;
    m_lastChild = 0;
#endif

    m_object->removeInlineBox(this);
    detach(arena, true /*no remove*/);
}

void InlineFlowBox::removeFromLine(InlineBox *child)
{
    if (!m_dirty)
        dirtyInlineBoxes();

    root()->childRemoved(child);

    if (child == m_firstChild) {
        m_firstChild = child->nextOnLine();
    }
    if (child == m_lastChild) {
        m_lastChild = child->prevOnLine();
    }
    if (child->nextOnLine()) {
        child->nextOnLine()->m_prev = child->prevOnLine();
    }
    if (child->prevOnLine()) {
        child->prevOnLine()->m_next = child->nextOnLine();
    }

    child->setParent(0);
}

void InlineBox::dirtyInlineBoxes()
{
    markDirty();
    for (InlineFlowBox* curr = parent(); curr && !curr->isDirty(); curr = curr->parent())
        curr->markDirty();
}

void InlineBox::deleteLine(RenderArena* arena)
{
    if (!m_extracted && m_object->isBox())
        static_cast<RenderBox*>(m_object)->setPlaceHolderBox(0);
    detach(arena, true /*no remove*/);
}

void InlineBox::extractLine()
{
    m_extracted = true;
    if (m_object->isBox())
        static_cast<RenderBox*>(m_object)->setPlaceHolderBox(0);
}

void InlineBox::attachLine()
{
    m_extracted = false;
    if (m_object->isBox())
        static_cast<RenderBox*>(m_object)->setPlaceHolderBox(this);
}

void InlineBox::adjustPosition(int dx, int dy)
{
    m_x += dx;
    m_y += dy;
    if (m_object->isReplaced() || m_object->isBR())
        m_object->setPos(m_object->xPos() + dx, m_object->yPos() + dy);
}

bool InlineBox::canAccommodateEllipsisBox(bool ltr, int blockEdge, int ellipsisWidth)
{
    // Non-replaced elements can always accommodate an ellipsis.
    if (!m_object || !m_object->isReplaced())
        return true;

    QRect boxRect(m_x, 0, m_width, 10);
    QRect ellipsisRect(ltr ? blockEdge - ellipsisWidth : blockEdge, 0, ellipsisWidth, 10);
    return !(boxRect.intersects(ellipsisRect));
}

int InlineBox::placeEllipsisBox(bool /*ltr*/, int /*blockEdge*/, int /*ellipsisWidth*/, bool&)
{
    // Use -1 to mean "we didn't set the position."
    return -1;
}

bool InlineBox::nextOnLineExists() const
{
    if (!parent())
        return false;

    if (nextOnLine())
        return true;

    return parent()->nextOnLineExists();
}

bool InlineBox::prevOnLineExists() const
{
    if (!parent())
        return false;

    if (prevOnLine())
        return true;

    return parent()->prevOnLineExists();
}

InlineBox* InlineBox::firstLeafChild()
{
    return this;
}

InlineBox* InlineBox::lastLeafChild()
{
    return this;
}

InlineBox* InlineBox::closestLeafChildForXPos(int _x, int _tx)
{
    if (!isInlineFlowBox())
        return this;

    InlineFlowBox *flowBox = static_cast<InlineFlowBox*>(this);
    if (!flowBox->firstChild())
        return this;

    InlineBox *box = flowBox->closestChildForXPos(_x, _tx);
    if (!box)
        return this;

    return box->closestLeafChildForXPos(_x, _tx);
}

#ifdef ENABLE_DUMP
static QString getInlineBoxType(const InlineBox* box)
{
    if (box->isInlineTextBox())
        return "Text";
    if (box->isRootInlineBox())
        return "RootBox";
    if (box->isInlineFlowBox())
        return "FlowBox";
    if (box->isPlaceHolderBox())
        return "PlaceHolderBox";
    return "InlineBox";
}

QString InlineBox::information() const
{
    QString result;
    QTextStream out(&result, QIODevice::WriteOnly);
    out << getInlineBoxType(this) << "(" << (void*)this << ") "
        << "Pos" << "(" << xPos() << "," << yPos() << ") "
        << "Size" << "(" << width() << "," << height() << ") "
        << "Overflow" << "(" << topOverflow() << "," << bottomOverflow() << ") "
        << (object() ? object()->renderName() : "NoRenderObject") << "(" << (void*)object() << ") ";
    if (isInlineTextBox()) {
        const InlineTextBox* textBox = static_cast<const InlineTextBox*>(this);
        out << "Text[" << textBox->renderText()->data().substring(textBox->start(), textBox->len()).string() << "]";
    }
    return result;
}

void InlineBox::printTree(int indent) const
{
    QString temp;
    temp.fill(' ', indent);

    // qDebug() << (temp + information());
    if (isRootInlineBox()) {
        // const RootInlineBox* root = static_cast<const RootInlineBox*>(this);
    }
    if (isInlineTextBox()) {
        //
    }
    if (isInlineFlowBox()) {
        const InlineFlowBox* flowBox = static_cast<const InlineFlowBox*>(this);
        for (InlineBox* box = flowBox->firstChild(); box; box = box->nextOnLine())
            box->printTree(indent + 2);
    }
}
#endif

int InlineFlowBox::marginLeft() const
{
    if (!includeLeftEdge())
        return 0;

    RenderStyle* cstyle = object()->style();
    Length margin = cstyle->marginLeft();
    if (!margin.isAuto())
        return (margin.isFixed() ? margin.value() : object()->marginLeft());
    return 0;
}

int InlineFlowBox::marginRight() const
{
    if (!includeRightEdge())
        return 0;

    RenderStyle* cstyle = object()->style();
    Length margin = cstyle->marginRight();
    if (!margin.isAuto())
        return (margin.isFixed() ? margin.value() : object()->marginRight());
    return 0;
}

int InlineFlowBox::marginBorderPaddingLeft() const
{
    return marginLeft() + borderLeft() + paddingLeft();
}

int InlineFlowBox::marginBorderPaddingRight() const
{
    return marginRight() + borderRight() + paddingRight();
}

int InlineFlowBox::getFlowSpacingWidth() const
{
    int totWidth = marginBorderPaddingLeft() + marginBorderPaddingRight();
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->isInlineFlowBox())
            totWidth += static_cast<InlineFlowBox*>(curr)->getFlowSpacingWidth();
    }
    return totWidth;
}

bool InlineFlowBox::nextOnLineExists()
{
    if (!parent())
        return false;

    if (nextOnLine())
        return true;

    return parent()->nextOnLineExists();
}

bool InlineFlowBox::prevOnLineExists()
{
    if (!parent())
        return false;

    if (prevOnLine())
        return true;

    return parent()->prevOnLineExists();
}

bool InlineFlowBox::onEndChain(RenderObject* endObject)
{
    if (!endObject)
        return false;

    if (endObject == object())
        return true;

    RenderObject* curr = endObject;
    RenderObject* parent = curr->parent();
    while (parent && !parent->isRenderBlock()) {
        if (parent->lastChild() != curr || parent == object())
            return false;

        curr = parent;
        parent = curr->parent();
    }

    return true;
}

void InlineFlowBox::determineSpacingForFlowBoxes(bool lastLine, RenderObject* endObject)
{
    // All boxes start off open.  They will not apply any margins/border/padding on
    // any side.
    bool includeLeftEdge = false;
    bool includeRightEdge = false;

    RenderFlow* flow = static_cast<RenderFlow*>(object());

    // The root inline box never has borders/margins/padding.
    if (parent()) {
        bool ltr = flow->style()->direction() == LTR;

        // Check to see if all initial lines are unconstructed.  If so, then
        // we know the inline began on this line.
        if (!flow->firstLineBox()->isConstructed() && !object()->isInlineContinuation()) {
            if (ltr && flow->firstLineBox() == this)
                includeLeftEdge = true;
            else if (!ltr && flow->lastLineBox() == this)
                includeRightEdge = true;
        }

        // In order to determine if the inline ends on this line, we check three things:
        // (1) If we are the last line and we don't have a continuation(), then we can
        // close up.
        // (2) If the last line box for the flow has an object following it on the line (ltr,
        // reverse for rtl), then the inline has closed.
        // (3) The line may end on the inline.  If we are the last child (climbing up
        // the end object's chain), then we just closed as well.
        if (!flow->lastLineBox()->isConstructed()) {
            if (ltr) {
                if (!nextLineBox() &&
                    ((lastLine && !object()->continuation()) || nextOnLineExists()
                     || onEndChain(endObject)))
                    includeRightEdge = true;
            }
            else {
                if ((!prevLineBox() || prevLineBox()->isConstructed()) &&
                    ((lastLine && !object()->continuation()) ||
                     prevOnLineExists() || onEndChain(endObject)))
                    includeLeftEdge = true;
            }

        }
    }

    setEdges(includeLeftEdge, includeRightEdge);

    // Recur into our children.
    for (InlineBox* currChild = firstChild(); currChild; currChild = currChild->nextOnLine()) {
        if (currChild->isInlineFlowBox()) {
            InlineFlowBox* currFlow = static_cast<InlineFlowBox*>(currChild);
            currFlow->determineSpacingForFlowBoxes(lastLine, endObject);
        }
    }
}

int InlineFlowBox::placeBoxesHorizontally(int x)
{
    // Set our x position.
    setXPos(x);

    int startX = x;
    x += borderLeft() + paddingLeft();

    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->object()->isText()) {
            InlineTextBox* text = static_cast<InlineTextBox*>(curr);
            text->setXPos(x);
            x += curr->width();
        }
        else {
            if (curr->object()->isPositioned()) {
                if (curr->object()->parent()->style()->direction() == LTR)
                    curr->setXPos(x);
                else {
                    // Our offset that we cache needs to be from the edge of the right border box and
                    // not the left border box.  We have to subtract |x| from the width of the block
                    // (which can be obtained by walking up to the root line box).
                    InlineBox* root = this;
                    while (!root->isRootInlineBox())
                        root = root->parent();
                    curr->setXPos(root->object()->width()-x);
                }
                continue; // The positioned object has no effect on the width.
            }
            if (curr->object()->isInlineFlow()) {
                InlineFlowBox* flow = static_cast<InlineFlowBox*>(curr);
                x += flow->marginLeft();
                x = flow->placeBoxesHorizontally(x);
                x += flow->marginRight();
            }
            else {
                x += curr->object()->marginLeft();
                curr->setXPos(x);
                x += curr->width() + curr->object()->marginRight();
            }
        }
    }

    x += borderRight() + paddingRight();
    setWidth(x-startX);
    return x;
}

void InlineFlowBox::verticallyAlignBoxes(int& heightOfBlock)
{
    int maxPositionTop = 0;
    int maxPositionBottom = 0;
    int maxAscent = 0;
    int maxDescent = 0;

    // Figure out if we're in strict mode.
    RenderObject* curr = object();
    while (curr && !curr->element())
        curr = curr->container();
    bool strictMode = (curr && curr->document()->inStrictMode());

    computeLogicalBoxHeights(maxPositionTop, maxPositionBottom, maxAscent, maxDescent, strictMode);

    if (maxAscent + maxDescent < qMax(maxPositionTop, maxPositionBottom))
        adjustMaxAscentAndDescent(maxAscent, maxDescent, maxPositionTop, maxPositionBottom);

    int maxHeight = maxAscent + maxDescent;
    int topPosition = heightOfBlock;
    int bottomPosition = heightOfBlock;
    placeBoxesVertically(heightOfBlock, maxHeight, maxAscent, strictMode, topPosition, bottomPosition);

    setOverflowPositions(topPosition, bottomPosition);

    // Shrink boxes with no text children in quirks and almost strict mode.
    if (!strictMode)
        shrinkBoxesWithNoTextChildren(topPosition, bottomPosition);

    heightOfBlock += maxHeight;
}

void InlineFlowBox::adjustMaxAscentAndDescent(int& maxAscent, int& maxDescent,
                                              int maxPositionTop, int maxPositionBottom)
{
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        // The computed lineheight needs to be extended for the
        // positioned elements
        // see khtmltests/rendering/html_align.html

        if (curr->object()->isPositioned())
            continue; // Positioned placeholders don't affect calculations.
        if (curr->yPos() == PositionTop || curr->yPos() == PositionBottom) {
            if (curr->yPos() == PositionTop) {
                if (maxAscent + maxDescent < curr->height())
                    maxDescent = curr->height() - maxAscent;
            }
            else {
                if (maxAscent + maxDescent < curr->height())
                    maxAscent = curr->height() - maxDescent;
            }

            if ( maxAscent + maxDescent >= qMax( maxPositionTop, maxPositionBottom ) )
                break;
        }

        if (curr->isInlineFlowBox())
            static_cast<InlineFlowBox*>(curr)->adjustMaxAscentAndDescent(maxAscent, maxDescent, maxPositionTop, maxPositionBottom);
    }
}

void InlineFlowBox::computeLogicalBoxHeights(int& maxPositionTop, int& maxPositionBottom,
                                             int& maxAscent, int& maxDescent, bool strictMode)
{
    if (isRootInlineBox()) {
        // Examine our root box.
        setHeight(object()->lineHeight(m_firstLine));
        bool isTableCell = object()->isTableCell();
        if (isTableCell) {
            RenderTableCell* tableCell = static_cast<RenderTableCell*>(object());
            setBaseline(tableCell->RenderBlock::baselinePosition(m_firstLine));
        }
        else
            setBaseline(object()->baselinePosition(m_firstLine));
        if (hasTextChildren() || strictMode) {
            int ascent = baseline();
            int descent = height() - ascent;
            if (maxAscent < ascent)
                maxAscent = ascent;
            if (maxDescent < descent)
                maxDescent = descent;
        }
    }

    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->object()->isPositioned())
            continue; // Positioned placeholders don't affect calculations.

        curr->setHeight(curr->object()->lineHeight(m_firstLine));
        curr->setBaseline(curr->object()->baselinePosition(m_firstLine));
        curr->setYPos(curr->object()->verticalPositionHint(m_firstLine));
        if (curr->yPos() == PositionTop) {
            if (maxPositionTop < curr->height())
                maxPositionTop = curr->height();
        }
        else if (curr->yPos() == PositionBottom) {
            if (maxPositionBottom < curr->height())
                maxPositionBottom = curr->height();
        }
        else if (curr->hasTextChildren() || strictMode) {
            int ascent = curr->baseline() - curr->yPos();
            int descent = curr->height() - ascent;
            if (maxAscent < ascent)
                maxAscent = ascent;
            if (maxDescent < descent)
                maxDescent = descent;
        }

        if (curr->isInlineFlowBox())
            static_cast<InlineFlowBox*>(curr)->computeLogicalBoxHeights(maxPositionTop, maxPositionBottom, maxAscent, maxDescent, strictMode);
    }
}

void InlineFlowBox::placeBoxesVertically(int y, int maxHeight, int maxAscent, bool strictMode,
                                         int& topPosition, int& bottomPosition)
{
    if (isRootInlineBox()) {
        setYPos(y + maxAscent - baseline());// Place our root box.
        // CSS2: 10.8.1 - line-height on the block level element specifies the *minimum*
        // height of the generated line box
        if (hasTextChildren() && maxHeight < object()->lineHeight(m_firstLine))
            maxHeight = object()->lineHeight(m_firstLine);
    }

    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->object()->isPositioned())
            continue; // Positioned placeholders don't affect calculations.

        // Adjust boxes to use their real box y/height and not the logical height (as dictated by
        // line-height).
        if (curr->isInlineFlowBox())
            static_cast<InlineFlowBox*>(curr)->placeBoxesVertically(y, maxHeight, maxAscent, strictMode,
                                                                    topPosition, bottomPosition);

        bool childAffectsTopBottomPos = true;

        if (curr->yPos() == PositionTop)
            curr->setYPos(y);
        else if (curr->yPos() == PositionBottom)
            curr->setYPos(y + maxHeight - curr->height());
        else {
            if (!strictMode && !curr->hasTextDescendant())
                childAffectsTopBottomPos = false;
            curr->setYPos(curr->yPos() + y + maxAscent - curr->baseline());
        }
        int newY = curr->yPos();
        int newHeight = curr->height();
        int newBaseline = curr->baseline();
        int overflowTop = 0;
        int overflowBottom = 0;
        if (curr->isInlineTextBox() || curr->isInlineFlowBox()) {
            const QFontMetrics &fm = curr->object()->fontMetrics( m_firstLine );
#ifdef APPLE_CHANGES
            newBaseline = fm.ascent();
            newY += curr->baseline() - newBaseline;
            newHeight = newBaseline+fm.descent();
#else
            // only adjust if the leading delta is superior to the font's natural leading
            if ( qAbs(fm.ascent() - curr->baseline()) > fm.leading()/2 ) {
                int ascent = fm.ascent()+fm.leading()/2;
                newBaseline = ascent;
                newY += curr->baseline() - newBaseline;
                newHeight = fm.lineSpacing();
            }
#endif
            for (ShadowData* shadow = curr->object()->style()->textShadow(); shadow; shadow = shadow->next) {
                overflowTop = qMin(overflowTop, shadow->y - shadow->blur);
                overflowBottom = qMax(overflowBottom, shadow->y + shadow->blur);
            }
            if (curr->isInlineFlowBox()) {
                newHeight += curr->object()->borderTop() + curr->object()->paddingTop() +
                            curr->object()->borderBottom() + curr->object()->paddingBottom();
                newY -= curr->object()->borderTop() + curr->object()->paddingTop();
                newBaseline += curr->object()->borderTop() + curr->object()->paddingTop();
            }
        } else {
            newY += curr->object()->marginTop();
            newHeight = curr->height() - (curr->object()->marginTop() + curr->object()->marginBottom());
            overflowTop = curr->object()->overflowTop();
            overflowBottom = curr->object()->overflowHeight() - newHeight;
       }
        curr->setYPos(newY);
        curr->setHeight(newHeight);
        curr->setBaseline(newBaseline);

        if (childAffectsTopBottomPos) {
            topPosition = qMin(topPosition, newY + overflowTop);
            bottomPosition = qMax(bottomPosition, newY + newHeight + overflowBottom);
        }
    }

    if (isRootInlineBox()) {
        const QFontMetrics &fm = object()->fontMetrics( m_firstLine );
#ifdef APPLE_CHANGES
        setHeight(fm.ascent()+fm.descent());
        setYPos(yPos() + baseline() - fm.ascent());
        setBaseline(fm.ascent());
#else
        if ( qAbs(fm.ascent() - baseline()) > fm.leading()/2 ) {
            int ascent = fm.ascent()+fm.leading()/2;
            setHeight(fm.lineSpacing());
            setYPos(yPos() + baseline() - ascent);
            setBaseline(ascent);
        }
#endif
        if (hasTextDescendant() || strictMode) {
            if (yPos() < topPosition)
                topPosition = yPos();
            if (yPos() + height() > bottomPosition)
                bottomPosition = yPos() + height();
        }
    }
}

void InlineFlowBox::shrinkBoxesWithNoTextChildren(int topPos, int bottomPos)
{
    // First shrink our kids.
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (curr->object()->isPositioned())
            continue; // Positioned placeholders don't affect calculations.

        if (curr->isInlineFlowBox())
            static_cast<InlineFlowBox*>(curr)->shrinkBoxesWithNoTextChildren(topPos, bottomPos);
    }

    // See if we have text children. If not, then we need to shrink ourselves to fit on the line.
    if (!hasTextDescendant()) {
        if (yPos() < topPos)
            setYPos(topPos);
        if (yPos() + height() > bottomPos)
            setHeight(bottomPos - yPos());
        if (baseline() > height())
            setBaseline(height());
    }
}

bool InlineFlowBox::nodeAtPoint(RenderObject::NodeInfo& i, int x, int y, int tx, int ty)
{
    // Check children first.
    for (InlineBox* curr = lastChild(); curr; curr = curr->prevOnLine()) {
        if (!curr->object()->layer() && curr->nodeAtPoint(i, x, y, tx, ty)) {
            object()->setInnerNode(i);
            return true;
        }
    }

    // Now check ourselves.
    QRect rect(tx + m_x, ty + m_y, m_width, m_height);
    if (object()->style()->visibility() == VISIBLE && rect.contains(x, y)) {
        object()->setInnerNode(i);
        return true;
    }

    return false;
}


void InlineFlowBox::paint(RenderObject::PaintInfo& i, int tx, int ty)
{
    bool intersectsDamageRect = true;
    int xPos = tx + m_x - object()->maximalOutlineSize(i.phase);
    int w = width() + 2 * object()->maximalOutlineSize(i.phase);
    if ((xPos >= i.r.x() + i.r.width()) || (xPos + w <= i.r.x()))
        intersectsDamageRect = false;

    if (intersectsDamageRect) {
        if (i.phase == PaintActionOutline) {
            // Add ourselves to the paint info struct's list of inlines that need to paint their
            // outlines.
            if (object()->style()->visibility() == VISIBLE && object()->style()->outlineWidth() > 0 &&
                !object()->isInlineContinuation() && !isRootInlineBox()) {
                if (!i.outlineObjects)
                    i.outlineObjects = new QList<RenderFlow*>;
                i.outlineObjects->append(static_cast<RenderFlow*>(object()));
            }
        }
        else {
            // 1. Paint our background and border.
            paintBackgroundAndBorder(i, tx, ty);

            // 2. Paint our underline and overline.
            paintDecorations(i, tx, ty, false);
        }
    }

    // 3. Paint our children.
    for (InlineBox* curr = firstChild(); curr; curr = curr->nextOnLine()) {
        if (!curr->object()->layer())
            curr->paint(i, tx, ty);
    }

    // 4. Paint our strike-through
    if (intersectsDamageRect && i.phase != PaintActionOutline)
        paintDecorations(i, tx, ty, true);
}


void InlineFlowBox::paintAllBackgrounds(QPainter* p, const QColor& c, const BackgroundLayer* bgLayer,
                                     QRect clipr, int _tx, int _ty, int w, int h)
{
    if (!bgLayer)
        return;
    paintAllBackgrounds(p, c, bgLayer->next(), clipr, _tx, _ty, w, h);
    paintOneBackground(p, c, bgLayer, clipr, _tx, _ty, w, h);
}

void InlineFlowBox::paintOneBackground(QPainter* p, const QColor& c, const BackgroundLayer* bgLayer,
                                    QRect clipr, int _tx, int _ty, int w, int h)
{
    CachedImage* bg = bgLayer->backgroundImage();
    bool hasBackgroundImage = bg && bg->isComplete() && 
                              !bg->isTransparent() && !bg->isErrorImage();
    if (!hasBackgroundImage || (!prevLineBox() && !nextLineBox()) || !parent())
        object()->paintBackgroundExtended(p, c, bgLayer, clipr, _tx, _ty, w, h, borderLeft(), borderRight(), paddingLeft(), paddingRight(),
                                          object()->borderTop(), object()->borderBottom(), object()->paddingTop(), object()->paddingBottom());
    else {
        // We have a background image that spans multiple lines.
        // We need to adjust _tx and _ty by the width of all previous lines.
        // Think of background painting on inlines as though you had one long line, a single continuous
        // strip.  Even though that strip has been broken up across multiple lines, you still paint it
        // as though you had one single line.  This means each line has to pick up the background where
        // the previous line left off.
        // FIXME: What the heck do we do with RTL here? The math we're using is obviously not right,
        // but it isn't even clear how this should work at all.
        int xOffsetOnLine = 0;
        for (InlineRunBox* curr = prevLineBox(); curr; curr = curr->prevLineBox())
            xOffsetOnLine += curr->width();
        int startX = _tx - xOffsetOnLine;
        int totalWidth = xOffsetOnLine;
        for (InlineRunBox* curr = this; curr; curr = curr->nextLineBox())
            totalWidth += curr->width();
        p->save();
        p->setClipRect(QRect(_tx, _ty, width(), height()));
        object()->paintBackgroundExtended(p, c, bgLayer, clipr, startX, _ty,
                                          totalWidth, h, borderLeft(), borderRight(), paddingLeft(), paddingRight(),
                                          object()->borderTop(), object()->borderBottom(), object()->paddingTop(), object()->paddingBottom());
        p->restore();
    }
}

void InlineFlowBox::paintBackgroundAndBorder(RenderObject::PaintInfo& pI, int _tx, int _ty)
{
    if (object()->style()->visibility() != VISIBLE || pI.phase != PaintActionForeground)
        return;

    // Move x/y to our coordinates.
    _tx += m_x;
    _ty += m_y;

    int w = width();
    int h = height();

    QRect cr;
    cr.setX(qMax(_tx, pI.r.x()));
    cr.setY(qMax(_ty, pI.r.y()));
    cr.setWidth(_tx<pI.r.x() ? qMax(0,w-(pI.r.x()-_tx)) : qMin(pI.r.width(),w));
    cr.setHeight(_ty<pI.r.y() ? qMax(0,h-(pI.r.y()-_ty)) : qMin(pI.r.height(),h));

    // You can use p::first-line to specify a background. If so, the root line boxes for
    // a line may actually have to paint a background.
    RenderStyle* styleToUse = object()->style(m_firstLine);
    if ((!parent() && m_firstLine && styleToUse != object()->style()) ||
        (parent() && object()->shouldPaintBackgroundOrBorder())) {
        QColor c = styleToUse->backgroundColor();
        paintAllBackgrounds(pI.p, c, styleToUse->backgroundLayers(), cr, _tx, _ty, w, h);

        // ::first-line cannot be used to put borders on a line. Always paint borders with our
        // non-first-line style.
        if (parent() && object()->style()->hasBorder())
            object()->paintBorder(pI.p, _tx, _ty, w, h, object()->style(), includeLeftEdge(), includeRightEdge());
    }
}

static bool shouldDrawDecoration(RenderObject* obj)
{
    bool shouldDraw = false;
    for (RenderObject* curr = obj->firstChild();
         curr; curr = curr->nextSibling()) {
        if (curr->isInlineFlow()) {
            shouldDraw = true;
            break;
        }
        else if (curr->isText() && !curr->isBR() && (curr->style()->preserveWS() ||
                 !curr->element() || !curr->element()->containsOnlyWhitespace())) {
            shouldDraw = true;
            break;
        }
    }
    return shouldDraw;
}

void InlineFlowBox::paintDecorations(RenderObject::PaintInfo& pI, int _tx, int _ty, bool paintedChildren)
{
    // Now paint our text decorations. We only do this if we aren't in quirks mode (i.e., in
    // almost-strict mode or strict mode).
    if (object()->style()->htmlHacks() || object()->style()->visibility() != VISIBLE)
        return;

    _tx += m_x;
    _ty += m_y;
    RenderStyle* styleToUse = object()->style(m_firstLine);
    int deco = parent() ? styleToUse->textDecoration() : styleToUse->textDecorationsInEffect();
    if (deco != TDNONE &&
        ((!paintedChildren && ((deco & UNDERLINE) || (deco & OVERLINE))) || (paintedChildren && (deco & LINE_THROUGH))) &&
        shouldDrawDecoration(object())) {
        // We must have child boxes and have decorations defined.
        _tx += borderLeft() + paddingLeft();
        int w = m_width - (borderLeft() + paddingLeft() + borderRight() + paddingRight());
        if ( !w )
            return;
        const QFontMetrics &fm = object()->fontMetrics( m_firstLine );
        // thick lines on small fonts look ugly
        int thickness = fm.height() > 20 ? fm.lineWidth() : 1;
        QColor underline, overline, linethrough;
        underline = overline = linethrough = styleToUse->color();
        if (!parent())
            object()->getTextDecorationColors(deco, underline, overline, linethrough);

        if (styleToUse->font() != pI.p->font())
            pI.p->setFont(styleToUse->font());

        if (deco & UNDERLINE && !paintedChildren) {
            int underlineOffset = ( fm.height() + m_baseline ) / 2;
            if (underlineOffset <= m_baseline) underlineOffset = m_baseline+1;

            pI.p->fillRect(_tx, _ty + underlineOffset, w, thickness, underline );
        }
        if (deco & OVERLINE && !paintedChildren) {
            pI.p->fillRect(_tx, _ty, w, thickness, overline );
        }
        if (deco & LINE_THROUGH && paintedChildren) {
            pI.p->fillRect(_tx, _ty + 2*m_baseline/3, w, thickness, linethrough );
        }
    }
}

bool InlineFlowBox::canAccommodateEllipsisBox(bool ltr, int blockEdge, int ellipsisWidth)
{
    for (InlineBox *box = firstChild(); box; box = box->nextOnLine()) {
        if (!box->canAccommodateEllipsisBox(ltr, blockEdge, ellipsisWidth))
            return false;
    }
    return true;
}

int InlineFlowBox::placeEllipsisBox(bool ltr, int blockEdge, int ellipsisWidth, bool& foundBox)
{
    int result = -1;
    for (InlineBox *box = firstChild(); box; box = box->nextOnLine()) {
        int currResult = box->placeEllipsisBox(ltr, blockEdge, ellipsisWidth, foundBox);
        if (currResult != -1 && result == -1)
            result = currResult;
    }
    return result;
}

void InlineFlowBox::clearTruncation()
{
    for (InlineBox *box = firstChild(); box; box = box->nextOnLine())
        box->clearTruncation();
}

void EllipsisBox::paint(RenderObject::PaintInfo& i, int _tx, int _ty)
{
    QPainter* p = i.p;
    RenderStyle* _style = m_firstLine ? m_object->style(true) : m_object->style();
    if (_style->font() != p->font())
        p->setFont(_style->font());

    const Font* font = &_style->htmlFont();
    QColor textColor = _style->color();
    if (textColor != p->pen().color())
        p->setPen(textColor);
    /*
    bool setShadow = false;
    if (_style->textShadow()) {
        p->setShadow(_style->textShadow()->x, _style->textShadow()->y,
                     _style->textShadow()->blur, _style->textShadow()->color);
        setShadow = true;
    }*/

    const DOMString& str = m_str.string();
    font->drawText(p, m_x + _tx,
                      m_y + _ty + m_baseline,
                      (str.implementation())->s,
                      str.length(), 0, str.length(),
                      0,
                      Qt::LeftToRight, _style->visuallyOrdered());

    /*
    if (setShadow)
        p->clearShadow();
    */

    if (m_markupBox) {
        // Paint the markup box
        _tx += m_x + m_width - m_markupBox->xPos();
        _ty += m_y + m_baseline - (m_markupBox->yPos() + m_markupBox->baseline());
        m_markupBox->object()->paint(i, _tx, _ty);
    }
}

bool EllipsisBox::nodeAtPoint(RenderObject::NodeInfo& info, int _x, int _y, int _tx, int _ty)
{
    // Hit test the markup box.
    if (m_markupBox) {
        _tx += m_x + m_width - m_markupBox->xPos();
        _ty += m_y + m_baseline - (m_markupBox->yPos() + m_markupBox->baseline());
        if (m_markupBox->nodeAtPoint(info, _x, _y, _tx, _ty)) {
            object()->setInnerNode(info);
            return true;
        }
    }

    QRect rect(_tx + m_x, _ty + m_y, m_width, m_height);
    if (object()->style()->visibility() == VISIBLE && rect.contains(_x, _y)) {
        object()->setInnerNode(info);
        return true;
    }
    return false;
}

void RootInlineBox::detach(RenderArena* arena, bool noRemove)
{
    if (m_lineBreakContext)
        m_lineBreakContext->deref();
    m_lineBreakContext = 0;
    detachEllipsisBox(arena);
    InlineFlowBox::detach(arena, noRemove);

}

void RootInlineBox::detachEllipsisBox(RenderArena* arena)
{
    if (m_ellipsisBox) {
        m_ellipsisBox->detach(arena);
        m_ellipsisBox = 0;
    }
}

void RootInlineBox::clearTruncation()
{
    if (m_ellipsisBox) {
        detachEllipsisBox(m_object->renderArena());
        InlineFlowBox::clearTruncation();
    }
}

bool RootInlineBox::canAccommodateEllipsis(bool ltr, int blockEdge, int lineBoxEdge, int ellipsisWidth)
{
    // First sanity-check the unoverflowed width of the whole line to see if there is sufficient room.
    int delta = ltr ? lineBoxEdge - blockEdge : blockEdge - lineBoxEdge;
    if (width() - delta < ellipsisWidth)
        return false;

    // Next iterate over all the line boxes on the line.  If we find a replaced element that intersects
    // then we refuse to accommodate the ellipsis.  Otherwise we're ok.
    return InlineFlowBox::canAccommodateEllipsisBox(ltr, blockEdge, ellipsisWidth);
}

void RootInlineBox::placeEllipsis(const DOMString& ellipsisStr,  bool ltr, int blockEdge, int ellipsisWidth, InlineBox* markupBox)
{
    // Create an ellipsis box.
    m_ellipsisBox = new (m_object->renderArena()) EllipsisBox(m_object, ellipsisStr, this,
                                                              ellipsisWidth - (markupBox ? markupBox->width() : 0),
                                                              yPos(), height(), baseline(), !prevRootBox(),
                                                              markupBox);

    if (ltr && (xPos() + width() + ellipsisWidth) <= blockEdge) {
        m_ellipsisBox->m_x = xPos() + width();
        return;
    }

    // Now attempt to find the nearest glyph horizontally and place just to the right (or left in RTL)
    // of that glyph.  Mark all of the objects that intersect the ellipsis box as not painting (as being
    // truncated).
    bool foundBox = false;
    m_ellipsisBox->m_x = placeEllipsisBox(ltr, blockEdge, ellipsisWidth, foundBox);
}

int RootInlineBox::placeEllipsisBox(bool ltr, int blockEdge, int ellipsisWidth, bool& foundBox)
{
    int result = InlineFlowBox::placeEllipsisBox(ltr, blockEdge, ellipsisWidth, foundBox);
    if (result == -1)
        result = ltr ? blockEdge - ellipsisWidth : blockEdge;
    return result;
}

void RootInlineBox::paintEllipsisBox(RenderObject::PaintInfo& i, int _tx, int _ty) const
{
    if (m_ellipsisBox)
        m_ellipsisBox->paint(i, _tx, _ty);
}

void RootInlineBox::paint(RenderObject::PaintInfo& i, int tx, int ty)
{
    InlineFlowBox::paint(i, tx, ty);
    paintEllipsisBox(i, tx, ty);
}

bool RootInlineBox::nodeAtPoint(RenderObject::NodeInfo& i, int x, int y, int tx, int ty)
{
    if (m_ellipsisBox && object()->style()->visibility() == VISIBLE) {
        if (m_ellipsisBox->nodeAtPoint(i, x, y, tx, ty)) {
            object()->setInnerNode(i);
            return true;
        }
    }
    return InlineFlowBox::nodeAtPoint(i, x, y, tx, ty);
}


BidiStatus RootInlineBox::lineBreakBidiStatus() const
{
    BidiStatus st;
    st.eor = KDE_CAST_BF_ENUM(QChar::Direction, m_lineBreakBidiStatusEor);
    st.last = KDE_CAST_BF_ENUM(QChar::Direction, m_lineBreakBidiStatusLast);
    st.lastStrong = KDE_CAST_BF_ENUM(QChar::Direction, m_lineBreakBidiStatusLastStrong);
    return st;
}

void RootInlineBox::childRemoved(InlineBox* box)
{
    if (box->object() == m_lineBreakObj)
        setLineBreakInfo(0, 0, BidiStatus(), 0);

    for (RootInlineBox* prev = prevRootBox(); prev && prev->lineBreakObj() == box->object(); prev = prev->prevRootBox()) {
        prev->setLineBreakInfo(0, 0, BidiStatus(), 0);
        prev->markDirty();
    }
}
    
void RootInlineBox::setLineBreakInfo(RenderObject* obj, unsigned breakPos, const BidiStatus& status, BidiContext* context)
{
    m_lineBreakObj = obj;
    m_lineBreakPos = breakPos;
    m_lineBreakBidiStatusEor = status.eor;
    m_lineBreakBidiStatusLastStrong = status.lastStrong;
    m_lineBreakBidiStatusLast = status.last;
    if (m_lineBreakContext)
        m_lineBreakContext->deref();
    m_lineBreakContext = context;
    if (m_lineBreakContext)
        m_lineBreakContext->ref();
}

InlineBox* InlineFlowBox::firstLeafChild()
{
    InlineBox *box = firstChild();
    while (box) {
        InlineBox* next = 0;
        if (!box->isInlineFlowBox())
            break;
        next = static_cast<InlineFlowBox*>(box)->firstChild();
        if (!next)
            break;
        box = next;
    }
    return box;
}

InlineBox* InlineFlowBox::lastLeafChild()
{
    InlineBox *box = lastChild();
    while (box) {
        InlineBox* next = 0;
        if (!box->isInlineFlowBox())
            break;
        next = static_cast<InlineFlowBox*>(box)->lastChild();
        if (!next)
            break;
        box = next;
    }
    return box;
}

InlineBox* InlineFlowBox::closestChildForXPos(int _x, int _tx)
{
    if (_x < _tx + firstChild()->m_x)
        // if the x coordinate is to the left of the first child
        return firstChild();
    else if (_x >= _tx + lastChild()->m_x + lastChild()->m_width)
        // if the x coordinate is to the right of the last child
        return lastChild();
    else
        // look for the closest child;
        // check only the right edges, since the left edge of the first
        // box has already been checked
        for (InlineBox *box = firstChild(); box; box = box->nextOnLine())
            if (_x < _tx + box->m_x + box->m_width)
                return box;

    return 0;
}

