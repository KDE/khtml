/*
 * This file is part of the render object implementation for KHTML.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 1999-2003 Antti Koivisto (koivisto@kde.org)
 *           (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 *           (C) 2003-2009 Apple Computer, Inc.
 *           (C) 2004-2009 Germain Garand (germain@ebooksfrance.org)
 *           (C) 2005 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2006 Charles Samuels (charles@kde.org)
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

//#define DEBUG
//#define DEBUG_LAYOUT
//#define BOX_DEBUG
//#define FLOAT_DEBUG
//#define PAGE_DEBUG

#include "render_block.h"

#include <QDebug>
#include <limits.h>
#include "render_text.h"
#include "render_table.h"
#include "render_canvas.h"
#include "render_layer.h"
#include "rendering/render_position.h"

#include <xml/dom_nodeimpl.h>
#include <xml/dom_docimpl.h>
#include <xml/dom_selection.h>
#include <html/html_formimpl.h>

#include <khtmlview.h>
#include <khtml_part.h>

using namespace DOM;

namespace khtml {

// -------------------------------------------------------------------------------------------------------

// Our MarginInfo state used when laying out block children.
RenderBlock::MarginInfo::MarginInfo(RenderBlock* block, int top, int bottom)
{
    // Whether or not we can collapse our own margins with our children.  We don't do this
    // if we had any border/padding (obviously), if we're the root or HTML elements, or if
    // we're positioned, floating, a table cell.
    m_canCollapseWithChildren = !block->isCanvas() && !block->isRoot() && !block->isPositioned() &&
        !block->isFloating() && !block->isTableCell() && !block->hasOverflowClip() && !block->isInlineBlockOrInlineTable();

    m_canCollapseTopWithChildren = m_canCollapseWithChildren && (top == 0) /*&& block->style()->marginTopCollapse() != MSEPARATE */;

    // If any height other than auto is specified in CSS, then we don't collapse our bottom
    // margins with our children's margins.  To do otherwise would be to risk odd visual
    // effects when the children overflow out of the parent block and yet still collapse
    // with it.  We also don't collapse if we have any bottom border/padding.
    m_canCollapseBottomWithChildren = m_canCollapseWithChildren && (bottom == 0) &&
        (block->style()->height().isAuto() && block->style()->height().isZero()) /*&& block->style()->marginBottomCollapse() != MSEPARATE*/;

    m_quirkContainer = block->isTableCell() || block->isBody() /*|| block->style()->marginTopCollapse() == MDISCARD ||
        block->style()->marginBottomCollapse() == MDISCARD*/;

    m_atTopOfBlock = true;
    m_atBottomOfBlock = false;

    m_posMargin = m_canCollapseTopWithChildren ? block->maxTopMargin(true) : 0;
    m_negMargin = m_canCollapseTopWithChildren ? block->maxTopMargin(false) : 0;

    m_selfCollapsingBlockClearedFloat = false;

    m_topQuirk = m_bottomQuirk = m_determinedTopQuirk = false;
}

// -------------------------------------------------------------------------------------------------------

RenderBlock::RenderBlock(DOM::NodeImpl* node)
    : RenderFlow(node)
{
    m_childrenInline = true;
    m_floatingObjects = 0;
    m_positionedObjects = 0;
    m_firstLine = false;
    m_avoidPageBreak = false;
    m_clearStatus = CNONE;
    m_maxTopPosMargin = m_maxTopNegMargin = m_maxBottomPosMargin = m_maxBottomNegMargin = 0;
    m_topMarginQuirk = m_bottomMarginQuirk = false;
    m_overflowHeight = m_overflowWidth = 0;
    m_overflowLeft = m_overflowTop = 0;
}

RenderBlock::~RenderBlock()
{
    if (m_floatingObjects) {
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        while (it.hasNext())
            delete it.next();
    }
    delete m_floatingObjects;
    delete m_positionedObjects;
}

void RenderBlock::setStyle(RenderStyle* _style)
{
    setReplaced(_style->isDisplayReplacedType());

    RenderFlow::setStyle(_style);

    // ### we could save this call when the change only affected
    // non inherited properties
    RenderObject *child = firstChild();
    while (child != 0)
    {
        if (child->isAnonymousBlock())
        {
            RenderStyle* newStyle = new RenderStyle();
            newStyle->inheritFrom(style());
            newStyle->setDisplay(BLOCK);
            child->setStyle(newStyle);
        }
        child = child->nextSibling();
    }

    if (attached()) {
        // Update generated content and ::inside
        updateReplacedContent();
        // Update pseudos for :before and :after
        updatePseudoChildren();
    }

    // handled by close() during parsing
    // ### remove close move upto updatePseudo
    if (!document()->parsing()) {
        updateFirstLetter();
    }
}

// Attach handles initial setStyle that requires parent nodes
void RenderBlock::attach()
{
    RenderFlow::attach();

    updateReplacedContent();
    updatePseudoChildren();
}

static inline bool isFirstLetterPunct(const QChar* c) {
    // CSS2.1/3 definition for ::first-letter doesn't include Pc or Pd.
    if (c->isPunct()) {
        QChar::Category cat = c->category();
        return cat != QChar::Punctuation_Connector &&
               cat != QChar::Punctuation_Dash;
    }
    return false;
}

void RenderBlock::updateFirstLetter()
{
    // Only blocks with inline-children can generate a first-letter
    if (!childrenInline() || !firstChild()) return;

    // Don't recurse
    if (style()->styleType() == RenderStyle::FIRST_LETTER) return;

    // The first-letter style is inheritable.
    RenderStyle *pseudoStyle = style()->getPseudoStyle(RenderStyle::FIRST_LETTER);
    RenderObject *o = this;
    while (o && !pseudoStyle) {
        // ### We should ignore empty preceding siblings
        if (o->parent() && o->parent()->firstChild() == this)
            o = o->parent();
        else
            break;
        pseudoStyle = o->style()->getPseudoStyle(RenderStyle::FIRST_LETTER);
    };

    // FIXME: Currently we don't delete first-letters, this is
    // handled instead in NodeImpl::diff by issuing Detach on first-letter changes.
    if (!pseudoStyle) {
        return;
    }

    // Drill into inlines looking for our first text child.
    RenderObject* firstText = firstChild();
    while (firstText && firstText->needsLayout() && !firstText->isFloating() && !firstText->isRenderBlock() && !firstText->isReplaced() && !firstText->isText())
        // ### We should skip first children with only white-space and punctuation
        firstText = firstText->firstChild();

    if (firstText && firstText->isText() && !firstText->isBR()) {
        RenderObject* firstLetterObject = 0;
        // Find the old first-letter
        if (firstText->parent()->style()->styleType() == RenderStyle::FIRST_LETTER)
            firstLetterObject = firstText->parent();

        // Force inline display (except for floating first-letters)
        pseudoStyle->setDisplay( pseudoStyle->isFloating() ? BLOCK : INLINE);
        pseudoStyle->setPosition( PSTATIC ); // CSS2 says first-letter can't be positioned.

        if (firstLetterObject != 0) {
            firstLetterObject->setStyle( pseudoStyle );
            RenderStyle* newStyle = new RenderStyle();
            newStyle->inheritFrom( pseudoStyle );
            firstText->setStyle( newStyle );
            return;
        }

        RenderText* textObj = static_cast<RenderText*>(firstText);
        RenderObject* firstLetterContainer = firstText->parent();

        firstLetterObject = RenderFlow::createFlow(node(), pseudoStyle, renderArena() );
        firstLetterObject->setIsAnonymous( true );
        firstLetterContainer->addChild(firstLetterObject, firstLetterContainer->firstChild());

        // if this object is the result of a :begin, then the text may have not been
        // generated yet if it is a counter
        if (textObj->recalcMinMax())
            textObj->recalcMinMaxWidths();

        // The original string is going to be either a generated content string or a DOM node's
        // string.  We want the original string before it got transformed in case first-letter has
        // no text-transform or a different text-transform applied to it.
        DOMStringImpl* oldText = textObj->originalString();
        if (!oldText)
            oldText = textObj->string();
        // ### In theory a first-letter can stretch across multiple text objects, if they only contain
        // punctuation and white-space
        if(oldText->l >= 1) {
            oldText->ref();
            // begin: we need skip leading whitespace so that RenderBlock::findNextLineBreak
            // won't think we're continuing from a previous run
            unsigned int begin = 0; // the position that first-letter begins
            unsigned int length = 0; // the position that "the rest" begins
            while ( length < oldText->l && (oldText->s+length)->isSpace() )
                length++;
            begin = length;
            while ( length < oldText->l &&
                    ( isFirstLetterPunct(oldText->s+length) || (oldText->s+length)->isSpace() ))
                length++;
            if ( length < oldText->l &&
                    !( (oldText->s+length)->isSpace() || isFirstLetterPunct(oldText->s+length) ))
                length++;
            while ( length < oldText->l && isFirstLetterPunct(oldText->s+length) )
                length++;

            // we need to generated a remainingText object even if no text is left
            // because it holds the place and style for the old textObj
            RenderTextFragment* remainingText =
                new (renderArena()) RenderTextFragment(textObj->node(), oldText, length, oldText->l-length);
            remainingText->setIsAnonymous( textObj->isAnonymous() );
            remainingText->setStyle(textObj->style());
            if (remainingText->element())
                remainingText->element()->setRenderer(remainingText);

            RenderObject* nextObj = textObj->nextSibling();
            textObj->detach();
            firstLetterContainer->addChild(remainingText, nextObj);

            RenderTextFragment* letter =
                new (renderArena()) RenderTextFragment(remainingText->node(), oldText, begin, length-begin);
            letter->setIsAnonymous( remainingText->isAnonymous() );
            RenderStyle* newStyle = new RenderStyle();
            newStyle->inheritFrom(pseudoStyle);
            letter->setStyle(newStyle);
            firstLetterObject->addChild(letter);
            oldText->deref();

            remainingText->setFirstLetter(letter);
        }
        firstLetterObject->close();
    }
}

void RenderBlock::addChildToFlow(RenderObject* newChild, RenderObject* beforeChild)
{
    // Make sure we don't append things after :after-generated content if we have it.
    if ( !beforeChild && lastChild() && lastChild()->style()->styleType() == RenderStyle::AFTER )
        beforeChild = lastChild();

    bool madeBoxesNonInline = false;

    // If the requested beforeChild is not one of our children, then this is most likely because
    // there is an anonymous block box within this object that contains the beforeChild. So
    // just insert the child into the anonymous block box instead of here. This may also be
    // needed in cases of things like anonymous tables.
    if (beforeChild && beforeChild->parent() != this) {

        KHTMLAssert(beforeChild->parent());

        // In the special case where we are prepending a block-level element before
        // something contained inside an anonymous block, we can just prepend it before
        // the anonymous block.
        if (!newChild->isInline() && beforeChild->parent()->isAnonymousBlock() &&
            beforeChild->parent()->parent() == this &&
            beforeChild->parent()->firstChild() == beforeChild)
            return addChildToFlow(newChild, beforeChild->parent());

        // Otherwise find our kid inside which the beforeChild is, and delegate to it.
        // This may be many levels deep due to anonymous tables, table sections, etc.
        RenderObject* responsible = beforeChild->parent();
        while (responsible->parent() != this)
            responsible = responsible->parent();

        return responsible->addChild(newChild,beforeChild);
    }

    // prevent elements that haven't received a layout yet from getting painted by pushing
    // them far above the top of the page
    if (!newChild->isInline())
        newChild->setPos(newChild->xPos(), -500000);

    // A block has to either have all of its children inline, or all of its children as blocks.
    // So, if our children are currently inline and a block child has to be inserted, we move all our
    // inline children into anonymous block boxes
    if ( m_childrenInline && !newChild->isInline() && !newChild->isFloatingOrPositioned() )
    {
        // This is a block with inline content. Wrap the inline content in anonymous blocks.
        makeChildrenNonInline(beforeChild);
        madeBoxesNonInline = true;

        if (beforeChild && beforeChild->parent() != this) {
            beforeChild = beforeChild->parent();
            KHTMLAssert(beforeChild->isAnonymousBlock());
            KHTMLAssert(beforeChild->parent() == this);
        }
    }
    else if (!m_childrenInline && !newChild->isFloatingOrPositioned())
    {
        // If we're inserting an inline child but all of our children are blocks, then we have to make sure
        // it is put into an anomyous block box. We try to use an existing anonymous box if possible, otherwise
        // a new one is created and inserted into our list of children in the appropriate position.
        if (newChild->isInline()) {
            if (beforeChild) {
                if ( beforeChild->previousSibling() && beforeChild->previousSibling()->isAnonymousBlock() ) {
                    beforeChild->previousSibling()->addChild(newChild);
                    return;
                }
            }
            else {
                if ( m_last && m_last->isAnonymousBlock() ) {
                    m_last->addChild(newChild);
                    return;
                }
            }

            // no suitable existing anonymous box - create a new one
            RenderBlock* newBox = createAnonymousBlock();
            RenderBox::addChild(newBox,beforeChild);
            newBox->addChild(newChild);

            //the above may actually destroy newBox in case an anonymous
            //table got created, and made the anonymous block redundant.
            //so look up what to hide indirectly.
            RenderObject* toHide = newChild;
            while (toHide->parent() != this)
                toHide = toHide->parent();

            toHide->setPos(toHide->xPos(), -500000);
            return;
        }
        else {
            // We are adding another block child... if the current last child is an anonymous box
            // then it needs to be closed.
            // ### get rid of the closing thing altogether this will only work during initial parsing
            if (lastChild() && lastChild()->isAnonymous()) {
                lastChild()->close();
            }
        }
    }

    RenderBox::addChild(newChild,beforeChild);
    // ### care about aligned stuff

    if ( madeBoxesNonInline && isAnonymousBlock() )
        parent()->removeSuperfluousAnonymousBlockChild( this );
    // we might be deleted now
}

static void getInlineRun(RenderObject* start, RenderObject* stop,
                         RenderObject*& inlineRunStart,
                         RenderObject*& inlineRunEnd)
{
    // Beginning at |start| we find the largest contiguous run of inlines that
    // we can.  We denote the run with start and end points, |inlineRunStart|
    // and |inlineRunEnd|.  Note that these two values may be the same if
    // we encounter only one inline.
    //
    // We skip any non-inlines we encounter as long as we haven't found any
    // inlines yet.
    //
    //
    // |stop| indicates a non-inclusive stop point.  Regardless of whether |stop|
    // is inline or not, we will not include it in a run with inlines before it.  It's as though we encountered
    // a non-inline.

    RenderObject * curr = start;
    bool sawInline;
    do {
        while (curr && !(curr->isInline() || curr->isFloatingOrPositioned()))
            curr = curr->nextSibling();

        inlineRunStart = inlineRunEnd = curr;

        if (!curr)
            return; // No more inline children to be found.

        sawInline = curr->isInline();

        curr = curr->nextSibling();
        while (curr && (curr->isInline() || curr->isFloatingOrPositioned()) && (curr != stop)) {
            inlineRunEnd = curr;
            if (curr->isInline())
                sawInline = true;
            curr = curr->nextSibling();
        }
    } while (!sawInline);

}

void RenderBlock::deleteLineBoxTree()
{
    InlineFlowBox* line = m_firstLineBox;
    InlineFlowBox* nextLine;
    while (line) {
        nextLine = line->nextFlowBox();
        line->deleteLine(renderArena());
        line = nextLine;
    }
    m_firstLineBox = m_lastLineBox = 0;
}

short RenderBlock::baselinePosition( bool firstLine ) const
{
    // CSS2.1-10.8.1 "The baseline of an 'inline-block' is the baseline of its last line box 
    // in the normal flow, unless it has either no in-flow line boxes or if its 'overflow' 
    // property has a computed value other than 'visible', in which case the baseline is the bottom margin edge."

    if (isReplaced() && !hasOverflowClip() && !needsLayout()) {
        int res = getBaselineOfLastLineBox();
        if (res != -1) {
            return  res +marginTop();
        }
    }
    return RenderBox::baselinePosition(firstLine);
}

int RenderBlock::getBaselineOfLastLineBox() const
{
    if (!isBlockFlow())
        return -1;

    if (childrenInline()) {
//       if (!firstLineBox() && hasLineIfEmpty())
//            return RenderFlow::baselinePosition(true) + borderTop() + paddingTop();
        if (lastLineBox())
            return lastLineBox()->yPos() + lastLineBox()->baseline();
        return -1;
    }
    else {
//        bool haveNormalFlowChild = false;
        for (RenderObject* curr = lastChild(); curr; curr = curr->previousSibling()) {
            if (!curr->isFloatingOrPositioned() && curr->isBlockFlow()) {
//                haveNormalFlowChild = true;
                int result = static_cast<RenderBlock*>(curr)->getBaselineOfLastLineBox();
                if (result != -1)
                    return curr->yPos() + result; // Translate to our coordinate space.
            }
        }
//        if (!haveNormalFlowChild && isRenderButton()) // hasLineIfEmpty()
//            return RenderFlow::baselinePosition(true) + borderTop() + paddingTop();
    }

    return -1;
}

void RenderBlock::makeChildrenNonInline(RenderObject *insertionPoint)
{
    // makeChildrenNonInline takes a block whose children are *all* inline and it
    // makes sure that inline children are coalesced under anonymous
    // blocks.  If |insertionPoint| is defined, then it represents the insertion point for
    // the new block child that is causing us to have to wrap all the inlines.  This
    // means that we cannot coalesce inlines before |insertionPoint| with inlines following
    // |insertionPoint|, because the new child is going to be inserted in between the inlines,
    // splitting them.
    KHTMLAssert(isReplacedBlock() || !isInline());
    KHTMLAssert(!insertionPoint || insertionPoint->parent() == this);

    deleteLineBoxTree();

    m_childrenInline = false;

    RenderObject *child = firstChild();

    while (child) {
        RenderObject *inlineRunStart, *inlineRunEnd;
        getInlineRun(child, insertionPoint, inlineRunStart, inlineRunEnd);

        if (!inlineRunStart)
            break;

        child = inlineRunEnd->nextSibling();

        RenderBlock* box = createAnonymousBlock();
        insertChildNode(box, inlineRunStart);
        RenderObject* o = inlineRunStart;
        while(o != inlineRunEnd)
        {
            RenderObject* no = o;
            o = no->nextSibling();
            box->appendChildNode(removeChildNode(no));
        }
        box->appendChildNode(removeChildNode(inlineRunEnd));
        box->close();
        box->setPos(box->xPos(), -500000);
    }
}

void RenderBlock::makePageBreakAvoidBlocks()
{
    KHTMLAssert(!childrenInline());
    KHTMLAssert(canvas()->pagedMode());

    RenderObject *breakAfter = firstChild();
    RenderObject *breakBefore = breakAfter ? breakAfter->nextSibling() : 0;

    RenderBlock* pageRun = 0;

    // ### Should follow margin-collapsing rules, skipping self-collapsing blocks
    // and exporting page-breaks from first/last child when collapsing with parent margin.
    while (breakAfter) {
        if (breakAfter->isRenderBlock() && !breakAfter->childrenInline())
            static_cast<RenderBlock*>(breakAfter)->makePageBreakAvoidBlocks();
        EPageBreak pbafter = breakAfter->style()->pageBreakAfter();
        EPageBreak pbbefore = breakBefore ? breakBefore->style()->pageBreakBefore() : PBALWAYS;
        if ((pbafter == PBAVOID && pbbefore == PBAVOID) ||
            (pbafter == PBAVOID && pbbefore == PBAUTO) ||
            (pbafter == PBAUTO && pbbefore == PBAVOID))
        {
            if (!pageRun) {
                pageRun = createAnonymousBlock();
                pageRun->m_avoidPageBreak = true;
                pageRun->setChildrenInline(false);
            }
            pageRun->appendChildNode(removeChildNode(breakAfter));
        } else
        {
            if (pageRun) {
                pageRun->appendChildNode(removeChildNode(breakAfter));
                pageRun->close();
                insertChildNode(pageRun, breakBefore);
                pageRun = 0;
            }
        }
        breakAfter = breakBefore;
        breakBefore = breakBefore ? breakBefore->nextSibling() : 0;
    }

    // recurse into positioned block children as well.
    if (m_positionedObjects) {
        RenderObject* obj;
        QListIterator<RenderObject*> it(*m_positionedObjects);
        while (it.hasNext()) {
            obj = it.next();
            if (obj->isRenderBlock() && !obj->childrenInline()) {
                static_cast<RenderBlock*>(obj)->makePageBreakAvoidBlocks();
            }
        }
    }

    // recurse into floating block children.
    if (m_floatingObjects) {
        FloatingObject* obj;
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        while (it.hasNext()) {
            obj = it.next();
            if (obj->node->isRenderBlock() && !obj->node->childrenInline()) {
                static_cast<RenderBlock*>(obj->node)->makePageBreakAvoidBlocks();
            }
        }
    }
}

void RenderBlock::removeChild(RenderObject *oldChild)
{
    // If this child is a block, and if our previous and next siblings are
    // both anonymous blocks with inline content, then we can go ahead and
    // fold the inline content back together.
    RenderObject* prev = oldChild->previousSibling();
    RenderObject* next = oldChild->nextSibling();
    RenderObject* lc = 0;
    bool mergedBlocks = false;
    bool checkContinuationMerge = false;
    if (!documentBeingDestroyed() && !isInline() && !oldChild->isInline() && !oldChild->continuation()) {
        if (prev && prev->isAnonymousBlock() && prev->childrenInline() &&
            next && next->isAnonymousBlock() && next->childrenInline()) {
            // Take all the children out of the |next| block and put them in
            // the |prev| block.
            RenderObject* o = next->firstChild();
            while (o) {
                RenderObject* no = o;
                o = no->nextSibling();
                prev->appendChildNode(next->removeChildNode(no));
            }

            // Detach the now-empty block.
            static_cast<RenderBlock*>(next)->deleteLineBoxTree();
            next->detach();

            mergedBlocks = true;
        }

        // Check if there are continuations we could merge
        checkContinuationMerge = (mergedBlocks || (!prev && !next)) && continuation() && isAnonymousBlock() && continuation()->isRenderInline() && 
                                previousSibling() && previousSibling()->isAnonymousBlock() && (lc = previousSibling()->lastChild());

        if (checkContinuationMerge) {
            while (lc->lastChild() && lc->continuation())
                lc = lc->lastChild();
            checkContinuationMerge = lc->isRenderInline() && lc->continuation() && (lc->continuation() == this );
        }
        if (checkContinuationMerge) {
            RenderObject* prev = lc->parent();
            RenderObject* cont = continuation()->parent();
            while ( prev && cont ) {
                if (prev == previousSibling() && cont == nextSibling() && cont->isAnonymousBlock())
                    break;
                if (!prev->continuation() || prev->continuation() != cont) {
                    checkContinuationMerge = false;
                    break;
                }
                prev = prev->parent();
                cont = cont->parent();
            }
        }
    } 

    RenderFlow::removeChild(oldChild);

    if (mergedBlocks && prev && !prev->previousSibling() && !prev->nextSibling()) {
        // The remerge has knocked us down to containing only a single anonymous
        // box.  We can go ahead and pull the content right back up into our
        // box.
        RenderBlock* anonBlock = static_cast<RenderBlock*>(prev);
        m_childrenInline = true;
        RenderObject* o = anonBlock->firstChild();
        while (o) {
            RenderObject* no = o;
            o = no->nextSibling();
            appendChildNode(anonBlock->removeChildNode(no));
        }

        // Detach the now-empty block.
        anonBlock->deleteLineBoxTree();
        anonBlock->detach();
    }
    if (checkContinuationMerge && ((!prev && !next) || m_childrenInline)) {
        // |oldChild| was a block that split an inline into continuations.
        // Now that we only have inline content left, we may merge back those continuations
        // into a single inline.
        assert(lc->isRenderInline());
        RenderFlow* prev = static_cast<RenderFlow*>(lc);
        while (RenderFlow* next = prev->continuation()) {
            RenderObject* o = next->firstChild();
            while (o) {
                RenderObject* no = o;
                o = no->nextSibling();
                prev->appendChildNode(next->removeChildNode(no));
            }
            prev->setContinuation( next->continuation() );
            next->setContinuation( 0 );
            if (next != this) {
                next->detach();
                prev = static_cast<RenderFlow*>(prev->parent());
                assert(!prev || prev->isRenderInline() || prev->isRenderBlock());
            }
        }
        assert( nextSibling() && nextSibling()->isAnonymousBlock() );
        if (!nextSibling()->firstChild()) {
            static_cast<RenderBlock*>(nextSibling())->deleteLineBoxTree();
            nextSibling()->detach();
        }
        deleteLineBoxTree();
        detach();
    }
}

bool RenderBlock::isSelfCollapsingBlock() const
{
    // We are not self-collapsing if we
    // (a) have a non-zero height according to layout (an optimization to avoid wasting time)
    // (b) are a table,
    // (c) have border/padding,
    // (d) have a min-height
    if (m_height > 0 ||
        isTable() || (borderBottom() + paddingBottom() + borderTop() + paddingTop()) != 0 ||
        style()->minHeight().isPositive())
        return false;

    bool hasAutoHeight = style()->height().isAuto();
    if (style()->height().isPercent() && !style()->htmlHacks()) {
        hasAutoHeight = true;
        for (RenderBlock* cb = containingBlock(); !cb->isCanvas(); cb = cb->containingBlock()) {
            if (cb->style()->height().isFixed() || cb->isTableCell())
                hasAutoHeight = false;
        }
    }

    // If the height is 0 or auto, then whether or not we are a self-collapsing block depends
    // on whether we have content that is all self-collapsing or not.
    if (hasAutoHeight || ((style()->height().isFixed() || style()->height().isPercent()) && style()->height().isZero())) {
        // If the block has inline children, see if we generated any line boxes.  If we have any
        // line boxes, then we can't be self-collapsing, since we have content.
        if (childrenInline())
            return !firstLineBox();

        // Whether or not we collapse is dependent on whether all our normal flow children
        // are also self-collapsing.
        for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
            if (child->isFloatingOrPositioned())
                continue;
            if (!child->isSelfCollapsingBlock())
                return false;
        }
        return true;
    }
    return false;
}

void RenderBlock::layout()
{
    // Table cells call layoutBlock directly, so don't add any logic here.  Put code into
    // layoutBlock().
    layoutBlock(false);
}

void RenderBlock::layoutBlock(bool relayoutChildren)
{
    if (isInline() && !isReplacedBlock()) {
        setNeedsLayout(false);
        return;
    }
    //    qDebug() << renderName() << " " << this << "::layoutBlock() start";
    //     QTime t;
    //     t.start();
    KHTMLAssert( needsLayout() );
    KHTMLAssert( minMaxKnown() );

    if (canvas()->pagedMode()) relayoutChildren = true;

    if (markedForRepaint()) {
        repaintDuringLayout();
        setMarkedForRepaint(false);
    }

    if (!relayoutChildren && posChildNeedsLayout() && !normalChildNeedsLayout() && !selfNeedsLayout()) {
        // All we have to is lay out our positioned objects.
        layoutPositionedObjects(relayoutChildren);
        if (hasOverflowClip())
            m_layer->checkScrollbarsAfterLayout();
        setNeedsLayout(false);
        return;
    }

    int oldWidth = m_width;

    calcWidth();
    m_overflowWidth = m_width;
    m_overflowLeft = 0;
    if (style()->direction() == LTR )
    {
        int cw=0;
        if (style()->textIndent().isPercent())
            cw = containingBlock()->contentWidth();
        m_overflowLeft = qMin(0, style()->textIndent().minWidth(cw));
    }

    if ( oldWidth != m_width )
        relayoutChildren = true;

    //     qDebug() << floatingObjects << "," << oldWidth << ","
    //                     << m_width << ","<< needsLayout() << "," << isAnonymousBox() << ","
    //                     << isPositioned() << endl;

#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << "(RenderBlock) " << this << " ::layout() width=" << m_width << ", needsLayout=" << needsLayout();
    if(containingBlock() == static_cast<RenderObject *>(this))
        qDebug() << renderName() << ": containingBlock == this";
#endif

    clearFloats();

    int previousHeight = m_height;
    m_height = 0;
    m_overflowHeight = 0;
    m_clearStatus = CNONE;

    // We use four values, maxTopPos, maxPosNeg, maxBottomPos, and maxBottomNeg, to track
    // our current maximal positive and negative margins.  These values are used when we
    // are collapsed with adjacent blocks, so for example, if you have block A and B
    // collapsing together, then you'd take the maximal positive margin from both A and B
    // and subtract it from the maximal negative margin from both A and B to get the
    // true collapsed margin.  This algorithm is recursive, so when we finish layout()
    // our block knows its current maximal positive/negative values.
    //
    // Start out by setting our margin values to our current margins.  Table cells have
    // no margins, so we don't fill in the values for table cells.
    if (!isTableCell()) {
        initMaxMarginValues();

        m_topMarginQuirk = style()->marginTop().isQuirk();
        m_bottomMarginQuirk = style()->marginBottom().isQuirk();

        if (element() && element()->id() == ID_FORM && static_cast<HTMLFormElementImpl*>(element())->isMalformed())
            // See if this form is malformed (i.e., unclosed). If so, don't give the form
            // a bottom margin.
            m_maxBottomPosMargin = m_maxBottomNegMargin = 0;
    }

    if (scrollsOverflow() && m_layer) {
        // For overflow:scroll blocks, ensure we have both scrollbars in place always.
        if (style()->overflowX() == OSCROLL)
            m_layer->showScrollbar( Qt::Horizontal, true );
        if (style()->overflowY() == OSCROLL)
            m_layer->showScrollbar( Qt::Vertical, true );
    }

    setContainsPageBreak(false);

    if (childrenInline())
        layoutInlineChildren( relayoutChildren );
    else
        layoutBlockChildren( relayoutChildren );

    // Expand our intrinsic height to encompass floats.
    int toAdd = borderBottom() + paddingBottom();
    if (m_layer && scrollsOverflowX() && style()->height().isAuto())
        toAdd += m_layer->horizontalScrollbarHeight();
    if ( floatBottom()+toAdd > m_height && (isFloatingOrPositioned() || flowAroundFloats()) )
        m_overflowHeight = m_height = floatBottom() + toAdd;

    int oldHeight = m_height;
    calcHeight();
    if (oldHeight != m_height) {
        m_overflowHeight -= toAdd;
        if (m_layer && scrollsOverflowY()) {
            // overflow-height only includes padding-bottom when it scrolls
            m_overflowHeight += paddingBottom();
        }
        // If the block got expanded in size, then increase our overflowheight to match.
        if (m_overflowHeight < m_height)
            m_overflowHeight = m_height;
    }
    if (previousHeight != m_height)
        relayoutChildren = true;

    if (isTableCell()) {
        // Table cells need to grow to accommodate both overhanging floats and
        // blocks that have overflowed content.
        // Check for an overhanging float first.
        // FIXME: This needs to look at the last flow, not the last child.
        if (lastChild() && lastChild()->hasOverhangingFloats() && !lastChild()->hasOverflowClip()) {
            KHTMLAssert(lastChild()->isRenderBlock());
            m_height = lastChild()->yPos() + static_cast<RenderBlock*>(lastChild())->floatBottom();
            m_height += borderBottom() + paddingBottom();
        }

        if (m_overflowHeight > m_height && !hasOverflowClip())
            m_height = m_overflowHeight + borderBottom() + paddingBottom();
    }

    if( hasOverhangingFloats() && ((isFloating() && style()->height().isAuto()) || isTableCell())) {
        m_height = floatBottom();
        m_height += borderBottom() + paddingBottom();
    }

    if (canvas()->pagedMode()) {
#ifdef PAGE_DEBUG
        qDebug() << renderName() << " Page Bottom: " << pageTopAfter(0);
        qDebug() << renderName() << " Bottom: " << m_height;
#endif
        bool needsPageBreak = false;
        int xpage = crossesPageBreak(0, m_height);
        if (xpage) {
            needsPageBreak = true;
#ifdef PAGE_DEBUG
            qDebug() << renderName() << " crosses to page " << xpage;
#endif
        }
        if (needsPageBreak && !containsPageBreak()) {
            setNeedsPageClear(true);
#ifdef PAGE_DEBUG
            qDebug() << renderName() << " marked for page-clear";
#endif
        }
    }

    layoutPositionedObjects( relayoutChildren );

    // Always ensure our overflow width/height are at least as large as our width/height.
    m_overflowWidth = qMax(m_overflowWidth, (int)m_width);
    m_overflowHeight = qMax(m_overflowHeight, m_height);

    // Update our scrollbars if we're overflow:auto/scroll now that we know if
    // we overflow or not.
    if (hasOverflowClip() && m_layer)
        m_layer->checkScrollbarsAfterLayout();

    setNeedsLayout(false);
}

void RenderBlock::adjustPositionedBlock(RenderObject* child, const MarginInfo& marginInfo)
{
    if (child->isBox() && child->hasStaticX()) {
        if (style()->direction() == LTR)
            static_cast<RenderBox*>(child)->setStaticX(borderLeft() + paddingLeft());
        else
            static_cast<RenderBox*>(child)->setStaticX(borderRight() + paddingRight());
    }

    if (child->isBox() && child->hasStaticY()) {
        int y = m_height;
        if (!marginInfo.canCollapseWithTop()) {
            child->calcVerticalMargins();
            int marginTop = child->marginTop();
            int collapsedTopPos = marginInfo.posMargin();
            int collapsedTopNeg = marginInfo.negMargin();
            if (marginTop > 0) {
                if (marginTop > collapsedTopPos)
                    collapsedTopPos = marginTop;
            } else {
                if (-marginTop > collapsedTopNeg)
                    collapsedTopNeg = -marginTop;
            }
            y += (collapsedTopPos - collapsedTopNeg) - marginTop;
        }
        static_cast<RenderBox*>(child)->setStaticY(y);
    }
}

void RenderBlock::adjustFloatingBlock(const MarginInfo& marginInfo)
{
    // The float should be positioned taking into account the bottom margin
    // of the previous flow.  We add that margin into the height, get the
    // float positioned properly, and then subtract the margin out of the
    // height again.  In the case of self-collapsing blocks, we always just
    // use the top margins, since the self-collapsing block collapsed its
    // own bottom margin into its top margin.
    //
    // Note also that the previous flow may collapse its margin into the top of
    // our block.  If this is the case, then we do not add the margin in to our
    // height when computing the position of the float.   This condition can be tested
    // for by simply calling canCollapseWithTop.  See
    // http://www.hixie.ch/tests/adhoc/css/box/block/margin-collapse/046.html for
    // an example of this scenario.
    int marginOffset = marginInfo.canCollapseWithTop() ? 0 : marginInfo.margin();
    m_height += marginOffset;
    positionNewFloats();
    m_height -= marginOffset;
}

RenderObject* RenderBlock::handleSpecialChild(RenderObject* child, const MarginInfo& marginInfo, CompactInfo& compactInfo, bool& handled)
{
    // Handle positioned children first.
    RenderObject* next = handlePositionedChild(child, marginInfo, handled);
    if (handled) return next;

    // Handle floating children next.
    next = handleFloatingChild(child, marginInfo, handled);
    if (handled) return next;

    // See if we have a compact element.  If we do, then try to tuck the compact element into the margin space of the next block.
    next = handleCompactChild(child, compactInfo, marginInfo, handled);
    if (handled) return next;

    // Finally, see if we have a run-in element.
    return handleRunInChild(child, handled);
}

RenderObject* RenderBlock::handlePositionedChild(RenderObject* child, const MarginInfo& marginInfo, bool& handled)
{
    if (child->isPositioned()) {
        handled = true;
        if (!child->inPosObjectList())
            child->containingBlock()->insertPositionedObject(child);
        adjustPositionedBlock(child, marginInfo);
        return child->nextSibling();
    }
    return 0;
}

RenderObject* RenderBlock::handleFloatingChild(RenderObject* child, const MarginInfo& marginInfo, bool& handled)
{
    if (child->isFloating()) {
        handled = true;
        insertFloatingObject(child);
        adjustFloatingBlock(marginInfo);
        return child->nextSibling();
    }
    return 0;
}

static inline bool isAnonymousWhitespace( RenderObject* o ) {
    if (!o->isAnonymous())
        return false;
    RenderObject *fc = o->firstChild();
    return fc && fc == o->lastChild() && fc->isText() && static_cast<RenderText *>(fc)->stringLength() == 1 &&
           static_cast<RenderText *>(fc)->text()[0].unicode() == ' ';
}

RenderObject* RenderBlock::handleCompactChild(RenderObject* child, CompactInfo& compactInfo, const MarginInfo& marginInfo, bool& handled)
{
    if (!child->isCompact())
        return 0;
    // FIXME: We only deal with one compact at a time.  It is unclear what should be
    // done if multiple contiguous compacts are encountered.  For now we assume that
    // compact A followed by another compact B should simply be treated as block A.
    if (!compactInfo.compact() && (child->childrenInline() || child->isReplaced())) {
        // Get the next non-positioned/non-floating RenderBlock.
        RenderObject* next = child->nextSibling();
        RenderObject* curr = next;
        while (curr && (curr->isFloatingOrPositioned() || isAnonymousWhitespace(curr) || curr->isAnonymousBlock()) )
            curr = curr->nextSibling();
        if (curr && curr->isRenderBlock() && !curr->isCompact() && !curr->isRunIn()) {
            curr->calcWidth(); // So that horizontal margins are correct.
            // Need to compute margins for the child as though it is a block.
            child->style()->setDisplay(BLOCK);
            child->calcWidth();
            child->style()->setDisplay(COMPACT);

            int childMargins = child->marginLeft() + child->marginRight();
            int margin = style()->direction() == LTR ? curr->marginLeft() : curr->marginRight();
            if (margin >= (childMargins + child->maxWidth())) {
                // The compact will fit in the margin.
                handled = true;
                compactInfo.set(child, curr);
                child->layoutIfNeeded();
                int off = marginInfo.margin();
                m_height += off + curr->marginTop() < child->marginTop() ?
                            child->marginTop() - curr->marginTop() -off: 0;

                child->setPos(0,0); // This position will be updated to reflect the compact's
                                    // desired position and the line box for the compact will
                                    // pick that position up.
                return next;
            }
        }
    }
    child->style()->setDisplay(BLOCK);
    child->layoutIfNeeded();
    child->style()->setDisplay(COMPACT);
    return 0;
}

void RenderBlock::adjustSizeForCompactIfNeeded(RenderObject* child, CompactInfo& compactInfo)
{
    // if the compact is bigger than the block it was run into
    // then "this" block should take the height of the compact
    if (compactInfo.matches(child)) {
        // We have a compact child to squeeze in.
        RenderObject* compactChild = compactInfo.compact();
        if (compactChild->height() > child->height())
            m_height += compactChild->height() - child->height();
    }
}

void RenderBlock::insertCompactIfNeeded(RenderObject* child, CompactInfo& compactInfo)
{
    if (compactInfo.matches(child)) {
        // We have a compact child to squeeze in.
        RenderObject* compactChild = compactInfo.compact();
        int compactXPos = borderLeft() + paddingLeft() + compactChild->marginLeft();
        if (style()->direction() == RTL) {
            compactChild->calcWidth(); // have to do this because of the capped maxwidth
            compactXPos = width() - borderRight() - paddingRight() -
                compactChild->width() - compactChild->marginRight();
        }

        int compactYPos = child->yPos() + child->borderTop() + child->paddingTop()
                            - compactChild->paddingTop() - compactChild->borderTop();
        int adj = 0;
        KHTMLAssert(child->isRenderBlock());
        InlineRunBox *b = static_cast<RenderBlock*>(child)->firstLineBox();
        InlineRunBox *c = static_cast<RenderBlock*>(compactChild)->firstLineBox();
        if (b && c) {
            // adjust our vertical position
            int vpos = compactChild->getVerticalPosition( true, child );
            if (vpos == PositionBottom)
                adj = b->height() > c->height() ? (b->height() + b->yPos() - c->height() - c->yPos()) : 0;
            else if (vpos == PositionTop)
                adj = b->yPos() - c->yPos();
            else
                adj = vpos;
            compactYPos += adj;
        }
        Length newLineHeight( qMax(compactChild->lineHeight(true)+adj, (int)child->lineHeight(true)), khtml::Fixed);
        child->style()->setLineHeight( newLineHeight );
        child->setNeedsLayout( true, false );
        child->layout();

        compactChild->setPos(compactXPos, compactYPos); // Set the x position.
        compactInfo.clear();
    }
}

RenderObject* RenderBlock::handleRunInChild(RenderObject* child, bool& handled)
{
    if (!child->isRunIn())
        return 0;
    // See if we have a run-in element with inline children.  If the
    // children aren't inline, then just treat the run-in as a normal
    // block.
    if (child->childrenInline() || child->isReplaced()) {
        // Get the next non-positioned/non-floating RenderBlock.
        RenderObject* curr = child->nextSibling();
        while (curr && (curr->isFloatingOrPositioned() || isAnonymousWhitespace(curr) || curr->isAnonymousBlock()) )
            curr = curr->nextSibling();
        if (curr && (curr->isRenderBlock() && curr->childrenInline() && !curr->isCompact() && !curr->isRunIn())) {
            // The block acts like an inline, so just null out its
            // position.
            handled = true;
            child->setInline(true);
            child->setPos(0,0);

            // Remove the child.
            RenderObject* next = child->nextSibling();
            removeChildNode(child);

            // Now insert the child under |curr|.
            curr->insertChildNode(child, curr->firstChild());
            return next;
        }
    }
    return 0;
}

int RenderBlock::collapseMargins(RenderObject* child, MarginInfo& marginInfo, int yPosEstimate)
{
    Q_UNUSED(yPosEstimate);
    // Get our max pos and neg top margins.
    int posTop = child->maxTopMargin(true);
    int negTop = child->maxTopMargin(false);

    // For self-collapsing blocks, collapse our bottom margins into our
    // top to get new posTop and negTop values.
    if (child->isSelfCollapsingBlock()) {
        posTop = qMax(posTop, (int)child->maxBottomMargin(true));
        negTop = qMax(negTop, (int)child->maxBottomMargin(false));
    }

    // See if the top margin is quirky. We only care if this child has
    // margins that will collapse with us.
    bool topQuirk = child->isTopMarginQuirk() /*|| style()->marginTopCollapse() == MDISCARD*/;

    if (marginInfo.canCollapseWithTop()) {
        // This child is collapsing with the top of the
        // block.  If it has larger margin values, then we need to update
        // our own maximal values.
        if (!style()->htmlHacks() || !marginInfo.quirkContainer() || !topQuirk) {
            m_maxTopPosMargin = qMax(posTop, (int)m_maxTopPosMargin);
            m_maxTopNegMargin = qMax(negTop, (int)m_maxTopNegMargin);
        }

        // The minute any of the margins involved isn't a quirk, don't
        // collapse it away, even if the margin is smaller (www.webreference.com
        // has an example of this, a <dt> with 0.8em author-specified inside
        // a <dl> inside a <td>.
        if (!marginInfo.determinedTopQuirk() && !topQuirk && (posTop-negTop)) {
            m_topMarginQuirk = false;
            marginInfo.setDeterminedTopQuirk(true);
        }

        if (!marginInfo.determinedTopQuirk() && topQuirk && marginTop() == 0)
            // We have no top margin and our top child has a quirky margin.
            // We will pick up this quirky margin and pass it through.
            // This deals with the <td><div><p> case.
            // Don't do this for a block that split two inlines though.  You do
            // still apply margins in this case.
            m_topMarginQuirk = true;
    }

    if (marginInfo.quirkContainer() && marginInfo.atTopOfBlock() && (posTop - negTop))
        marginInfo.setTopQuirk(topQuirk);

    int ypos = m_height;
    if (child->isSelfCollapsingBlock()) {
        // This child has no height.  We need to compute our
        // position before we collapse the child's margins together,
        // so that we can get an accurate position for the zero-height block.
        int collapsedTopPos = qMax(marginInfo.posMargin(), (int)child->maxTopMargin(true));
        int collapsedTopNeg = qMax(marginInfo.negMargin(), (int)child->maxTopMargin(false));
        marginInfo.setMargin(collapsedTopPos, collapsedTopNeg);

        // Now collapse the child's margins together, which means examining our
        // bottom margin values as well.
        marginInfo.setPosMarginIfLarger(child->maxBottomMargin(true));
        marginInfo.setNegMarginIfLarger(child->maxBottomMargin(false));

        if (!marginInfo.canCollapseWithTop())
            // We need to make sure that the position of the self-collapsing block
            // is correct, since it could have overflowing content
            // that needs to be positioned correctly (e.g., a block that
            // had a specified height of 0 but that actually had subcontent).
            ypos = m_height + collapsedTopPos - collapsedTopNeg;
    }
    else {
#ifdef APPLE_CHANGES
        if (child->style()->marginTopCollapse() == MSEPARATE) {
            m_height += marginInfo.margin() + child->marginTop();
            ypos = m_height;
        }
        else
#endif
        if (!marginInfo.atTopOfBlock() ||
            (!marginInfo.canCollapseTopWithChildren()
             && (!style()->htmlHacks() || !marginInfo.quirkContainer() || !marginInfo.topQuirk()))) {
            // We're collapsing with a previous sibling's margins and not
            // with the top of the block.
            m_height += qMax(marginInfo.posMargin(), posTop) - qMax(marginInfo.negMargin(), negTop);
            ypos = m_height;
        }

        marginInfo.setPosMargin(child->maxBottomMargin(true));
        marginInfo.setNegMargin(child->maxBottomMargin(false));

        if (marginInfo.margin())
            marginInfo.setBottomQuirk(child->isBottomMarginQuirk() /*|| style()->marginBottomCollapse() == MDISCARD*/);

        marginInfo.setSelfCollapsingBlockClearedFloat(false);
    }
    return ypos;
}

int RenderBlock::clearFloatsIfNeeded(RenderObject* child, MarginInfo& marginInfo, int oldTopPosMargin, int oldTopNegMargin, int yPos)
{
    int heightIncrease = getClearDelta(child, yPos);
    if (heightIncrease) {

        // Increase our height by the amount we had to clear.
        bool selfCollapsing = child->isSelfCollapsingBlock();
        if (!selfCollapsing)
            m_height += heightIncrease;
        else {
            // For self-collapsing blocks that clear, they may end up collapsing
            // into the bottom of the parent block.  We simulate this behavior by
            // setting our positive margin value to compensate for the clear.
            marginInfo.setPosMargin(qMax(0, child->yPos() - m_height));
            marginInfo.setNegMargin(0);
            marginInfo.setSelfCollapsingBlockClearedFloat(true);
        }

        if (marginInfo.canCollapseWithTop()) {
            // We can no longer collapse with the top of the block since a clear
            // occurred.  The empty blocks collapse into the cleared block.
            // FIXME: This isn't quite correct.  Need clarification for what to do
            // if the height the cleared block is offset by is smaller than the
            // margins involved.
            m_maxTopPosMargin = oldTopPosMargin;
            m_maxTopNegMargin = oldTopNegMargin;
            marginInfo.setAtTopOfBlock(false);
        }
/*
        // If our value of clear caused us to be repositioned vertically to be
        // underneath a float, we might have to do another layout to take into account
        // the extra space we now have available.
        if (!selfCollapsing && !child->style()->width().isFixed() && child->usesLineWidth())
            // The child's width is a percentage of the line width.
            // When the child shifts to clear an item, its width can
            // change (because it has more available line width).
            // So go ahead and mark the item as dirty.
            child->setChildNeedsLayout(true);
        if (!child->flowAroundFloats() && child->hasFloats())
            child->markAllDescendantsWithFloatsForLayout();
        child->layoutIfNeeded();
*/
        return yPos + heightIncrease;
    }
    return yPos;
}

bool RenderBlock::canClear(RenderObject *child, PageBreakLevel level)
{
    KHTMLAssert(child->parent() && child->parent() == this);
    KHTMLAssert(canvas()->pagedMode());

    // Positioned elements cannot be moved. Only normal flow and floating.
    if (child->isPositioned() || child->isRelPositioned()) return false;

    switch(level) {
        case PageBreakNormal:
            // check page-break-inside: avoid
            if (!style()->pageBreakInside())
                // we cannot, but can our parent?
                if(!parent()->canClear(this, level)) return false;
        case PageBreakHarder:
            // check page-break-after/before: avoid
            if (m_avoidPageBreak)
                // we cannot, but can our parent?
                if(!parent()->canClear(this, level)) return false;
        case PageBreakForced:
            // child is larger than page-height and is forced to break
            if(child->height() > canvas()->pageHeight()) return false;
            return true;
    }
    assert(false);
    return false;
}

void RenderBlock::clearPageBreak(RenderObject* child, int pageBottom)
{
    KHTMLAssert(child->parent() && child->parent() == this);
    KHTMLAssert(canvas()->pagedMode());

    if (child->yPos() >= pageBottom) return;

    int heightIncrease = 0;

    heightIncrease = pageBottom - child->yPos();

    // ### should never happen, canClear should have been called to detect it.
    if (child->height() > canvas()->pageHeight()) {
        // qDebug() << "### child is too large to clear: " << child->height() << " > " << canvas()->pageHeight();
        return;
    }

    // The child needs to be lowered.  Move the child so that it just clears the break.
    child->setPos(child->xPos(), pageBottom);

#ifdef PAGE_DEBUG
    qDebug() << "Cleared block " << heightIncrease << "px";
#endif

    // Increase our height by the amount we had to clear.
    m_height += heightIncrease;

    // We might have to do another layout to take into account
    // the extra space we now have available.
    if (!child->style()->width().isFixed()  && child->usesLineWidth())
        // The child's width is a percentage of the line width.
        // When the child shifts to clear a page-break, its width can
        // change (because it has more available line width).
        // So go ahead and mark the item as dirty.
        child->setChildNeedsLayout(true);
    if (!child->flowAroundFloats() && child->hasFloats())
        child->markAllDescendantsWithFloatsForLayout();
    if (child->containsPageBreak())
        child->setNeedsLayout(true);
    child->layoutIfNeeded();

    child->setAfterPageBreak(true);
}

int RenderBlock::estimateVerticalPosition(RenderObject* child, const MarginInfo& marginInfo)
{
    // FIXME: We need to eliminate the estimation of vertical position, because
    // when it's wrong we sometimes trigger a pathological relayout if there are
    // intruding floats.
    int yPosEstimate = m_height;
    if (!marginInfo.canCollapseWithTop()) {
        int childMarginTop = child->selfNeedsLayout() ? child->marginTop() : child->collapsedMarginTop();
        yPosEstimate += qMax(marginInfo.margin(), childMarginTop);
    }
    yPosEstimate += getClearDelta(child, yPosEstimate);
    return yPosEstimate;
}

void RenderBlock::determineHorizontalPosition(RenderObject* child)
{
    if (style()->direction() == LTR) {
        int xPos = borderLeft() + paddingLeft();

        if (m_layer && scrollsOverflowY() && m_layer->hasReversedScrollbar())
            xPos += m_layer->verticalScrollbarWidth();

        // Add in our left margin.
        int chPos = xPos + child->marginLeft();

        // Some objects (e.g., tables, horizontal rules, overflow:auto blocks) avoid floats.  They need
        // to shift over as necessary to dodge any floats that might get in the way.
        if (child->flowAroundFloats()) {
            int leftOff = leftOffset(m_height);
            if (style()->textAlign() != KHTML_CENTER && !child->style()->marginLeft().isAuto()) {
                if (child->marginLeft() < 0)
                    leftOff += child->marginLeft();
                chPos = qMax(chPos, leftOff); // Let the float sit in the child's margin if it can fit.
            }
            else if (leftOff != xPos) {
                // The object is shifting right. The object might be centered, so we need to
                // recalculate our horizontal margins. Note that the containing block content
                // width computation will take into account the delta between |leftOff| and |xPos|
                // so that we can just pass the content width in directly to the |calcHorizontalMargins|
                // function.
                static_cast<RenderBox*>(child)->calcHorizontalMargins(child->style()->marginLeft(), child->style()->marginRight(), lineWidth(child->yPos()));
                chPos = leftOff + child->marginLeft();
            }
        }

        child->setPos(chPos, child->yPos());
    } else {
        int xPos = m_width - borderRight() - paddingRight();
        if (m_layer && scrollsOverflowY() && !m_layer->hasReversedScrollbar())
            xPos -= m_layer->verticalScrollbarWidth();
        int chPos = xPos - (child->width() + child->marginRight());
        if (child->flowAroundFloats()) {
            int rightOff = rightOffset(m_height);
            if (style()->textAlign() != KHTML_CENTER && !child->style()->marginRight().isAuto()) {
                if (child->marginRight() < 0)
                    rightOff -= child->marginRight();
                chPos = qMin(chPos, rightOff - child->width()); // Let the float sit in the child's margin if it can fit.
            } else if (rightOff != xPos) {
                // The object is shifting left. The object might be centered, so we need to
                // recalculate our horizontal margins. Note that the containing block content
                // width computation will take into account the delta between |rightOff| and |xPos|
                // so that we can just pass the content width in directly to the |calcHorizontalMargins|
                // function.
                static_cast<RenderBox*>(child)->calcHorizontalMargins(child->style()->marginLeft(), child->style()->marginRight(), lineWidth(child->yPos()));
                chPos = rightOff - child->marginRight() - child->width();
            }
        }
        child->setPos(chPos, child->yPos());
    }
}

void RenderBlock::setCollapsedBottomMargin(const MarginInfo& marginInfo)
{
    if (marginInfo.canCollapseWithBottom() && !marginInfo.canCollapseWithTop()) {
        // Update our max pos/neg bottom margins, since we collapsed our bottom margins
        // with our children.
        m_maxBottomPosMargin = qMax((int)m_maxBottomPosMargin, marginInfo.posMargin());
        m_maxBottomNegMargin = qMax((int)m_maxBottomNegMargin, marginInfo.negMargin());

        if (!marginInfo.bottomQuirk())
            m_bottomMarginQuirk = false;

        if (marginInfo.bottomQuirk() && marginBottom() == 0)
            // We have no bottom margin and our last child has a quirky margin.
            // We will pick up this quirky margin and pass it through.
            // This deals with the <td><div><p> case.
            m_bottomMarginQuirk = true;
    }
}

void RenderBlock::handleBottomOfBlock(int top, int bottom, MarginInfo& marginInfo)
{
    // If our last flow was a self-collapsing block that cleared a float, then we don't
    // collapse it with the bottom of the block.
    if (!marginInfo.selfCollapsingBlockClearedFloat())
        marginInfo.setAtBottomOfBlock(true);

    // If we can't collapse with children then go ahead and add in the bottom margin.
    if (!marginInfo.canCollapseWithBottom() && !marginInfo.canCollapseWithTop()
        && (!style()->htmlHacks() || !marginInfo.quirkContainer() || !marginInfo.bottomQuirk()))
        m_height += marginInfo.margin();

    // Now add in our bottom border/padding.
    m_height += bottom;

    // Negative margins can cause our height to shrink below our minimal height (border/padding).
    // If this happens, ensure that the computed height is increased to the minimal height.
    m_height = qMax(m_height, top + bottom);

    // Always make sure our overflow height is at least our height.
    m_overflowHeight = qMax(m_height, m_overflowHeight);

    // Update our bottom collapsed margin info.
    setCollapsedBottomMargin(marginInfo);
}

void RenderBlock::layoutBlockChildren( bool relayoutChildren )
{
#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << " layoutBlockChildren( " << this <<" ), relayoutChildren="<< relayoutChildren;
#endif

    int top = borderTop() + paddingTop();
    int bottom = borderBottom() + paddingBottom();
    if (m_layer && scrollsOverflowX() && style()->height().isAuto())
        bottom += m_layer->horizontalScrollbarHeight();

    m_height = m_overflowHeight = top;

    // The margin struct caches all our current margin collapsing state.
    // The compact struct caches state when we encounter compacts.
    MarginInfo marginInfo(this, top, bottom);
    CompactInfo compactInfo;

    // Fieldsets need to find their legend and position it inside the border of the object.
    // The legend then gets skipped during normal layout.
    RenderObject* legend = layoutLegend(relayoutChildren);

    PageBreakInfo pageBreakInfo(pageTopAfter(0));

    int previousFloatBottom = 0;
    RenderObject* child = firstChild();
    while( child != 0 )
    {
        if (legend == child) {
            child = child->nextSibling();
            continue; // Skip the legend, since it has already been positioned up in the fieldset's border.
        }

        int oldTopPosMargin = m_maxTopPosMargin;
        int oldTopNegMargin = m_maxTopNegMargin;

        // make sure we relayout children if we need it.
        if ((!child->isPositioned()||child->isPosWithStaticDim()) && (relayoutChildren ||
             (child->isReplaced() && (child->style()->width().isPercent() || child->style()->height().isPercent())) ||
             (child->isRenderBlock() && child->style()->height().isPercent()) ||
             (child->isBody() && child->style()->htmlHacks())))
        {
            child->setChildNeedsLayout(true);
        }

        // Handle the four types of special elements first.  These include positioned content, floating content, compacts and
        // run-ins.  When we encounter these four types of objects, we don't actually lay them out as normal flow blocks.
        bool handled = false;
        RenderObject* next = handleSpecialChild(child, marginInfo, compactInfo, handled);
        if (handled) { child = next; continue; }

        // The child is a normal flow object.  Compute its vertical margins now.
        child->calcVerticalMargins();

#ifdef APPLE_CHANGES /* margin-*-collapse not merged yet */
        // Do not allow a collapse if the margin top collapse style is set to SEPARATE.
        if (child->style()->marginTopCollapse() == MSEPARATE) {
            marginInfo.setAtTopOfBlock(false);
            marginInfo.clearMargin();
        }
#endif

        // Try to guess our correct y position.  In most cases this guess will
        // be correct.  Only if we're wrong (when we compute the real y position)
        // will we have to potentially relayout.
        int yPosEstimate = estimateVerticalPosition(child, marginInfo);
        bool markDescendantsWithFloats = false;
        if (yPosEstimate != child->yPos() && !child->flowAroundFloats() && child->hasFloats())
            markDescendantsWithFloats = true;
        else if (!child->flowAroundFloats() || child->usesLineWidth()) {
            // If an element might be affected by the presence of floats, then always mark it for
            // layout.
            int fb = qMax(previousFloatBottom, floatBottom());
            if (fb > yPosEstimate)
                markDescendantsWithFloats = true;
        }

        if (child->isRenderBlock()) {
            if (markDescendantsWithFloats)
                child->markAllDescendantsWithFloatsForLayout();
            previousFloatBottom = qMax(previousFloatBottom, child->yPos() + static_cast<RenderBlock*>(child)->floatBottom());
        }

        // Go ahead and position the child as though it didn't collapse with the top.
        child->setPos(child->xPos(), yPosEstimate);
        child->layoutIfNeeded();

        // Now determine the correct ypos based on examination of collapsing margin
        // values.
        int yBeforeClear = collapseMargins(child, marginInfo, yPosEstimate);

        // Now check for clear.
        int yAfterClear = clearFloatsIfNeeded(child, marginInfo, oldTopPosMargin, oldTopNegMargin, yBeforeClear);
        
        child->setPos(child->xPos(), yAfterClear);

        // Now we have a final y position.  See if it really does end up being different from our estimate.
        if (yAfterClear != yPosEstimate) {
            if (child->usesLineWidth()) {
                // The child's width depends on the line width.
                // When the child shifts to clear an item, its width can
                // change (because it has more available line width).
                // So go ahead and mark the item as dirty.
                child->setChildNeedsLayout(true, false);
            }

            if (!child->flowAroundFloats() && child->hasFloats())
                child->markAllDescendantsWithFloatsForLayout();

            // Our guess was wrong. Make the child lay itself out again.
            child->layoutIfNeeded();
        }

        // We are no longer at the top of the block if we encounter a non-empty child.
        // This has to be done after checking for clear, so that margins can be reset if a clear occurred.
        if (marginInfo.atTopOfBlock() && !child->isSelfCollapsingBlock())
            marginInfo.setAtTopOfBlock(false);

        // Now place the child in the correct horizontal position
        determineHorizontalPosition(child);

        adjustSizeForCompactIfNeeded(child, compactInfo);
        // Update our height now that the child has been placed in the correct position.
        m_height += child->height();

#ifdef APPLE_CHANGES
        if (child->style()->marginBottomCollapse() == MSEPARATE) {
            m_height += child->marginBottom();
            marginInfo.clearMargin();
        }
#endif

        // Check for page-breaks
        if (canvas()->pagedMode())
            clearChildOfPageBreaks(child, pageBreakInfo, marginInfo);

        if (child->hasOverhangingFloats() && !child->flowAroundFloats()) {
            // need to add the child's floats to our floating objects list, but not in the case where
            // overflow is auto/scroll
            addOverHangingFloats( static_cast<RenderBlock *>(child), -child->xPos(), -child->yPos(), true );
        }

        // See if this child has made our overflow need to grow.
        int effX = child->effectiveXPos();
        int effY = child->effectiveYPos();
        m_overflowWidth = qMax(effX + child->effectiveWidth(), m_overflowWidth);
        m_overflowLeft = qMin(effX, m_overflowLeft);
        m_overflowHeight = qMax(effY + child->effectiveHeight(), m_overflowHeight);
        m_overflowTop = qMin(effY, m_overflowTop);

        // Insert our compact into the block margin if we have one.
        insertCompactIfNeeded(child, compactInfo);

        child = child->nextSibling();
    }

    // The last child had forced page-break-after
    if (pageBreakInfo.forcePageBreak())
        m_height = pageBreakInfo.pageBottom();

    // Now do the handling of the bottom of the block, adding in our bottom border/padding and
    // determining the correct collapsed bottom margin information.
    handleBottomOfBlock(top, bottom, marginInfo);

    setNeedsLayout(false);
}

void RenderBlock::clearChildOfPageBreaks(RenderObject *child, PageBreakInfo &pageBreakInfo, MarginInfo &marginInfo)
{
    (void)marginInfo;
    int childTop = child->yPos();
    int childBottom = child->yPos()+child->height();
#ifdef PAGE_DEBUG
    qDebug() << renderName() << " ChildTop: " << childTop << " ChildBottom: " << childBottom;
#endif

    bool forcePageBreak = pageBreakInfo.forcePageBreak() || child->style()->pageBreakBefore() == PBALWAYS;
#ifdef PAGE_DEBUG
    if (forcePageBreak)
        qDebug() << renderName() << "Forced break required";
#endif

    int xpage = crossesPageBreak(childTop, childBottom);
    if (xpage || forcePageBreak)
    {
        if (!forcePageBreak && child->containsPageBreak() && !child->needsPageClear()) {
#ifdef PAGE_DEBUG
            qDebug() << renderName() << " Child contains page-break to page " << xpage;
#endif
            // ### Actually this assumes floating children are breaking/clearing
            // nicely as well.
            setContainsPageBreak(true);
        }
        else {
            bool doBreak = true;
            // don't break before the first child or when page-break-inside is avoid
            if (!forcePageBreak && (!style()->pageBreakInside() || m_avoidPageBreak || child == firstChild())) {
                if (parent() && parent()->canClear(this, (m_avoidPageBreak) ? PageBreakHarder : PageBreakNormal )) {
#ifdef PAGE_DEBUG
                    qDebug() << renderName() << "Avoid page-break inside";
#endif
                    child->setNeedsPageClear(false);
                    setNeedsPageClear(true);
                    doBreak = false;
                }
#ifdef PAGE_DEBUG
                else
                    qDebug() << renderName() << "Ignoring page-break avoid";
#endif
            }
            if (doBreak) {
#ifdef PAGE_DEBUG
                qDebug() << renderName() << " Clearing child of page-break";
                qDebug() << renderName() << " child top of page " << xpage;
#endif
                clearPageBreak(child, pageBreakInfo.pageBottom());
                child->setNeedsPageClear(false);
                setContainsPageBreak(true);
            }
        }
        pageBreakInfo.setPageBottom(pageBreakInfo.pageBottom() + canvas()->pageHeight());
    }
    else
    if (child->yPos() >= pageBreakInfo.pageBottom()) {
        bool doBreak = true;
#ifdef PAGE_DEBUG
        qDebug() << "Page-break between children";
#endif
        if (!style()->pageBreakInside() || m_avoidPageBreak) {
            if (parent() && parent()->canClear(this, (m_avoidPageBreak) ? PageBreakHarder : PageBreakNormal )) {
#ifdef PAGE_DEBUG
                qDebug() << "Avoid page-break inside";
#endif
                child->setNeedsPageClear(false);
                setNeedsPageClear(true);
                doBreak = false;
            }
#ifdef PAGE_DEBUG
            else
                qDebug() << "Ignoring page-break avoid";
#endif
        }
        if (doBreak) {
            // Break between children
            setContainsPageBreak(true);
            // ### Should collapse top-margin with page-margin
        }
        pageBreakInfo.setPageBottom(pageBreakInfo.pageBottom() + canvas()->pageHeight());
    }

    // Do we need a forced page-break before next child?
    pageBreakInfo.setForcePageBreak(false);
    if (child->style()->pageBreakAfter() == PBALWAYS)
        pageBreakInfo.setForcePageBreak(true);
}

void RenderBlock::layoutPositionedObjects(bool relayoutChildren)
{
    if (m_positionedObjects) {
        for (int i=0; i < m_positionedObjects->size(); i++) { // size() can grow during loop
            RenderObject * const r = m_positionedObjects->at(i);
            if (r->markedForRepaint()) {
                r->repaintDuringLayout();
                r->setMarkedForRepaint(false);
            }
            if (relayoutChildren || (r->isPosWithStaticDim() && r->parent() != this && r->parent()->isBlockFlow())) {
                r->setChildNeedsLayout(true, false);
                r->dirtyFormattingContext(false);
            }
            r->layoutIfNeeded();
        }
    }
}

void RenderBlock::paint(PaintInfo& pI, int _tx, int _ty)
{
    _tx += m_x;
    _ty += m_y;

    // check if we need to do anything at all...
    if (!isRoot() && !isInlineFlow() && !isRelPositioned() && !isPositioned() )
    {
        int h = m_overflowHeight;
        int yPos = _ty;
        if (m_floatingObjects && floatBottom() > h)
            h = floatBottom();

        yPos += overflowTop();

        int os = maximalOutlineSize(pI.phase);
        if( (yPos > pI.r.bottom() + os) || (_ty + h <= pI.r.y() - os))
            return;
    }

    paintObject(pI, _tx, _ty);
}

void RenderBlock::paintObject(PaintInfo& pI, int _tx, int _ty, bool shouldPaintOutline)
{
#ifdef DEBUG_LAYOUT
   qDebug() << renderName() << "(RenderBlock) " << this << " ::paintObject() w/h = (" << width() << "/" << height() << ")";
#endif

    // If we're a repositioned run-in, don't paint background/borders.
    bool inlineFlow = isInlineFlow();

    // 1. paint background, borders etc
    if (!inlineFlow &&
        (pI.phase == PaintActionElementBackground || pI.phase == PaintActionChildBackground ) &&
         shouldPaintBackgroundOrBorder() && style()->visibility() == VISIBLE)
        paintBoxDecorations(pI, _tx, _ty);

    if ( pI.phase == PaintActionElementBackground )
        return;
    if ( pI.phase == PaintActionChildBackgrounds )
        pI.phase = PaintActionChildBackground;

    // 2. paint contents
    int scrolledX = _tx;
    int scrolledY = _ty;
    if (hasOverflowClip() && m_layer)
        m_layer->subtractScrollOffset(scrolledX, scrolledY);

    if (childrenInline())
        paintLines(pI, scrolledX, scrolledY);
    else {
        for(RenderObject *child = firstChild(); child; child = child->nextSibling())
            if(!child->layer() && !child->isFloating())
                child->paint(pI, scrolledX, scrolledY);
    }

    // 3. paint floats.
    if (!inlineFlow && (pI.phase == PaintActionFloat || pI.phase == PaintActionSelection))
        paintFloats(pI, scrolledX, scrolledY, pI.phase == PaintActionSelection);

    // 4. paint outline.
    if (shouldPaintOutline && !inlineFlow && pI.phase == PaintActionOutline &&
        style()->outlineWidth() && style()->visibility() == VISIBLE)
        paintOutline(pI.p, _tx, _ty, width(), height(), style());

    // 5. paint caret.
    /*
        If the caret's node's render object's containing block is this block,
        and the paint action is PaintActionForeground,
        then paint the caret.
    */
    if ((!canvas()->hasSelection() && pI.phase == PaintActionForeground)
         || pI.phase == PaintActionSelection) {
        KHTMLPart *part = document()->part();
        const Selection &s = part->caret();
        NodeImpl *baseNode = s.extent().node();
        RenderObject *renderer = baseNode ? baseNode->renderer() : 0;
        if (renderer && renderer->containingBlock() == this && (part->isCaretMode() || baseNode->isContentEditable())) {
            part->paintCaret(pI.p, pI.r);
            part->paintDragCaret(pI.p, pI.r);
        }
    }

#ifdef BOX_DEBUG
    if ( style() && style()->visibility() == VISIBLE ) {
        if(isAnonymous())
            outlineBox(pI.p, _tx, _ty, "green");
        if(isFloating())
            outlineBox(pI.p, _tx, _ty, "yellow");
        else
            outlineBox(pI.p, _tx, _ty);
    }
#endif
}

QRegion RenderBlock::visibleFloatingRegion(int x, int y) const
{
    if (!m_floatingObjects)
        return QRegion();
    FloatingObject* fo;
    QRegion r;
    QListIterator<FloatingObject*> it(*m_floatingObjects);
    while ( it.hasNext() ) {
        fo = it.next();
        if (!fo->noPaint && !fo->node->layer() && fo->node->style()->visibility() == VISIBLE) {
            const RenderStyle *s = fo->node->style();
            int ow = s->outlineSize();
            if ( s->backgroundImage() || s->backgroundColor().isValid() || s->hasBorder() || fo->node->isReplaced() || ow ) {
                r += QRect(x -ow +fo->left+fo->node->marginLeft(),
                           y -ow +fo->startY+fo->node->marginTop(),
                           fo->width +ow*2 -fo->node->marginLeft() -fo->node->marginRight(),
                           fo->endY-fo->startY +ow*2 -fo->node->marginTop() -fo->node->marginBottom());
            } else {
                r += fo->node->visibleFlowRegion(x+fo->left+fo->node->marginLeft(), y+fo->startY+fo->node->marginTop());
            }
        }
    }
    return r;
}
                                 
void RenderBlock::paintFloats(PaintInfo& pI, int _tx, int _ty, bool paintSelection)
{
    if (!m_floatingObjects)
        return;

    FloatingObject* r;
    QListIterator<FloatingObject*> it(*m_floatingObjects);
    while ( it.hasNext() ) {
        r = it.next();
        // Only paint the object if our noPaint flag isn't set.
        if (r->node->isFloating() && !r->noPaint && !r->node->layer()) {
            PaintAction oldphase = pI.phase;
            if (paintSelection) {
                pI.phase = PaintActionSelection;
                r->node->paint(pI, _tx + r->left - r->node->xPos() + r->node->marginLeft(),
                               _ty + r->startY - r->node->yPos() + r->node->marginTop());
            }
            else {
                pI.phase = PaintActionElementBackground;
                r->node->paint(pI,
                               _tx + r->left - r->node->xPos() + r->node->marginLeft(),
                               _ty + r->startY - r->node->yPos() + r->node->marginTop());
                pI.phase = PaintActionChildBackgrounds;
                r->node->paint(pI,
                               _tx + r->left - r->node->xPos() + r->node->marginLeft(),
                               _ty + r->startY - r->node->yPos() + r->node->marginTop());
                pI.phase = PaintActionFloat;
                r->node->paint(pI,
                               _tx + r->left - r->node->xPos() + r->node->marginLeft(),
                               _ty + r->startY - r->node->yPos() + r->node->marginTop());
                pI.phase = PaintActionForeground;
                r->node->paint(pI,
                               _tx + r->left - r->node->xPos() + r->node->marginLeft(),
                               _ty + r->startY - r->node->yPos() + r->node->marginTop());
                pI.phase = PaintActionOutline;
                r->node->paint(pI,
                               _tx + r->left - r->node->xPos() + r->node->marginLeft(),
                               _ty + r->startY - r->node->yPos() + r->node->marginTop());
            }
            pI.phase = oldphase;
        }
    }
}

void RenderBlock::insertPositionedObject(RenderObject *o)
{
    // Create the list of special objects if we don't aleady have one
    if (!m_positionedObjects) {
        m_positionedObjects = new QList<RenderObject*>;
    }

    // Create the special object entry & append it to the list
    m_positionedObjects->append(o);
    o->setInPosObjectList();
}

void RenderBlock::removePositionedObject(RenderObject *o)
{
    if (m_positionedObjects) {
        m_positionedObjects->removeAll(o);
        if (m_positionedObjects->isEmpty()) {
            delete m_positionedObjects;
            m_positionedObjects = 0;
        }
    }
    o->setInPosObjectList( false );
}

void RenderBlock::insertFloatingObject(RenderObject *o)
{
    // Create the list of special objects if we don't aleady have one
    if (!m_floatingObjects) {
        m_floatingObjects = new QList<FloatingObject*>;
    }
    else {
        // Don't insert the object again if it's already in the list
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        FloatingObject* f;
        while ( it.hasNext() ) {
            f = it.next();
            if (f->node == o) return;
        }
    }

    // Create the special object entry & append it to the list

    FloatingObject *newObj;
    if (o->isFloating()) {
        // floating object
        o->layoutIfNeeded();

        if(o->style()->floating() & FLEFT)
            newObj = new FloatingObject(FloatingObject::FloatLeft);
        else
            newObj = new FloatingObject(FloatingObject::FloatRight);

        newObj->startY = -500000;
        newObj->endY = -500000;
        newObj->width = o->width() + o->marginLeft() + o->marginRight();
    }
    else {
        // We should never get here, as insertFloatingObject() should only ever be called with floating
        // objects.
        KHTMLAssert(false);
        newObj = 0; // keep gcc's uninitialized variable warnings happy
    }

    newObj->node = o;

    m_floatingObjects->append(newObj);
}

void RenderBlock::removeFloatingObject(RenderObject *o)
{
    if (m_floatingObjects) {
        QMutableListIterator<FloatingObject*> it(*m_floatingObjects);
        while (it.hasNext()) {
            if (it.next()->node == o) {
                delete it.peekPrevious();
                it.remove();
            }
        }
    }
}

void RenderBlock::positionNewFloats()
{
    if (!m_floatingObjects) return;
    QListIterator<FloatingObject*> it(*m_floatingObjects);
    it.toBack();
    if (!it.hasPrevious() || it.previous()->startY != -500000) return;
    FloatingObject *lastFloat;
    while(1)
    {
        lastFloat = it.hasPrevious() ? it.previous() : 0;
        if (!lastFloat || lastFloat->startY != -500000) {
            if (lastFloat) it.next();
            break;
        }
    }
    int y = m_height;

    // the float can not start above the y position of the last positioned float.
    if(lastFloat && lastFloat->startY > y)
        y = lastFloat->startY;

    KHTMLAssert( it.hasNext() );
    FloatingObject *f = it.next();

    while(f)
    {
        //skip elements copied from elsewhere and positioned elements
        if (f->node->containingBlock()!=this)
        {
            f = it.hasNext() ? it.next() : 0;
            continue;
        }

        RenderObject *o = f->node;
        int _height = o->height() + o->marginTop() + o->marginBottom();

        // floats avoid page-breaks
        if(canvas()->pagedMode())
        {
            int top = y;
            int bottom = y + o->height();
            if (crossesPageBreak(top, bottom) && o->height() < canvas()->pageHeight() ) {
                int newY = pageTopAfter(top);
#ifdef PAGE_DEBUG
                qDebug() << renderName() << " clearing float " << newY - y << "px";
#endif
                y = newY;
            }
        }

        int ro = rightOffset(); // Constant part of right offset.
        int lo = leftOffset(); // Constant part of left offset.
        int fwidth = f->width; // The width we look for.
                               //qDebug() << " Object width: " << fwidth << " available width: " << ro - lo;

        // in quirk mode, floated auto-width tables try to fit within remaining linewidth
        bool ftQuirk = o->isTable() && style()->htmlHacks() && o->style()->width().isAuto();
        if (ftQuirk)
            fwidth = qMin( o->minWidth()+o->marginLeft()+o->marginRight(), fwidth );

        if (ro - lo < fwidth)
            fwidth = ro - lo; // Never look for more than what will be available.

        if ( o->style()->clear() & CLEFT )
            y = qMax( leftBottom(), y );
        if ( o->style()->clear() & CRIGHT )
            y = qMax( rightBottom(), y );

        bool canClearLine;
        if (o->style()->floating() & FLEFT)
        {
            int heightRemainingLeft = 1;
            int heightRemainingRight = 1;
            int fx = leftRelOffset(y,lo, false, &heightRemainingLeft, &canClearLine);
            if (canClearLine)
            {
                while (rightRelOffset(y,ro, false, &heightRemainingRight)-fx < fwidth)
                {
                    y += qMin( heightRemainingLeft, heightRemainingRight );
                    fx = leftRelOffset(y,lo, false, &heightRemainingLeft);
                }
            }
            if (ftQuirk && (rightRelOffset(y,ro, false)-fx < f->width)) {
                o->setPos( o->xPos(), y + o->marginTop() );
                o->setChildNeedsLayout(true, false);
                o->layoutIfNeeded();
                _height = o->height() + o->marginTop() + o->marginBottom();
                f->width = o->width() + o->marginLeft() + o->marginRight();
            }
            f->left = fx;
            //qDebug() << "positioning left aligned float at (" << fx + o->marginLeft()  << "/" << y + o->marginTop() << ") fx=" << fx;
            o->setPos(fx + o->marginLeft(), y + o->marginTop());
        }
        else
        {
            int heightRemainingLeft = 1;
            int heightRemainingRight = 1;
            int fx = rightRelOffset(y,ro, false, &heightRemainingRight, &canClearLine);
            if (canClearLine)
            {
                while (fx - leftRelOffset(y,lo, false, &heightRemainingLeft) < fwidth)
                {
                    y += qMin(heightRemainingLeft, heightRemainingRight);
                    fx = rightRelOffset(y,ro, false, &heightRemainingRight);
                }
            }
            if (ftQuirk && (fx - leftRelOffset(y,lo, false) < f->width)) {
                o->setPos( o->xPos(), y + o->marginTop() );
                o->setChildNeedsLayout(true, false);
                o->layoutIfNeeded();
                _height = o->height() + o->marginTop() + o->marginBottom();
                f->width = o->width() + o->marginLeft() + o->marginRight();
            }
            f->left = fx - f->width;
            //qDebug() << "positioning right aligned float at (" << fx - o->marginRight() - o->width() << "/" << y + o->marginTop() << ")";
            o->setPos(fx - o->marginRight() - o->width(), y + o->marginTop());
        }

        if ( m_layer && hasOverflowClip()) {
            if (o->xPos()+o->width() > m_overflowWidth)
                m_overflowWidth = o->xPos()+o->width();
            else
            if (o->xPos() < m_overflowLeft)
                m_overflowLeft = o->xPos();
        }

        f->startY = y;
        f->endY = f->startY + _height;

        //qDebug() << "floatingObject x/y= (" << f->left << "/" << f->startY << "-" << f->width << "/" << f->endY - f->startY << ")";

        f = it.hasNext() ? it.next() : 0;
    }
}

void RenderBlock::newLine()
{
    positionNewFloats();
    // set y position
    int newY = 0;
    switch(m_clearStatus)
    {
        case CLEFT:
            newY = leftBottom();
            break;
        case CRIGHT:
            newY = rightBottom();
            break;
        case CBOTH:
            newY = floatBottom();
        default:
            break;
    }
    if(m_height < newY)
    {
        //      qDebug() << "adjusting y position";
        m_height = newY;
    }
    m_clearStatus = CNONE;
}

int
RenderBlock::leftOffset() const
{
    int left = borderLeft()+paddingLeft();
    if (m_layer && scrollsOverflowY() && m_layer->hasReversedScrollbar())
        left += m_layer->verticalScrollbarWidth();
    return left;
}

int
RenderBlock::leftRelOffset(int y, int fixedOffset, bool applyTextIndent, int *heightRemaining, bool *canClearLine ) const
{
    int left = fixedOffset;
    if (canClearLine) *canClearLine = true;

    if (m_floatingObjects) {
        if ( heightRemaining ) *heightRemaining = 1;
        FloatingObject* r;
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        while ( it.hasNext() )
        {
            r = it.next();
            //qDebug() <<(void *)this << " left: sy, ey, x, w " << r->startY << "," << r->endY << "," << r->left << "," << r->width << " ";
            if (r->startY <= y && r->endY > y &&
                r->type == FloatingObject::FloatLeft &&
                r->left + r->width > left) {
                left = r->left + r->width;
                if ( heightRemaining ) *heightRemaining = r->endY - y;
                if ( canClearLine ) *canClearLine = (r->node->style()->floating() != FLEFT_ALIGN);
            }
        }
    }

    if (applyTextIndent && m_firstLine && style()->direction() == LTR ) {
        int cw=0;
        if (style()->textIndent().isPercent())
            cw = containingBlock()->contentWidth();
        left += style()->textIndent().minWidth(cw);
    }

    //qDebug() << "leftOffset(" << y << ") = " << left;
    return left;
}

int
RenderBlock::rightOffset() const
{
    int right = m_width - borderRight() - paddingRight();
    if (m_layer && scrollsOverflowY() && !m_layer->hasReversedScrollbar())
        right -= m_layer->verticalScrollbarWidth();
    return right;
}

int
RenderBlock::rightRelOffset(int y, int fixedOffset, bool applyTextIndent, int *heightRemaining, bool *canClearLine ) const
{
    int right = fixedOffset;
    if (canClearLine) *canClearLine = true;

    if (m_floatingObjects) {
        if (heightRemaining) *heightRemaining = 1;
        FloatingObject* r;
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        while ( it.hasNext() )
        {
            r = it.next();
            //qDebug() << "right: sy, ey, x, w " << r->startY << "," << r->endY << "," << r->left << "," << r->width << " ";
            if (r->startY <= y && r->endY > y &&
                r->type == FloatingObject::FloatRight &&
                r->left < right) {
                right = r->left;
                if ( heightRemaining ) *heightRemaining = r->endY - y;
                if ( canClearLine ) *canClearLine = (r->node->style()->floating() != FRIGHT_ALIGN);
            }
        }
    }

    if (applyTextIndent &&  m_firstLine && style()->direction() == RTL ) {
        int cw=0;
        if (style()->textIndent().isPercent())
            cw = containingBlock()->contentWidth();
        right -= style()->textIndent().minWidth(cw);
    }

    //qDebug() << "rightOffset(" << y << ") = " << right;
    return right;
}

unsigned short
RenderBlock::lineWidth(int y, bool *canClearLine) const
{
    //qDebug() << "lineWidth(" << y << ")=" << rightOffset(y) - leftOffset(y);
    int result;
    if (canClearLine) {
        bool rightCanClearLine;
        bool leftCanClearLine;
        result = rightOffset(y, &rightCanClearLine) - leftOffset(y, &leftCanClearLine);
        *canClearLine = rightCanClearLine && leftCanClearLine;
    } else
        result = rightOffset(y) - leftOffset(y);
    return (result < 0) ? 0 : result;
}

int
RenderBlock::nearestFloatBottom(int height) const
{
   if (!m_floatingObjects)
        return 0;

    int bottom = INT_MAX;
    FloatingObject* r;
    QListIterator<FloatingObject*> it(*m_floatingObjects);
    while (it.hasNext()) {
        r = it.next();
        if (r->endY > height)
            bottom = qMin(r->endY, bottom);
    }

    return bottom == INT_MAX ? 0 : bottom;
}

int RenderBlock::floatBottom() const
{
    if (!m_floatingObjects) return 0;
    int bottom=0;
    FloatingObject* r;
    QListIterator<FloatingObject*> it(*m_floatingObjects);
    while ( it.hasNext() ) {
        r = it.next();
        if (r->endY>bottom)
            bottom=r->endY;
    }
    return bottom;
}

int RenderBlock::lowestPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int bottom = RenderFlow::lowestPosition(includeOverflowInterior, includeSelf);
    if (!includeOverflowInterior && hasOverflowClip())
        return bottom;
    if (includeSelf && m_overflowHeight > bottom)
        bottom = m_overflowHeight;

    if (m_floatingObjects) {
        FloatingObject* r;
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        while ( it.hasNext() ) {
            r = it.next();
            if (!r->noPaint) {
                int lp = r->startY + r->node->marginTop() + r->node->lowestPosition(false);
                bottom = qMax(bottom, lp);
            }
        }
    }
    bottom = qMax(bottom, lowestAbsolutePosition());

    if (!includeSelf && lastLineBox()) {
        int lp = lastLineBox()->yPos() + lastLineBox()->height();
        bottom = qMax(bottom, lp);
    }

    return bottom;
}

int RenderBlock::lowestAbsolutePosition() const
{
    if (!m_positionedObjects)
        return 0;

    // Fixed positioned objects do not scroll and thus should not constitute
    // part of the lowest position.
    int bottom = 0;
    RenderObject* r;
    QListIterator<RenderObject*> it(*m_positionedObjects);
    while (it.hasNext()) {
        r = it.next();
        if (r->style()->position() == PFIXED)
            continue;
        int lp = r->yPos() + r->lowestPosition(false);
        bottom = qMax(bottom, lp);
    }
    return bottom;
}

int RenderBlock::rightmostPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int right = RenderFlow::rightmostPosition(includeOverflowInterior, includeSelf);
    if (!includeOverflowInterior && hasOverflowClip())
        return right;
    if (includeSelf && m_overflowWidth > right)
        right = m_overflowWidth;

    if (m_floatingObjects) {
        FloatingObject* r;
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        while ( it.hasNext() ) {
            r = it.next();
            if (!r->noPaint) {
                int rp = r->left + r->node->marginLeft() + r->node->rightmostPosition(false);
           	right = qMax(right, rp);
            }
        }
    }
    right = qMax(right, rightmostAbsolutePosition());

    if (!includeSelf && firstLineBox()) {
        for (InlineRunBox* currBox = firstLineBox(); currBox; currBox = currBox->nextLineBox()) {
            int rp = currBox->xPos() + currBox->width();
            right = qMax(right, rp);
        }
    }

    return right;
}

int RenderBlock::rightmostAbsolutePosition() const
{
    if (!m_positionedObjects)
        return 0;
    int right = 0;
    RenderObject* r;
    QListIterator<RenderObject*> it(*m_positionedObjects);
    while (it.hasNext()) {
        r = it.next();
        if (r->style()->position() == PFIXED)
            continue;
        int rp = r->xPos() + r->rightmostPosition(false);
        right = qMax(right, rp);
    }
    return right;
}

int RenderBlock::leftmostPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int left = RenderFlow::leftmostPosition(includeOverflowInterior, includeSelf);
    if (!includeOverflowInterior && hasOverflowClip())
        return left;

    if (includeSelf && m_overflowLeft < left)
        left = m_overflowLeft;

    if (m_floatingObjects) {
        FloatingObject* r;
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        while ( it.hasNext() ) {
            r = it.next();
            if (!r->noPaint) {
                int lp = r->left + r->node->marginLeft() + r->node->leftmostPosition(false);
                left = qMin(left, lp);
            }
        }
    }
    left = qMin(left, leftmostAbsolutePosition());

    if (!includeSelf && firstLineBox()) {
        for (InlineRunBox* currBox = firstLineBox(); currBox; currBox = currBox->nextLineBox())
            left = qMin(left, (int)currBox->xPos());
    }

    return left;
}

int RenderBlock::leftmostAbsolutePosition() const
{
    if (!m_positionedObjects)
        return 0;
    int  left = 0;
    RenderObject* r;
    QListIterator<RenderObject*> it(*m_positionedObjects);
    while (it.hasNext()) {
        r = it.next();
        if (r->style()->position() == PFIXED)
            continue;
        int lp = r->xPos() + r->leftmostPosition(false);
        left = qMin(left, lp);
    }
    return left;
}

int RenderBlock::highestPosition(bool includeOverflowInterior, bool includeSelf) const
{
    int top = RenderFlow::highestPosition(includeOverflowInterior, includeSelf);
    if (!includeOverflowInterior && hasOverflowClip())
        return top;

    if (includeSelf && m_overflowTop < top)
        top = m_overflowTop;

    if (m_floatingObjects) {
        FloatingObject* r;
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        while ( it.hasNext() ) {
            r = it.next();
            if (!r->noPaint) {
                int hp = r->startY + r->node->marginTop() + r->node->highestPosition(false);
                top = qMin(top, hp);
            }
        }
    }
    top = qMin(top, highestAbsolutePosition());

    if (!includeSelf && firstLineBox()) {
        top = qMin(top, (int)firstLineBox()->yPos());
    }

    return top;
}

int RenderBlock::highestAbsolutePosition() const
{
    if (!m_positionedObjects)
        return 0;
    int  top = 0;
    RenderObject* r;
    QListIterator<RenderObject*> it(*m_positionedObjects);
    while ( it.hasNext() ) {
        r = it.next();
        if (r->style()->position() == PFIXED)
            continue;
        int hp = r->yPos() + r->highestPosition(false);
        hp = qMin(top, hp);
    }
    return top;
}

int
RenderBlock::leftBottom()
{
    if (!m_floatingObjects) return 0;
    int bottom=0;
    FloatingObject* r;
    QListIterator<FloatingObject*> it(*m_floatingObjects);
    while ( it.hasNext() ) {
        r = it.next();
        if (r->endY>bottom && r->type == FloatingObject::FloatLeft)
            bottom=r->endY;
    }
    return bottom;
}

int
RenderBlock::rightBottom()
{
    if (!m_floatingObjects) return 0;
    int bottom=0;
    FloatingObject* r;
    QListIterator<FloatingObject*> it(*m_floatingObjects);
    while ( it.hasNext() ) {
        r = it.next();
        if (r->endY>bottom && r->type == FloatingObject::FloatRight)
            bottom=r->endY;
    }
    return bottom;
}

void
RenderBlock::clearFloats()
{
    if (m_floatingObjects) {
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        while ( it.hasNext() )
            delete it.next();
        m_floatingObjects->clear();
    }

    // we are done if the element defines a new block formatting context
    if (flowAroundFloats() || isRoot() || isCanvas() || isFloatingOrPositioned() || isTableCell()) return;

    RenderObject *prev = previousSibling();

    // find the element to copy the floats from
    // pass non-flows
    // pass fAF's
    bool parentHasFloats = false;
    while (prev) {
        if (!prev->isRenderBlock() || prev->isFloatingOrPositioned() || prev->flowAroundFloats()) {
            if ( prev->isFloating() && parent()->isRenderBlock() ) {
                parentHasFloats = true;
            }
            prev = prev->previousSibling();
        } else
            break;
    }

    int offset = m_y;
    if (parentHasFloats)
    {
        addOverHangingFloats( static_cast<RenderBlock *>( parent() ),
                              parent()->borderLeft() + parent()->paddingLeft(), offset, false );
    }

    int xoffset = 0;
    if (prev) {
        if(prev->isTableCell()) return;
        offset -= prev->yPos();
    } else {
        prev = parent();
        if(!prev) return;
        xoffset += prev->borderLeft() + prev->paddingLeft();
    }
    //qDebug() << "RenderBlock::clearFloats found previous "<< (void *)this << " prev=" << (void *)prev;

    // add overhanging special objects from the previous RenderBlock
    if(!prev->isRenderBlock()) return;
    RenderBlock * flow = static_cast<RenderBlock *>(prev);
    if(!flow->m_floatingObjects) return;
    if(flow->floatBottom() > offset)
        addOverHangingFloats( flow, xoffset, offset, false );
}

void RenderBlock::addOverHangingFloats( RenderBlock *flow, int xoff, int offset, bool child )
{
#ifdef DEBUG_LAYOUT
    qDebug() << (void *)this << ": adding overhanging floats xoff=" << xoff << "  offset=" << offset << " child=" << child;
#endif

    // Prevent floats from being added to the canvas by the root element, e.g., <html>.
    if ( !flow->m_floatingObjects || (child && flow->isRoot()) )
        return;

    // if I am clear of my floats, don't add them
    // the CSS spec also mentions that child floats
    // are not cleared.
    if (!child && style()->clear() == CBOTH)
    {
        return;
    }

    QListIterator<FloatingObject*> it(*flow->m_floatingObjects);
    FloatingObject *r;
    while ( it.hasNext() ) {
        r = it.next();

        if (!child && r->type == FloatingObject::FloatLeft && style()->clear() == CLEFT )
            continue;
        if (!child && r->type == FloatingObject::FloatRight && style()->clear() == CRIGHT )
            continue;

        if ( ( !child && r->endY > offset ) ||
             ( child && flow->yPos() + r->endY > height() ) ) {
            if (child && !r->crossedLayer) {
                if (flow->enclosingLayer() == enclosingLayer()) {
                  // Set noPaint to true only if we didn't cross layers.
                  r->noPaint = true;
                } else {
                  r->crossedLayer = true;
                }
            }

            // don't insert it twice!
            if (!containsFloat(r->node)) {
                FloatingObject *floatingObj = new FloatingObject(KDE_CAST_BF_ENUM(FloatingObject::Type, r->type));
                floatingObj->startY = r->startY - offset;
                floatingObj->endY = r->endY - offset;
                floatingObj->left = r->left - xoff;
                // Applying the child's margin makes no sense in the case where the child was passed in.
                // since his own margin was added already through the subtraction of the |xoff| variable
                // above.  |xoff| will equal -flow->marginLeft() in this case, so it's already been taken
                // into account.  Only apply this code if |child| is false, since otherwise the left margin
                // will get applied twice. -dwh
                if (!child && flow != parent())
                    floatingObj->left += flow->marginLeft();
                if ( !child ) {
                    floatingObj->left -= marginLeft();
                    floatingObj->noPaint = true;
                }
                else {
                    floatingObj->noPaint = (r->crossedLayer || !r->noPaint);
                    floatingObj->crossedLayer = r->crossedLayer;
                }

                floatingObj->width = r->width;
                floatingObj->node = r->node;
                if (!m_floatingObjects)
                    m_floatingObjects = new QList<FloatingObject*>;
                m_floatingObjects->append(floatingObj);
#ifdef DEBUG_LAYOUT
                qDebug() << "addOverHangingFloats x/y= (" << floatingObj->left << "/" << floatingObj->startY << "-" << floatingObj->width << "/" << floatingObj->endY - floatingObj->startY << ")";
#endif
            }
        }
    }
}

bool RenderBlock::containsFloat(RenderObject* o) const
{
    if (m_floatingObjects) {
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        while (it.hasNext()) {
            if (it.next()->node == o)
                return true;
        }
    }
    return false;
}

void RenderBlock::markAllDescendantsWithFloatsForLayout(RenderObject* floatToRemove)
{
    dirtyFormattingContext(false);
    setChildNeedsLayout(true);

    if (floatToRemove)
        removeFloatingObject(floatToRemove);

    // Iterate over our children and mark them as needed.
    if (!childrenInline()) {
        for (RenderObject* child = firstChild(); child; child = child->nextSibling()) {
            if (isBlockFlow() && !child->isFloatingOrPositioned() &&
                ((floatToRemove ? child->containsFloat(floatToRemove) : child->hasFloats()) || child->usesLineWidth()))
                child->markAllDescendantsWithFloatsForLayout(floatToRemove);
        }
    }
}

int RenderBlock::getClearDelta(RenderObject *child, int yPos)
{
    if (!hasFloats())
        return 0;

    //qDebug() << "getClearDelta on child " << child << " oldheight=" << m_height;
    bool clearSet = child->style()->clear() != CNONE;
    int bottom = 0;
    switch(child->style()->clear())
    {
        case CNONE:
            break;
        case CLEFT:
            bottom = leftBottom();
            break;
        case CRIGHT:
            bottom = rightBottom();
            break;
        case CBOTH:
            bottom = floatBottom();
            break;
    }

    // We also clear floats if we are too big to sit on the same line as
    // a float, and happen to flow around floats.
    int result = clearSet ? qMax(0, bottom - yPos) : 0;
    if (!result && child->flowAroundFloats()) {
        bool canClear = true;
        bool needsRecalc = child->usesLineWidth();
        int cury = yPos;
        int childw = 0;
        int aw = contentWidth();
#if 0
        // this is a silly Gecko compatibility hack - enable only if it becomes
        // necessary, and then check regularly with new Gecko versions
        if (!style()->hasBorder()) {
            RenderObject* ps = child;
            while ((ps = ps->previousSibling())) {
                if (!ps->isFloating() && !ps->isPositioned())
                    break;
            }
            if (!ps)
                return 0;
        }
#endif
        while (true) {
            int curw = lineWidth(cury, &canClear);
            if (!canClear || curw == aw)
                return cury-yPos;
            if (!childw || needsRecalc) {
                int oy = child->yPos();
                int ow = child->width();
                child->setPos(child->xPos(), cury);
                child->calcWidth();
                childw = child->width();
                child->setPos(child->xPos(), oy);
                child->setWidth(ow);
            }
            if (childw <= curw)
                return cury-yPos;
            if (!(cury = nearestFloatBottom(cury)))
                return 0;
        }
    }
    return result;
}

bool RenderBlock::isPointInScrollbar(int _x, int _y, int _tx, int _ty)
{
    if (!scrollsOverflow() || !m_layer)
        return false;

    if (m_layer->verticalScrollbarWidth()) {
        bool rtl = QApplication::isRightToLeft();
        QRect vertRect(_tx + (rtl ? borderLeft() :  width() - borderRight() - m_layer->verticalScrollbarWidth()),
                       _ty + borderTop() - borderTopExtra(),
                       m_layer->verticalScrollbarWidth(),
                       height() + borderTopExtra() + borderBottomExtra()-borderTop()-borderBottom());
        if (vertRect.contains(_x, _y)) {
            RenderLayer::gScrollBar = m_layer->verticalScrollbar();
            return true;
        }
    }

    if (m_layer->horizontalScrollbarHeight()) {
        QRect horizRect(_tx + borderLeft(),
                        _ty + height() - borderBottom() + borderBottomExtra() - m_layer->horizontalScrollbarHeight(),
                        width()-borderLeft()-borderRight(),
                        m_layer->horizontalScrollbarHeight());
        if (horizRect.contains(_x, _y)) {
            RenderLayer::gScrollBar = m_layer->horizontalScrollbar();
            return true;
        }
    }

    return false;
}

bool RenderBlock::nodeAtPoint(NodeInfo& info, int _x, int _y, int _tx, int _ty, HitTestAction hitTestAction, bool inBox)
{
    bool inScrollbar = isPointInScrollbar(_x, _y, _tx+xPos(), _ty+yPos());
    if (inScrollbar && hitTestAction != HitTestChildrenOnly)
        inBox = true;

    if (hitTestAction != HitTestSelfOnly && m_floatingObjects && !inScrollbar) {
        int stx = _tx + xPos();
        int sty = _ty + yPos();
        if (hasOverflowClip() && m_layer)
            m_layer->subtractScrollOffset(stx, sty);
        FloatingObject* o;
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        it.toBack();
        while (it.hasPrevious()) {
            o = it.previous();
            if (!o->noPaint && !o->node->layer())
                inBox |= o->node->nodeAtPoint(info, _x, _y,
                                              stx+o->left + o->node->marginLeft() - o->node->xPos(),
                                              sty+o->startY + o->node->marginTop() - o->node->yPos(), HitTestAll ) ;
        }
    }
    inBox |= RenderFlow::nodeAtPoint(info, _x, _y, _tx, _ty, hitTestAction, inBox);
    return inBox;
}

RenderPosition RenderBlock::positionForBox(InlineBox *box, bool start) const
{
    if (!box)
        return RenderPosition();

    if (!box->object()->element())
        return RenderPosition(element(), start ? caretMinOffset() : caretMaxOffset());

    if (!box->isInlineTextBox())
        return RenderPosition(box->object()->element(), start ? box->object()->caretMinOffset() : box->object()->caretMaxOffset());

    InlineTextBox *textBox = static_cast<InlineTextBox *>(box);
    return RenderPosition(box->object()->element(), start ? textBox->start() : textBox->start() + textBox->len());
}

RenderPosition RenderBlock::positionForRenderer(RenderObject *renderer, bool start) const
{
    if (!renderer)
        return RenderPosition(element(), 0);

    NodeImpl *node = renderer->element() ? renderer->element() : element();
    if (!node)
        return RenderPosition();

    long offset = start ? node->caretMinOffset() : node->caretMaxOffset();
    return RenderPosition(node, offset);
}

RenderPosition RenderBlock::positionForCoordinates(int _x, int _y)
{
    if (isTable())
        return RenderFlow::positionForCoordinates(_x, _y);

    int absx, absy;
    absolutePosition(absx, absy);

    int top = absy + borderTop() + paddingTop();
    int bottom = top + contentHeight();

    if (_y < top)
        // y coordinate is above block
        return positionForRenderer(firstLeafChild(), true);

    if (_y >= bottom)
        // y coordinate is below block
        return positionForRenderer(lastLeafChild(), false);

    if (childrenInline()) {
        if (!firstRootBox())
            return Position(element(), 0);

        if (_y >= top && _y < absy + firstRootBox()->topOverflow())
            // y coordinate is above first root line box
            return positionForBox(firstRootBox()->firstLeafChild(), true);

        // look for the closest line box in the root box which is at the passed-in y coordinate
        for (RootInlineBox *root = firstRootBox(); root; root = root->nextRootBox()) {
            top = absy + root->topOverflow();
            // set the bottom based on whether there is a next root box
            if (root->nextRootBox())
                bottom = absy + root->nextRootBox()->topOverflow();
            else
                bottom = absy + root->bottomOverflow();
            // check if this root line box is located at this y coordinate
            if (_y >= top && _y < bottom && root->firstChild()) {
                InlineBox *closestBox = root->closestLeafChildForXPos(_x, absx);
                if (closestBox) {
                    // pass the box a y position that is inside it
                    return closestBox->object()->positionForCoordinates(_x, absy + closestBox->m_y);
                }
            }
        }

        if (lastRootBox())
            // y coordinate is below last root line box
            return positionForBox(lastRootBox()->lastLeafChild(), false);

        return RenderPosition(element(), 0);
    }

    // see if any child blocks exist at this y coordinate
    for (RenderObject *renderer = firstChild(); renderer; renderer = renderer->nextSibling()) {
        if (renderer->isFloatingOrPositioned())
            continue;
        renderer->absolutePosition(absx, top);
        RenderObject *next = renderer->nextSibling();
        while (next && next->isFloatingOrPositioned())
            next = next->nextSibling();
        if (next)
            next->absolutePosition(absx, bottom);
        else
            bottom = top + contentHeight();
        if (_y >= top && _y < bottom) {
            return renderer->positionForCoordinates(_x, _y);
        }
    }

    // pass along to the first child
    if (firstChild())
        return firstChild()->positionForCoordinates(_x, _y);

    // still no luck...return this render object's element and offset 0
    return RenderPosition(element(), 0);
}

void RenderBlock::calcMinMaxWidth()
{
    KHTMLAssert( !minMaxKnown() );

#ifdef DEBUG_LAYOUT
    qDebug() << renderName() << "(RenderBlock)::calcMinMaxWidth() this=" << this;
#endif
    if (!isTableCell() && style()->width().isFixed() && style()->width().isPositive())
        m_minWidth = m_maxWidth = calcContentWidth(style()->width().value());
    else {
        m_minWidth = 0;
        m_maxWidth = 0;

        bool noWrap = !style()->autoWrap();
        if (childrenInline())
            calcInlineMinMaxWidth();
        else
            calcBlockMinMaxWidth();

        if(m_maxWidth < m_minWidth) m_maxWidth = m_minWidth;

        if (noWrap && childrenInline()) {
             m_minWidth = m_maxWidth;

            // A horizontal marquee with inline children has no minimum width.
            if (style()->overflowX() == OMARQUEE && m_layer && m_layer->marquee() &&
                m_layer->marquee()->isHorizontal() && !m_layer->marquee()->isUnfurlMarquee())
                m_minWidth = 0;
        }

        if (isTableCell()) {
            Length w = static_cast<RenderTableCell*>(this)->styleOrColWidth();
            if (w.isFixed() && w.isPositive())
                m_maxWidth = qMax((int)m_minWidth, calcContentWidth(w.value()));
        }
    }

    if (style()->minWidth().isFixed() && style()->minWidth().isPositive()) {
        m_maxWidth = qMax(m_maxWidth, (int)calcContentWidth(style()->minWidth().value()));
        m_minWidth = qMax(m_minWidth, (short)calcContentWidth(style()->minWidth().value()));
    }

    if (style()->maxWidth().isFixed() && !style()->maxWidth().isUndefined()) {
        m_maxWidth = qMin(m_maxWidth, (int)calcContentWidth(style()->maxWidth().value()));
        m_minWidth = qMin(m_minWidth, (short)calcContentWidth(style()->maxWidth().value()));
    }

    int toAdd = 0;
    toAdd = borderLeft() + borderRight() + paddingLeft() + paddingRight();

    m_minWidth += toAdd;
    m_maxWidth += toAdd;

    setMinMaxKnown();

    //qDebug() << "Text::calcMinMaxWidth(" << this << "): min = " << m_minWidth << " max = " << m_maxWidth;
    // ### compare with min/max width set in style sheet...
}

// bidi.cpp defines the following functions too.
// Maybe these should not be static, after all...


static int getBPMWidth(int childValue, Length cssUnit)
{
    if (!cssUnit.isAuto())
        return (cssUnit.isFixed() ? cssUnit.value() : childValue);
    return 0;
}

static int getBorderPaddingMargin(RenderObject* child, bool endOfInline)
{
    RenderStyle* cstyle = child->style();
    int result = 0;
    bool leftSide = (cstyle->direction() == LTR) ? !endOfInline : endOfInline;
    result += getBPMWidth((leftSide ? child->marginLeft() : child->marginRight()),
                          (leftSide ? cstyle->marginLeft() :
                                      cstyle->marginRight()));
    result += getBPMWidth((leftSide ? child->paddingLeft() : child->paddingRight()),
                          (leftSide ? cstyle->paddingLeft() :
                                      cstyle->paddingRight()));
    result += leftSide ? child->borderLeft() : child->borderRight();
    return result;
}

static void stripTrailingSpace(bool preserveWS,
                               int& inlineMax, int& inlineMin,
                               RenderObject* trailingSpaceChild)
{
    if (!preserveWS && trailingSpaceChild && trailingSpaceChild->isText()) {
        // Collapse away the trailing space at the end of a block.
        RenderText* t = static_cast<RenderText *>(trailingSpaceChild);
        const Font *f = t->htmlFont( false );
        QChar space[1]; space[0] = ' ';
        int spaceWidth = f->charWidth(space, 1, 0, true/*fast algo*/);
        inlineMax -= spaceWidth;
        if (inlineMin > inlineMax)
            inlineMin = inlineMax;
    }
}

void RenderBlock::calcInlineMinMaxWidth()
{
    int inlineMax=0;
    int inlineMin=0;

    int cw = containingBlock()->contentWidth();

    // If we are at the start of a line, we want to ignore all white-space.
    // Also strip spaces if we previously had text that ended in a trailing space.
    bool stripFrontSpaces = true;

    bool isTcQuirk = isTableCell() && style()->htmlHacks() && style()->width().isAuto();

    RenderObject* trailingSpaceChild = 0;

    bool autoWrap, oldAutoWrap;
    autoWrap = oldAutoWrap = style()->autoWrap();

    InlineMinMaxIterator childIterator(this, this);
    bool addedTextIndent = false; // Only gets added in once.
    RenderObject* prevFloat = 0;
    while (RenderObject* child = childIterator.next())
    {
        autoWrap = child->isReplaced() ? child->parent()->style()->autoWrap() : child->style()->autoWrap();

        if( !child->isBR() )
        {
            // Step One: determine whether or not we need to go ahead and
            // terminate our current line.  Each discrete chunk can become
            // the new min-width, if it is the widest chunk seen so far, and
            // it can also become the max-width.

            // Children fall into three categories:
            // (1) An inline flow object.  These objects always have a min/max of 0,
            // and are included in the iteration solely so that their margins can
            // be added in.
            //
            // (2) An inline non-text non-flow object, e.g., an inline replaced element.
            // These objects can always be on a line by themselves, so in this situation
            // we need to go ahead and break the current line, and then add in our own
            // margins and min/max width on its own line, and then terminate the line.
            //
            // (3) A text object.  Text runs can have breakable characters at the start,
            // the middle or the end.  They may also lose whitespace off the front if
            // we're already ignoring whitespace.  In order to compute accurate min-width
            // information, we need three pieces of information.
            // (a) the min-width of the first non-breakable run.  Should be 0 if the text string
            // starts with whitespace.
            // (b) the min-width of the last non-breakable run. Should be 0 if the text string
            // ends with whitespace.
            // (c) the min/max width of the string (trimmed for whitespace).
            //
            // If the text string starts with whitespace, then we need to go ahead and
            // terminate our current line (unless we're already in a whitespace stripping
            // mode.
            //
            // If the text string has a breakable character in the middle, but didn't start
            // with whitespace, then we add the width of the first non-breakable run and
            // then end the current line.  We then need to use the intermediate min/max width
            // values (if any of them are larger than our current min/max).  We then look at
            // the width of the last non-breakable run and use that to start a new line
            // (unless we end in whitespace).
            RenderStyle* cstyle = child->style();
            int childMin = 0;
            int childMax = 0;

            if (!child->isText()) {
                // Case (1) and (2).  Inline replaced and inline flow elements.
                if (child->isInlineFlow()) {
                    // Add in padding/border/margin from the appropriate side of
                    // the element.
                    int bpm = getBorderPaddingMargin(child, childIterator.endOfInline);
                    childMin += bpm;
                    childMax += bpm;

                    inlineMin += childMin;
                    inlineMax += childMax;
                    if (child->isWordBreak()) {
                        // End a line and start a new line.
                        m_minWidth = qMax(inlineMin, (int)m_minWidth);
                        inlineMin = 0;
                    }
                }
                else {
                    // Inline replaced elements add in their margins to their min/max values.
                    int margins = 0;
                    LengthType type = cstyle->marginLeft().type();                  
                    if ( type == Fixed )
                        margins += cstyle->marginLeft().value();
                    type = cstyle->marginRight().type();
                    if ( type == Fixed )
                        margins += cstyle->marginRight().value();
                    childMin += margins;
                    childMax += margins;
                }
            }

            if (!child->isRenderInline() && !child->isText()) {
                // Case (2). Inline replaced elements and floats.

                // Common wrapping quirk
                bool qBreak = isTcQuirk && !child->isFloating();

                childMin += child->minWidth();
                childMax += child->maxWidth();

                // Check our "clear" setting.
                bool clearPreviousFloat = false;
                if (child->isFloating()) {
                    if (prevFloat &&
                         (((prevFloat->style()->floating() & FLEFT) && (child->style()->clear() & CLEFT)) ||
                          ((prevFloat->style()->floating() & FRIGHT) && (child->style()->clear() & CRIGHT)))) {
                        clearPreviousFloat = true;
                    }
                    prevFloat = child;
                }

                if ((!qBreak && (autoWrap || oldAutoWrap)) || clearPreviousFloat) {
                    if(m_minWidth < inlineMin) m_minWidth = inlineMin;
                    inlineMin = 0;
                }

                // If we're supposed to clear the previous float, then terminate maxwidth as well.
                if (clearPreviousFloat) {
                    m_maxWidth = qMax(inlineMax, m_maxWidth);
                    inlineMax = 0;
                }

                // Add in text-indent.  This is added in only once.
                int ti = 0;
                if ( !addedTextIndent ) {
                    addedTextIndent = true;
                    ti = style()->textIndent().minWidth( cw );
                    childMin+=ti;
                    childMax+=ti;
                }

                // Add our width to the max.
                inlineMax += childMax;

                if ((!autoWrap && !child->isFloating())||qBreak) {
                    inlineMin += childMin;
                } else {
                    // Now check our line.
                    m_minWidth = qMax(childMin, (int)m_minWidth);

                    // Now start a new line.
                    inlineMin = 0;
                }

                // We are no longer stripping whitespace at the start of
                // a line.
                if (!child->isFloating()) {
                    stripFrontSpaces = false;
                   trailingSpaceChild = 0;
                }
            }
            else if (child->isText())
            {
                // Case (3). Text.
                RenderText* t = static_cast<RenderText *>(child);

                // Determine if we have a breakable character.  Pass in
                // whether or not we should ignore any spaces at the front
                // of the string.  If those are going to be stripped out,
                // then they shouldn't be considered in the breakable char
                // check.
                bool hasBreakableChar, hasBreak;
                int beginMin, endMin;
                bool beginWS, endWS;
                int beginMax, endMax;
                t->trimmedMinMaxWidth(beginMin, beginWS, endMin, endWS, hasBreakableChar,
                                      hasBreak, beginMax, endMax,
                                      childMin, childMax, stripFrontSpaces);

                // This text object is insignificant and will not be rendered.  Just
                // continue.
                if (!hasBreak && childMax == 0) continue;

                if (stripFrontSpaces)
                    trailingSpaceChild = child;
                else
                    trailingSpaceChild = 0;

                // Add in text-indent.  This is added in only once.
                int ti = 0;
                if (!addedTextIndent) {
                    addedTextIndent = true;
                    ti = style()->textIndent().minWidth(cw);
                    childMin+=ti; beginMin += ti;
                    childMax+=ti; beginMax += ti;
                }

                // If we have no breakable characters at all,
                // then this is the easy case. We add ourselves to the current
                // min and max and continue.
                if (!hasBreakableChar) {
                    inlineMin += childMin;
                }
                else {
                    // We have a breakable character.  Now we need to know if
                    // we start and end with whitespace.
                    if (beginWS) {
                        // Go ahead and end the current line.
                        if(m_minWidth < inlineMin) m_minWidth = inlineMin;
                    }
                    else {
                        inlineMin += beginMin;
                        if(m_minWidth < inlineMin) m_minWidth = inlineMin;
                        childMin -= ti;
                    }

                    inlineMin = childMin;

                    if (endWS) {
                        // We end in whitespace, which means we can go ahead
                        // and end our current line.
                        if(m_minWidth < inlineMin) m_minWidth = inlineMin;
                        inlineMin = 0;
                    }
                    else {
                        if(m_minWidth < inlineMin) m_minWidth = inlineMin;
                        inlineMin = endMin;
                    }
                }

                if (hasBreak) {
                    inlineMax += beginMax;
                    if (m_maxWidth < inlineMax) m_maxWidth = inlineMax;
                    if (m_maxWidth < childMax) m_maxWidth = childMax;
                    inlineMax = endMax;
                }
                else
                    inlineMax += childMax;
            }

            // Ignore spaces after a list marker.
            if (child->isListMarker())
                stripFrontSpaces = true;
        }
        else
        {
            if(m_minWidth < inlineMin) m_minWidth = inlineMin;
            if(m_maxWidth < inlineMax) m_maxWidth = inlineMax;
            inlineMin = inlineMax = 0;
            stripFrontSpaces = true;
            trailingSpaceChild = 0;
        }

        oldAutoWrap = autoWrap;
    }

    stripTrailingSpace(style()->preserveWS(), inlineMax, inlineMin, trailingSpaceChild);

    if(m_minWidth < inlineMin) m_minWidth = inlineMin;
    if(m_maxWidth < inlineMax) m_maxWidth = inlineMax;
    //         qDebug() << "m_minWidth=" << m_minWidth
    // 			<< " m_maxWidth=" << m_maxWidth << endl;
}

// Use a very large value (in effect infinite).
#define BLOCK_MAX_WIDTH 15000

void RenderBlock::calcBlockMinMaxWidth()
{
    bool nowrap = !style()->autoWrap();

    RenderObject *child = firstChild();
    int floatLeftWidth = 0, floatRightWidth = 0;

    while(child != 0)
    {
        // positioned children don't affect the minmaxwidth
        if (child->isPositioned()) {
            child = child->nextSibling();
            continue;
        }

         if (child->isFloating() || child->flowAroundFloats()) {
             int floatTotalWidth = floatLeftWidth + floatRightWidth;
             if (child->style()->clear() & CLEFT) {
                 m_maxWidth = qMax(floatTotalWidth, m_maxWidth);
                 floatLeftWidth = 0;
             }
             if (child->style()->clear() & CRIGHT) {
                 m_maxWidth = qMax(floatTotalWidth, m_maxWidth);
                 floatRightWidth = 0;
             }
        }

        Length ml = child->style()->marginLeft();
        Length mr = child->style()->marginRight();

        // Call calcWidth on the child to ensure that our margins are
        // up to date.  This method can be called before the child has actually
        // calculated its margins (which are computed inside calcWidth).
        if (ml.isPercent() || mr.isPercent())
            calcWidth();

        // A margin basically has three types: fixed, percentage, and auto (variable).
        // Auto margins simply become 0 when computing min/max width.
        // Fixed margins can be added in as is.
        // Percentage margins are computed as a percentage of the width we calculated in
        // the calcWidth call above.  In this case we use the actual cached margin values on
        // the RenderObject itself.
        int margin = 0, marginLeft = 0, marginRight = 0;
        if (ml.isFixed())
            marginLeft += ml.value();
        else if (ml.isPercent())
            marginLeft += child->marginLeft();

        if (mr.isFixed())
            marginRight += mr.value();
        else if (mr.isPercent())
            marginRight += child->marginRight();

        margin = marginLeft + marginRight;

        int w = child->minWidth() + margin;
        if(m_minWidth < w) m_minWidth = w;

        // IE ignores tables for calculation of nowrap. Makes some sense.
        if ( nowrap && !child->isTable() && m_maxWidth < w )
            m_maxWidth = w;

        w = child->maxWidth() + margin;

         if (!child->isFloating()) {
             if (child->flowAroundFloats()) {
                 // Determine a left and right max value based on whether or not the floats can fit in the
                 // margins of the object.  For negative margins, we will attempt to overlap the float if the negative margin
                 // is smaller than the float width.
                 int maxLeft = marginLeft > 0 ? qMax(floatLeftWidth, marginLeft) : floatLeftWidth + marginLeft;
                 int maxRight = marginRight > 0 ? qMax(floatRightWidth, marginRight) : floatRightWidth + marginRight;
                 w = child->maxWidth() + maxLeft + maxRight;
                 w = qMax(w, floatLeftWidth + floatRightWidth);
             }
             else
                 m_maxWidth = qMax(floatLeftWidth + floatRightWidth, m_maxWidth);
             floatLeftWidth = floatRightWidth = 0;
         }

        if (child->isFloating()) {
            if (style()->floating() & FLEFT)
                floatLeftWidth += w;
            else
                floatRightWidth += w;
        } else if (m_maxWidth < w)
            m_maxWidth = w;

        // A very specific WinIE quirk.
        // Example:
        /*
           <div style="position:absolute; width:100px; top:50px;">
              <div style="position:absolute;left:0px;top:50px;height:50px;background-color:green">
                <table style="width:100%"><tr><td></table>
              </div>
           </div>
        */
        // In the above example, the inner absolute positioned block should have a computed width
        // of 100px because of the table.
        // We can achieve this effect by making the maxwidth of blocks that contain tables
        // with percentage widths be infinite (as long as they are not inside a table cell).
        if (style()->htmlHacks() && child->style()->width().isPercent() &&
            !isTableCell() && child->isTable() && m_maxWidth < BLOCK_MAX_WIDTH) {
            RenderBlock* cb = containingBlock();
            while (!cb->isCanvas() && !cb->isTableCell())
                cb = cb->containingBlock();
            if (!cb->isTableCell())
                m_maxWidth = BLOCK_MAX_WIDTH;
        }
        child = child->nextSibling();
    }
    m_maxWidth = qMax(floatLeftWidth + floatRightWidth, m_maxWidth);
}

void RenderBlock::close()
{
    if (lastChild() && lastChild()->isAnonymousBlock())
        lastChild()->close();
    updateFirstLetter();
    RenderFlow::close();
}

int RenderBlock::getBaselineOfFirstLineBox()
{
    if (m_firstLineBox)
        return m_firstLineBox->yPos() + m_firstLineBox->baseline();

    if (isInline())
        return -1; // We're inline and had no line box, so we have no baseline we can return.

    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
        int result = curr->getBaselineOfFirstLineBox();
        if (result != -1)
            return curr->yPos() + result; // Translate to our coordinate space.
    }

    return -1;
}

InlineFlowBox* RenderBlock::getFirstLineBox()
{
    if (m_firstLineBox)
        return m_firstLineBox;

    if (isInline())
        return 0; // We're inline and had no line box, so we have no baseline we can return.

    for (RenderObject* curr = firstChild(); curr; curr = curr->nextSibling()) {
        InlineFlowBox* result = curr->getFirstLineBox();
        if (result)
            return result;
    }

    return 0;
}

bool RenderBlock::inRootBlockContext() const
{
    if (isTableCell() || isFloatingOrPositioned() || hasOverflowClip())
        return false;

    if (isRoot() || isCanvas())
        return true;

    return containingBlock()->inRootBlockContext();
}

const char *RenderBlock::renderName() const
{
    if (isFloating())
        return "RenderBlock (floating)";
    if (isPositioned())
        return "RenderBlock (positioned)";
    if (isAnonymousBlock() && m_avoidPageBreak)
        return "RenderBlock (avoidPageBreak)";
    if (isAnonymousBlock())
        return "RenderBlock (anonymous)";
    else if (isAnonymous())
        return "RenderBlock (generated)";
    if (isRelPositioned())
        return "RenderBlock (relative positioned)";
    if (style() && style()->display() == COMPACT)
        return "RenderBlock (compact)";
    if (style() && style()->display() == RUN_IN)
        return "RenderBlock (run-in)";
    return "RenderBlock";
}

#ifdef ENABLE_DUMP
void RenderBlock::printTree(int indent) const
{
    RenderFlow::printTree(indent);

    if (m_floatingObjects)
    {
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        FloatingObject *r;
        while ( it.hasNext() )
        {
            r = it.next();
            QString s;
            s.fill(' ', indent);
            qDebug() << s << renderName() << ":  " <<
               (r->type == FloatingObject::FloatLeft ? "FloatLeft" : "FloatRight" )  <<
               "[" << r->node->renderName() << ": " << (void*)r->node << "] (" << r->startY << " - " << r->endY << ")" << "width: " << r->width <<
               endl;
        }
    }
}

void RenderBlock::dump(QTextStream &stream, const QString &ind) const
{
    RenderFlow::dump(stream,ind);

    if (m_childrenInline) { stream << QLatin1String(" childrenInline"); }
    // FIXME: currently only print pre to not mess up regression
    if (style()->preserveWS()) { stream << " pre"; }
    if (m_firstLine) { stream << QLatin1String(" firstLine"); }

    if (m_floatingObjects && !m_floatingObjects->isEmpty())
    {
        stream << QLatin1String(" special(");
        QListIterator<FloatingObject*> it(*m_floatingObjects);
        FloatingObject *r;
        bool first = true;
        while ( it.hasNext() )
        {
            r = it.next();
            if (!first)
                stream << QLatin1Char(',');
            stream << r->node->renderName();
            first = false;
        }
        stream << QLatin1Char(')');
    }

    // ### EClear m_clearStatus
}
#endif

#undef DEBUG
#undef DEBUG_LAYOUT
#undef BOX_DEBUG

} // namespace khtml

