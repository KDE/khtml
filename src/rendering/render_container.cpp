/**
 * This file is part of the html renderer for KDE.
 *
 * Copyright (C) 2001-2003 Lars Knoll (knoll@kde.org)
 *           (C) 2001 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000-2003 Dirk Mueller (mueller@kde.org)
 *           (C) 2002-2007 Apple Computer, Inc.
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
 *
 */

//#define DEBUG_LAYOUT

#include "rendering/render_container.h"
#include "rendering/render_table.h"
#include "rendering/render_text.h"
#include "rendering/render_image.h"
#include "rendering/render_canvas.h"
#include "rendering/render_generated.h"
#include "rendering/render_inline.h"
#include "rendering/render_layer.h"
#include "rendering/render_position.h"
#include "xml/dom_docimpl.h"
#include "xml/dom_position.h"
#include "css/css_valueimpl.h"

#include <QDebug>
#include <assert.h>
#include <limits.h>

using DOM::Position;
using namespace khtml;

RenderContainer::RenderContainer(DOM::NodeImpl* node)
    : RenderObject(node)
{
    m_first = 0;
    m_last = 0;
}

void RenderContainer::addChild(RenderObject *newChild, RenderObject *beforeChild)
{
#ifdef DEBUG_LAYOUT
    // qDebug() << this << ": " <<  renderName() << "(RenderObject)::addChild( " << newChild << ": " <<
        newChild->renderName() << ", " << (beforeChild ? beforeChild->renderName() : "0") << " )" << endl;
#endif
    // protect ourselves from deletion
    setDoNotDelete(true);

    bool needsTable = false;

    if(!newChild->isText() && !newChild->isReplaced()) {
        switch(newChild->style()->display()) {
        case INLINE:
        case BLOCK:
        case LIST_ITEM:
        case RUN_IN:
	case COMPACT:
        case INLINE_BLOCK:
        case TABLE:
        case INLINE_TABLE:
            break;
        case TABLE_COLUMN:
            if ( isTableCol() )
                break;
            // nobreak
        case TABLE_COLUMN_GROUP:
        case TABLE_CAPTION:
        case TABLE_ROW_GROUP:
        case TABLE_HEADER_GROUP:
        case TABLE_FOOTER_GROUP:

            //qDebug() << "adding section";
            if ( !isTable() )
                needsTable = true;
            break;
        case TABLE_ROW:
            //qDebug() << "adding row";
            if ( !isTableSection() )
                needsTable = true;
            break;
        case TABLE_CELL:
            //qDebug() << "adding cell";
            if ( !isTableRow() )
                needsTable = true;
            // I'm not 100% sure this is the best way to fix this, but without this
            // change we recurse infinitely when trying to render the CSS2 test page:
            // http://www.bath.ac.uk/%7Epy8ieh/internet/eviltests/htmlbodyheadrendering2.html.
            if ( isTableCell() && !firstChild() && !newChild->isTableCell() )
                needsTable = false;

            break;
        case NONE:
            // RenderHtml and some others can have display:none
            // KHTMLAssert(false);
            break;
        }
    }

    if ( needsTable ) {
        RenderTable *table;
	RenderObject *last = beforeChild ? beforeChild->previousSibling() : lastChild();
        if ( last && last->isTable() && last->isAnonymous() ) {
            table = static_cast<RenderTable *>(last);
        } else {
	    //qDebug() << "creating anonymous table, before=" << beforeChild;
            table = new (renderArena()) RenderTable(document() /* is anonymous */);
            RenderStyle *newStyle = new RenderStyle();
            newStyle->inheritFrom(style());
	    newStyle->setDisplay( TABLE );
	    newStyle->setFlowAroundFloats( true );
	    table->setParent( this ); // so it finds the arena
            table->setStyle(newStyle);
	    table->setParent( 0 );
            addChild(table, beforeChild);
        }
        table->addChild(newChild);
    } else {
	// just add it...
	insertChildNode(newChild, beforeChild);
    }

    if (newChild->isText() && newChild->style()->textTransform() == CAPITALIZE) {
        DOM::DOMStringImpl* textToTransform =  static_cast<RenderText*>(newChild)->originalString();
        if (textToTransform)
            static_cast<RenderText*>(newChild)->setText(textToTransform, true);
    }
    newChild->attach();
    
    setDoNotDelete(false);
}

RenderObject* RenderContainer::removeChildNode(RenderObject* oldChild)
{
    KHTMLAssert(oldChild->parent() == this);
    bool inCleanup = documentBeingDestroyed();

    if ( !inCleanup ) {
        oldChild->setNeedsLayoutAndMinMaxRecalc(); // Dirty the containing block chain
        oldChild->setNeedsLayout( false ); // The child itself does not need to layout - it's going away.

        // Repaint, so that the area exposed when the child
        // disappears gets repainted properly.
        if (oldChild->height() && oldChild->width())
            oldChild->repaint();
    }

    // detach the place holder box
    if (oldChild->isBox()) {
        RenderBox* rb = static_cast<RenderBox*>(oldChild);
        InlineBox* ph = rb->placeHolderBox();
        if (ph) {
            ph->detach(rb->renderArena(), inCleanup /*NoRemove*/);
            rb->setPlaceHolderBox( 0 );
        }
    }

    if ( !inCleanup ) {
        // if we remove visible child from an invisible parent, we don't know the layer visibility any more
        RenderLayer* layer = 0;
        if (m_style->visibility() != VISIBLE && oldChild->style()->visibility() == VISIBLE && !oldChild->layer()) {
            layer = enclosingLayer();
            if (layer) {
                layer->dirtyVisibleContentStatus();
            }
        }

         // Keep our layer hierarchy updated.
        if (oldChild->firstChild() || oldChild->layer()) {
            if (!layer) {
                layer = enclosingLayer();
            }
            oldChild->removeLayers(layer);
        }
        // remove the child from any special layout lists
        oldChild->removeFromObjectLists();

        // keep our fixed object lists updated.
        if (oldChild->style()->hasFixedBackgroundImage() || oldChild->style()->position() == PFIXED) {
            if (oldChild->style()->hasFixedBackgroundImage())
                canvas()->removeStaticObject( oldChild );
            if (oldChild->style()->position() == PFIXED)
                canvas()->removeStaticObject( oldChild, true );
        }

        if (oldChild->isPosWithStaticDim() && childrenInline())
            dirtyLinesFromChangedChild(oldChild);

        // We are about to take out node from the rendering tree and therefore
        // it's possible that we're modifying the line box tree too.
        // In order to properly recalculate it later we need
        // to delete all the boxes from the current flow of the removed child. (vtokarev)
        // In particular that's relevant when we're merging split inline flow. (continuations)
        // We're taking the render objects from one block and insert into another
        // so we have to force line box tree recalculation
        if (oldChild->isInline()) {
            if (oldChild->isText()) {
                InlineTextBox* box = static_cast<RenderText*>(oldChild)->firstTextBox();
                InlineTextBox* nextTextBox;
                assert(!box || box->parent());
                // delete all the text boxes
                for (; box; box = nextTextBox) {
                    nextTextBox = box->nextTextBox();
                    box->remove();
                    box->deleteLine(renderArena());
                }
            } else if (oldChild->isInlineFlow()) {
                InlineFlowBox* box = static_cast<RenderFlow*>(oldChild)->firstLineBox();
                InlineFlowBox* nextFlowBox;
                assert(!box || box->parent());
                // delete all the flow
                for (; box; box = nextFlowBox) {
                    nextFlowBox = box->nextFlowBox();
                    box->remove();
                    box->deleteLine(renderArena());
                }
            }
        }

        // if oldChild is the start or end of the selection, then clear
        // the selection to avoid problems of invalid pointers

        // ### This is not the "proper" solution... ideally the selection
        // ### should be maintained based on DOM Nodes and a Range, which
        // ### gets adjusted appropriately when nodes are deleted/inserted
        // ### near etc. But this at least prevents crashes caused when
        // ### the start or end of the selection is deleted and then
        // ### accessed when the user next selects something.

        if (oldChild->isSelectionBorder())
            canvas()->clearSelection();
    }

    // remove the child from the render-tree
    if (oldChild->previousSibling())
        oldChild->previousSibling()->setNextSibling(oldChild->nextSibling());
    if (oldChild->nextSibling())
        oldChild->nextSibling()->setPreviousSibling(oldChild->previousSibling());

    if (m_first == oldChild)
        m_first = oldChild->nextSibling();
    if (m_last == oldChild)
        m_last = oldChild->previousSibling();

    oldChild->setPreviousSibling(0);
    oldChild->setNextSibling(0);
    oldChild->setParent(0);

    return oldChild;
}

void RenderContainer::setStyle(RenderStyle* _style)
{
    RenderObject::setStyle(_style);

    // If we are a pseudo-container we need to restyle the children
    if (style()->isGenerated())
    {
        // ### we could save this call when the change only affected
        // non inherited properties
        RenderStyle *pseudoStyle = new RenderStyle();
        pseudoStyle->inheritFrom(style());
        pseudoStyle->ref();
        RenderObject *child = firstChild();
        while (child != 0)
        {
            child->setStyle(pseudoStyle);
            child = child->nextSibling();
        }
        pseudoStyle->deref();
    }
}

static bool inUpdatePseudoChildren = false;

void RenderContainer::updatePseudoChildren()
{
    if (inUpdatePseudoChildren)
        return;
    inUpdatePseudoChildren = true;

    // In CSS2, before/after pseudo-content cannot nest.  Check this first.
    // Remove when CSS 3 Generated Content becomes Candidate Recommendation
    if (style()->styleType() == RenderStyle::BEFORE
     || style()->styleType() == RenderStyle::AFTER)
        return;

    updatePseudoChild(RenderStyle::BEFORE);
    updatePseudoChild(RenderStyle::AFTER);
    // updatePseudoChild(RenderStyle::MARKER, marker());

    inUpdatePseudoChildren = false;
}

void RenderContainer::updatePseudoChild(RenderStyle::PseudoId type)
{
    // The head manages generated content for its continuations
    if (isInlineContinuation()) return;

    RenderStyle* pseudo = style()->getPseudoStyle(type);

    RenderObject* child = pseudoContainer(type);

    // Whether or not we currently have generated content attached.
    bool oldContentPresent = child && (child->style()->styleType() == type);

    // Whether or not we now want generated content.
    bool newContentWanted = pseudo && pseudo->display() != NONE;

    // No generated content
    if (!oldContentPresent && !newContentWanted)
        return;

    bool movedContent = (type == RenderStyle::AFTER && isRenderInline() && continuation());

    // Whether or not we want the same old content.
    bool sameOldContent = oldContentPresent && newContentWanted && !movedContent
                       && (child->style()->contentDataEquivalent(pseudo));

    // No change in content, update style
    if( sameOldContent ) {
        child->setStyle(pseudo);
        return;
    }

    // If we don't want generated content any longer, or if we have generated content,
    // but it's no longer identical to the new content data we want to build
    // render objects for, then we nuke all of the old generated content.
    if (oldContentPresent && (!newContentWanted || !sameOldContent))
    {
        // The child needs to be removed.
        oldContentPresent = false;
        child->detach();
        child = 0;
    }

    // If we have no pseudo-style or if the pseudo's display type is NONE, then we
    // have no generated content and can now return.
    if (!newContentWanted)
        return;

    // Generated content consists of a single container that houses multiple children (specified
    // by the content property).  This pseudo container gets the pseudo style set on it.
    RenderContainer* pseudoContainer = 0;
    pseudoContainer = RenderFlow::createFlow(element(), pseudo, renderArena());
    pseudoContainer->setIsAnonymous( true );
    pseudoContainer->createGeneratedContent();

    // Only add the container if it had content
    if (pseudoContainer->firstChild()) {
        addPseudoContainer(pseudoContainer);
        pseudoContainer->close();
    }
}

void RenderContainer::createGeneratedContent()
{
    RenderStyle* pseudo = style();
    RenderStyle* style = new RenderStyle();
    style->ref();
    style->inheritFrom(pseudo);

    // Now walk our list of generated content and create render objects for every type
    // we encounter.
    for (ContentData* contentData = pseudo->contentData();
         contentData; contentData = contentData->_nextContent)
    {
        if (contentData->_contentType == CONTENT_TEXT)
        {
            RenderText* t = new (renderArena()) RenderText( node(), 0);
            t->setIsAnonymous( true );
            t->setStyle(style);
            t->setText(contentData->contentText());
            addChild(t);
        }
        else if (contentData->_contentType == CONTENT_OBJECT)
        {
            RenderImage* img = new (renderArena()) RenderImage(node());
            img->setIsAnonymous( true );
            img->setStyle(style);
            img->setContentObject(contentData->contentObject());
            addChild(img);
        }
        else if (contentData->_contentType == CONTENT_COUNTER)
        {
            // really a counter or just a glyph?
            EListStyleType type = (EListStyleType)contentData->contentCounter()->listStyle();
            RenderObject *t = 0;
            if (isListStyleCounted(type)) {
                t = new (renderArena()) RenderCounter( node(), contentData->contentCounter() );
            }
            else {
                t = new  (renderArena()) RenderGlyph( node(), type );
            }
            t->setIsAnonymous( true );
            t->setStyle(style);
            addChild(t);
        }
        else if (contentData->_contentType == CONTENT_QUOTE)
        {
            RenderQuote* t = new (renderArena()) RenderQuote( node(), contentData->contentQuote() );
            t->setIsAnonymous( true );
            t->setStyle(style);
            addChild(t);
        }
    }
    style->deref();
}

RenderContainer* RenderContainer::pseudoContainer(RenderStyle::PseudoId type) const
{
    RenderObject *child = 0;
    switch (type) {
        case RenderStyle::AFTER:
            child = lastChild();
            break;
        case RenderStyle::BEFORE:
            child = firstChild();
            break;
        case RenderStyle::REPLACED:
            child = lastChild();
            if (child && child->style()->styleType() == RenderStyle::AFTER)
                child = child->previousSibling();
            break;
        default:
            child = 0;
    }

    if (child && child->style()->styleType() == type) {
        assert(child->isRenderBlock() || child->isRenderInline());
        return static_cast<RenderContainer*>(child);
    }
    if (type == RenderStyle::AFTER) {
        // check continuations
        if (continuation())
            return continuation()->pseudoContainer(type);
    }
    if (child && child->isAnonymousBlock())
        return static_cast<RenderBlock*>(child)->pseudoContainer(type);
    return 0;
}

void RenderContainer::addPseudoContainer(RenderObject* child)
{
    RenderStyle::PseudoId type = child->style()->styleType();
    switch (type) {
        case RenderStyle::AFTER: {
            RenderObject *o = this;
            while (o->continuation()) o = o->continuation();

            // Coalesce inlines
            if (child->style()->display() == INLINE && o->lastChild() && o->lastChild()->isAnonymousBlock()) {
                o->lastChild()->addChild(child, 0);
            } else
                o->addChild(child, 0);
            break;
        }
        case RenderStyle::BEFORE:
            // Coalesce inlines
            if (child->style()->display() == INLINE && firstChild() && firstChild()->isAnonymousBlock()) {
                firstChild()->addChild(child, firstChild()->firstChild());
            } else
                addChild(child, firstChild());
            break;
        case RenderStyle::REPLACED:
            addChild(child, pseudoContainer(RenderStyle::AFTER));
            break;
        default:
            break;
    }
}

void RenderContainer::updateReplacedContent()
{
    // Only for normal elements
    if (!style() || style()->styleType() != RenderStyle::NOPSEUDO)
        return;

    // delete old generated content
    RenderContainer *container = pseudoContainer(RenderStyle::REPLACED);
    if (container) {
        container->detach();
    }

    if (style()->useNormalContent()) return;

    // create generated content
    RenderStyle* pseudo = style()->getPseudoStyle(RenderStyle::REPLACED);
    if (!pseudo) {
        pseudo = new RenderStyle();
        pseudo->inheritFrom(style());
        pseudo->setStyleType(RenderStyle::REPLACED);
    }
    if (pseudo->useNormalContent())
        pseudo->setContentData(style()->contentData());

    container = RenderFlow::createFlow(node(), pseudo, renderArena());
    container->setIsAnonymous( true );
    container->createGeneratedContent();

    addChild(container, pseudoContainer(RenderStyle::AFTER));
}

void RenderContainer::appendChildNode(RenderObject* newChild)
{
    KHTMLAssert(newChild->parent() == 0);

    newChild->setParent(this);
    RenderObject* lChild = lastChild();

    if(lChild)
    {
        newChild->setPreviousSibling(lChild);
        lChild->setNextSibling(newChild);
    }
    else
        setFirstChild(newChild);

    setLastChild(newChild);

    // Keep our layer hierarchy updated.  Optimize for the common case where we don't have any children
    // and don't have a layer attached to ourselves.
    RenderLayer* layer = 0;
    if (newChild->firstChild() || newChild->layer()) {
        layer = enclosingLayer();
        newChild->addLayers(layer, newChild);
        // keep our fixed object lists updated.
        if (newChild->style()&& (newChild->style()->hasFixedBackgroundImage() || newChild->style()->position() == PFIXED)) {
            if (newChild->style()->hasFixedBackgroundImage())
                canvas()->addStaticObject( newChild );
            if (newChild->style()->position() == PFIXED)
                canvas()->addStaticObject( newChild, true );
        }
    }

    // if the new child is visible but this object was not, tell the layer it has some visible content
    // that needs to be drawn and layer visibility optimization can't be used
    if (style()->visibility() != VISIBLE && newChild->style()->visibility() == VISIBLE && !newChild->layer()) {
        if (!layer) {
            layer = enclosingLayer();
        }
        if (layer) {
            layer->setHasVisibleContent(true);
        }
    }

    if (!newChild->isFloatingOrPositioned() && childrenInline())
        dirtyLinesFromChangedChild(newChild);

    newChild->setNeedsLayoutAndMinMaxRecalc(); // Goes up the containing block hierarchy.

    if (!normalChildNeedsLayout()) {
        // We may supply the static position for an absolute positioned child.
        if (newChild->firstChild() || newChild->isPosWithStaticDim() || !newChild->isPositioned())
            setChildNeedsLayout(true);
        else {
            assert(!newChild->inPosObjectList());
            newChild->containingBlock()->insertPositionedObject(newChild);
        }
    }
}

void RenderContainer::insertChildNode(RenderObject* child, RenderObject* beforeChild)
{
    if(!beforeChild) {
        appendChildNode(child);
        return;
    }

    KHTMLAssert(!child->parent());
    while ( beforeChild->parent() != this && beforeChild->parent()->isAnonymous() )
	beforeChild = beforeChild->parent();
    KHTMLAssert(beforeChild->parent() == this);

    if(beforeChild == firstChild())
        setFirstChild(child);

    RenderObject* prev = beforeChild->previousSibling();
    child->setNextSibling(beforeChild);
    beforeChild->setPreviousSibling(child);
    if(prev) prev->setNextSibling(child);
    child->setPreviousSibling(prev);
    child->setParent(this);

    // Keep our layer hierarchy updated.  Optimize for the common case where we don't have any children
    // and don't have a layer attached to ourselves.
    RenderLayer* layer = 0;
    if (child->firstChild() || child->layer()) {
        layer = enclosingLayer();
        child->addLayers(layer, child);
        // keep our fixed object lists updated.
        if (child->style() && (child->style()->hasFixedBackgroundImage() || child->style()->position() == PFIXED)) {
            if (child->style()->hasFixedBackgroundImage())
                canvas()->addStaticObject( child );
            if (child->style()->position() == PFIXED)
                canvas()->addStaticObject( child, true );
        }

    }

    // if the new child is visible but this object was not, tell the layer it has some visible content
    // that needs to be drawn and layer visibility optimization can't be used
    if (style()->visibility() != VISIBLE && child->style()->visibility() == VISIBLE && !child->layer()) {
        if (!layer) {
            layer = enclosingLayer();
        }
        if (layer) {
            layer->setHasVisibleContent(true);
        }
    }

    if (!child->isFloating() && childrenInline())
        dirtyLinesFromChangedChild(child);

    child->setNeedsLayoutAndMinMaxRecalc();

    if (!normalChildNeedsLayout()) {
        // We may supply the static position for an absolute positioned child.
        if (child->firstChild() || child->isPosWithStaticDim() || !child->isPositioned())
            setChildNeedsLayout(true);
        else {
            assert(!child->inPosObjectList());
            child->containingBlock()->insertPositionedObject(child);
        }
    }
}


void RenderContainer::layout()
{
    KHTMLAssert( needsLayout() );
    KHTMLAssert( minMaxKnown() );
    const bool pagedMode = canvas()->pagedMode();
    RenderObject *child = firstChild();
    while( child ) {
        if (pagedMode) child->setNeedsLayout(true);
        child->layoutIfNeeded();
        if (child->containsPageBreak()) setContainsPageBreak(true);
        if (child->needsPageClear()) setNeedsPageClear(true);
        child = child->nextSibling();
    }
    setNeedsLayout(false);
}

void RenderContainer::removeSuperfluousAnonymousBlockChild( RenderObject* child )
{
    KHTMLAssert( child->parent() == this && child->isAnonymousBlock() );

    if (child->doNotDelete() || child->continuation())
        return;

    RenderObject *childSFirstChild = child->firstChild();
    RenderObject *childSLastChild = child->lastChild();

    if (childSFirstChild) {
        RenderObject *o = childSFirstChild;
        while( o ) {
            o->setParent( this );
            o = o->nextSibling();
        }
        childSFirstChild->setPreviousSibling( child->previousSibling() );
        childSLastChild->setNextSibling( child->nextSibling() );
        if ( child->previousSibling() )
	    child->previousSibling()->setNextSibling( childSFirstChild );
        if ( child->nextSibling() )
	    child->nextSibling()->setPreviousSibling( childSLastChild );
        if ( child == firstChild() )
	    m_first = childSFirstChild;
        if ( child == lastChild() )
	    m_last = childSLastChild;
    } else {
        if ( child->previousSibling() )
	    child->previousSibling()->setNextSibling( child->nextSibling() );
        if ( child->nextSibling() )
	    child->nextSibling()->setPreviousSibling( child->previousSibling() );
        if ( child == firstChild() )
            m_first = child->nextSibling();
        if ( child == lastChild() )
            m_last = child->previousSibling();
    }
    child->setParent( 0 );
    child->setPreviousSibling( 0 );
    child->setNextSibling( 0 );
    if ( !child->isText() ) {
        RenderContainer *c = static_cast<RenderContainer *>(child);
	c->m_first = 0;
	c->m_next = 0;
    }
    child->detach();
}

RenderPosition RenderContainer::positionForCoordinates(int _x, int _y)
{
    // no children...return this render object's element, if there is one, and offset 0
    if (!firstChild())
        return RenderPosition(element(), 0);
  
    // look for the geometrically-closest child and pass off to that child
    int min = INT_MAX;
    RenderObject *closestRenderer = firstChild();
    for (RenderObject *renderer = firstChild(); renderer; renderer = renderer->nextSibling()) {
        int absx, absy;
        renderer->absolutePosition(absx, absy);
            
        int top = absy + borderTop() + paddingTop();
        int bottom = top + renderer->contentHeight();
        int left = absx + borderLeft() + paddingLeft();
        int right = left + renderer->contentWidth();
            
        int cmp;
        cmp = abs(_y - top);    if (cmp < min) { closestRenderer = renderer; min = cmp; }
        cmp = abs(_y - bottom); if (cmp < min) { closestRenderer = renderer; min = cmp; }
        cmp = abs(_x - left);   if (cmp < min) { closestRenderer = renderer; min = cmp; }
        cmp = abs(_x - right);  if (cmp < min) { closestRenderer = renderer; min = cmp; }
    }

    return closestRenderer->positionForCoordinates(_x, _y);
}
    
#undef DEBUG_LAYOUT
