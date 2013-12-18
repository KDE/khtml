/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999-2003 Lars Knoll (knoll@kde.org)
 *           (C) 2001-2003 Dirk Mueller (mueller@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2002-2003 Apple Computer, Inc.
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

#include "dom_textimpl.h"
#include "dom2_eventsimpl.h"
#include "dom_docimpl.h"
#include <dom/dom_exception.h>
#include <css/cssstyleselector.h>

#include <rendering/render_text.h>
#include <rendering/render_flow.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

#include <QDebug>

// for SVG
#include <rendering/RenderSVGInlineText.h>

using namespace DOM;
using namespace khtml;

static DOMString escapeHTML( const DOMString& in )
{
    return in.implementation()->escapeHTML();
}

CharacterDataImpl::CharacterDataImpl(DocumentImpl *doc, DOMStringImpl* _text)
    : NodeImpl(doc)
{
    str = _text ? _text : new DOMStringImpl((QChar*)0, 0);
    str->ref();
}

CharacterDataImpl::~CharacterDataImpl()
{
    if(str) str->deref();
}

void CharacterDataImpl::setData( const DOMString &_data, int &exceptioncode )
{
    // NO_MODIFICATION_ALLOWED_ERR: Raised when the node is readonly
    if (isReadOnly()) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    if(str == _data.impl) return; // ### fire DOMCharacterDataModified if modified?
    DOMStringImpl *oldStr = str;
    str = _data.impl;
    if (!str)
        str = new DOMStringImpl((QChar*)0, 0);
    str->ref();
    if (m_render)
      (static_cast<RenderText*>(m_render))->setText(str);
    setChanged(true);

    dispatchModifiedEvent(oldStr);
    if(oldStr) oldStr->deref();
}

unsigned long CharacterDataImpl::length() const
{
    return str->l;
}

DOMString CharacterDataImpl::substringData( const unsigned long offset, const unsigned long count, int &exceptioncode )
{
    exceptioncode = 0;
    if ((long)count < 0)
	exceptioncode = DOMException::INDEX_SIZE_ERR;
    else
	checkCharDataOperation(offset, exceptioncode);
    if (exceptioncode)
        return DOMString();

    return str->substring(offset,count);
}

void CharacterDataImpl::appendData( const DOMString &arg, int &exceptioncode )
{
    exceptioncode = 0;

    // NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly
    if (isReadOnly()) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    DOMStringImpl *oldStr = str;
    str = str->copy();
    str->ref();
    str->append(arg.impl);
    if (m_render)
      (static_cast<RenderText*>(m_render))->setText(str);
    setChanged(true);

    dispatchModifiedEvent(oldStr);
    oldStr->deref();
}

void CharacterDataImpl::insertData( const unsigned long offset, const DOMString &arg, int &exceptioncode )
{
    exceptioncode = 0;
    checkCharDataOperation(offset, exceptioncode);
    if (exceptioncode)
        return;

    DOMStringImpl *oldStr = str;
    str = str->copy();
    str->ref();
    str->insert(arg.impl, offset);
    if (m_render)
      (static_cast<RenderText*>(m_render))->setText(str);
    setChanged(true);

    dispatchModifiedEvent(oldStr);
    oldStr->deref();
}

void CharacterDataImpl::deleteData( const unsigned long offset, const unsigned long count, int &exceptioncode )
{
    exceptioncode = 0;
    if ((long)count < 0)
	exceptioncode = DOMException::INDEX_SIZE_ERR;
    else
	checkCharDataOperation(offset, exceptioncode);
    if (exceptioncode)
        return;

    DOMStringImpl *oldStr = str;
    str = str->copy();
    str->ref();
    str->remove(offset,count);
    if (m_render)
      (static_cast<RenderText*>(m_render))->setText(str);
    setChanged(true);

    dispatchModifiedEvent(oldStr);
    oldStr->deref();
}

void CharacterDataImpl::replaceData( const unsigned long offset, const unsigned long count, const DOMString &arg, int &exceptioncode )
{
    exceptioncode = 0;
    if ((long)count < 0)
	exceptioncode = DOMException::INDEX_SIZE_ERR;
    else
	checkCharDataOperation(offset, exceptioncode);
    if (exceptioncode)
        return;

    unsigned long realCount;
    if (offset + count > str->l)
        realCount = str->l-offset;
    else
        realCount = count;

    DOMStringImpl *oldStr = str;
    str = str->copy();
    str->ref();
    str->remove(offset,realCount);
    str->insert(arg.impl, offset);
    if (m_render)
      (static_cast<RenderText*>(m_render))->setText(str);
    setChanged(true);

    dispatchModifiedEvent(oldStr);
    oldStr->deref();
}

DOMString CharacterDataImpl::nodeValue() const
{
    return str;
}

bool CharacterDataImpl::containsOnlyWhitespace() const
{
    return str->containsOnlyWhitespace();
}

void CharacterDataImpl::setNodeValue( const DOMString &_nodeValue, int &exceptioncode )
{
    // NO_MODIFICATION_ALLOWED_ERR: taken care of by setData()
    setData(_nodeValue, exceptioncode);
}

void CharacterDataImpl::dispatchModifiedEvent(DOMStringImpl *prevValue)
{
    if (parentNode())
        parentNode()->childrenChanged();
    if ((str->length() == 0) != (prevValue->length() == 0)) {
       // changes in emptiness triggers changes in :empty selector
       if (parentNode() && parentNode()->isElementNode())
           parentNode()->backwardsStructureChanged();
       // ### to fully support dynamic changes to :contains selector
       // backwardsStructureChanged should be called for all changes
    }
    if (!document()->hasListenerType(DocumentImpl::DOMCHARACTERDATAMODIFIED_LISTENER))
        return;

    DOMStringImpl *newValue = str->copy();
    newValue->ref();
    int exceptioncode = 0;
    MutationEventImpl* const evt = new MutationEventImpl(EventImpl::DOMCHARACTERDATAMODIFIED_EVENT,true,false,0,prevValue,newValue,DOMString(),0);
    evt->ref();
    dispatchEvent(evt,exceptioncode);
    evt->deref();
    newValue->deref();
    dispatchSubtreeModifiedEvent();
}

void CharacterDataImpl::checkCharDataOperation( const unsigned long offset, int &exceptioncode )
{
    exceptioncode = 0;

    // INDEX_SIZE_ERR: Raised if the specified offset is negative or greater than the number of 16-bit
    // units in data.
    if (offset > str->l) {
        exceptioncode = DOMException::INDEX_SIZE_ERR;
        return;
    }

    // NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly
    if (isReadOnly()) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
}

long CharacterDataImpl::maxOffset() const
{
  RenderText *r = static_cast<RenderText *>(renderer());
  if (!r || !r->isText()) return 0;
    return (long)length();
}

long CharacterDataImpl::caretMinOffset() const
{
    RenderText *r = static_cast<RenderText *>(renderer());
    return r && r->isText() ? r->convertToDOMPosition(r->caretMinOffset()) : 0;
}

long CharacterDataImpl::caretMaxOffset() const
{
    RenderText *r = static_cast<RenderText *>(renderer());
    return r && r->isText() ? r->convertToDOMPosition(r->caretMaxOffset()) : (long)length();
}

unsigned long CharacterDataImpl::caretMaxRenderedOffset() const
{
    RenderText *r = static_cast<RenderText *>(renderer());
    return r ? r->caretMaxRenderedOffset() : length();
}


bool CharacterDataImpl::rendererIsNeeded(khtml::RenderStyle *style)
{
    if (!str || !str->l)
        return false;
    return NodeImpl::rendererIsNeeded(style);
}

// ---------------------------------------------------------------------------

DOMString CommentImpl::nodeName() const
{
    return "#comment";
}

unsigned short CommentImpl::nodeType() const
{
    return Node::COMMENT_NODE;
}

WTF::PassRefPtr<NodeImpl> CommentImpl::cloneNode(bool /*deep*/)
{
    return document()->createComment( str );
}

NodeImpl::Id CommentImpl::id() const
{
    return ID_COMMENT;
}

// DOM Section 1.1.1
bool CommentImpl::childTypeAllowed( unsigned short /*type*/ )
{
    return false;
}

DOMString CommentImpl::toString() const
{
    // FIXME: substitute entity references as needed!
    return DOMString("<!--") + nodeValue() + DOMString("-->");
}

// ---------------------------------------------------------------------------

TextImpl *TextImpl::splitText( const unsigned long offset, int &exceptioncode )
{
    exceptioncode = 0;

    // INDEX_SIZE_ERR: Raised if the specified offset is negative or greater than
    // the number of 16-bit units in data.

    // ### we explicitly check for a negative long that has been cast to an unsigned long
    // ... this can happen if JS code passes in -1 - we need to catch this earlier! (in the
    // kjs bindings)
    if (offset > str->l || (long)offset < 0) {
        exceptioncode = DOMException::INDEX_SIZE_ERR;
        return 0;
    }

    // NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
    if (isReadOnly()) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return 0;
    }

    DOMStringImpl *oldStr = str;
    TextImpl *newText = createNew(str->substring(offset,str->l-offset));
    str = str->copy();
    str->ref();
    str->remove(offset,str->l-offset);

    dispatchModifiedEvent(oldStr);
    oldStr->deref();

    if (parentNode())
        parentNode()->insertBefore(newText,nextSibling(), exceptioncode );
    if ( exceptioncode )
        return 0;

    if (m_render)
        (static_cast<RenderText*>(m_render))->setText(str);
    setChanged(true);
    return newText;
}

static const TextImpl* earliestLogicallyAdjacentTextNode(const TextImpl* t)
{
    const NodeImpl* n = t;
    while ((n = n->previousSibling())) {
        unsigned short type = n->nodeType();
        if (type == Node::TEXT_NODE || type == Node::CDATA_SECTION_NODE) {
            t = static_cast<const TextImpl*>(n);
            continue;
        }

        // We would need to visit EntityReference child text nodes if they existed
        assert(type != Node::ENTITY_REFERENCE_NODE || !n->hasChildNodes());
        break;
    }
    return t;
}

static const TextImpl* latestLogicallyAdjacentTextNode(const TextImpl* t)
{
    const NodeImpl* n = t;
    while ((n = n->nextSibling())) {
        unsigned short type = n->nodeType();
        if (type == Node::TEXT_NODE || type == Node::CDATA_SECTION_NODE) {
            t = static_cast<const TextImpl*>(n);
            continue;
        }

        // We would need to visit EntityReference child text nodes if they existed
        assert(type != Node::ENTITY_REFERENCE_NODE || !n->hasChildNodes());
        break;
    }
    return t;
}

DOMString TextImpl::wholeText() const
{
    const TextImpl* startText = earliestLogicallyAdjacentTextNode(this);
    const TextImpl* endText = latestLogicallyAdjacentTextNode(this);

    DOMString result;
    NodeImpl* onePastEndText = endText->nextSibling();
    for (const NodeImpl* n = startText; n != onePastEndText; n = n->nextSibling()) {
        if (!n->isTextNode())
            continue;
        const TextImpl* t = static_cast<const TextImpl*>(n);
        const DOMString& data = t->data();
        result += data;
    }

    return result;
}

TextImpl* TextImpl::replaceWholeText(const DOMString& newText, int &ec)
{
    Q_UNUSED(ec);
    // We don't support "read-only" text nodes (no Entity node support)
    // Thus, we remove all adjacent text nodes, and replace the contents of this one.
    assert(!isReadOnly());
    // This method only raises exceptions when dealing with Entity nodes (which we don't support)

    // Protect startText and endText against mutation event handlers removing the last ref
    RefPtr<TextImpl> startText = const_cast<TextImpl*>(earliestLogicallyAdjacentTextNode(this));
    RefPtr<TextImpl> endText = const_cast<TextImpl*>(latestLogicallyAdjacentTextNode(this));

    RefPtr<TextImpl> protectedThis(this); // Mutation event handlers could cause our last ref to go away
    NodeImpl* parent = parentNode(); // Protect against mutation handlers moving this node during traversal
    int ignored = 0;
    for (RefPtr<NodeImpl> n = startText; n && n != this && n->isTextNode() && n->parentNode() == parent;) {
        RefPtr<NodeImpl> nodeToRemove(n.release());
        n = nodeToRemove->nextSibling();
        parent->removeChild(nodeToRemove.get(), ignored);
    }

    if (this != endText) {
        NodeImpl* onePastEndText = endText->nextSibling();
        for (RefPtr<NodeImpl> n = nextSibling(); n && n != onePastEndText && n->isTextNode() && n->parentNode() == parent;) {
            RefPtr<NodeImpl> nodeToRemove(n.release());
            n = nodeToRemove->nextSibling();
            parent->removeChild(nodeToRemove.get(), ignored);
        }
    }

    if (newText.isEmpty()) {
        if (parent && parentNode() == parent)
            parent->removeChild(this, ignored);
        return 0;
    }

    setData(newText, ignored);
    return protectedThis.release().get();
}

DOMString TextImpl::nodeName() const
{
  return "#text";
}

unsigned short TextImpl::nodeType() const
{
    return Node::TEXT_NODE;
}

WTF::PassRefPtr<NodeImpl> TextImpl::cloneNode(bool /*deep*/)
{
    return document()->createTextNode(str);
}

bool TextImpl::rendererIsNeeded(RenderStyle *style)
{
    if (!CharacterDataImpl::rendererIsNeeded(style)) {
        return false;
    }
    bool onlyWS = containsOnlyWhitespace();
    if (!onlyWS) {
        return true;
    }

    RenderObject *par = parentNode()->renderer();

    if (par->isTable() || par->isTableRow() || par->isTableSection()) {
        return false;
    }

    if (style->preserveWS() || style->preserveLF()) {
        return true;
    }

    RenderObject *prev = previousRenderer();
    if (par->isInlineFlow()) {
        // <span><div/> <div/></span>
        if (prev && !prev->isInline()) {
            return false;
        }
    } else {
        if (par->isRenderBlock() && !par->childrenInline() && (!prev || !prev->isInline())) {
            return false;
        }

        RenderObject *first = par->firstChild();
        while (first && first->isFloatingOrPositioned())
            first = first->nextSibling();
        RenderObject *next = nextRenderer();
        if (!first || next == first) {
            // Whitespace at the start of a block just goes away.  Don't even
            // make a render object for this text.
            return false;
        }
    }

    return true;
}

RenderObject *TextImpl::createRenderer(RenderArena *arena, RenderStyle * /*style*/)
{
    // for SVG
    if (parentNode()->isSVGElement())
        return new (arena) WebCore::RenderSVGInlineText(this, str);
    return new (arena) RenderText(this, str);
}

void TextImpl::attach()
{
    createRendererIfNeeded();
    CharacterDataImpl::attach();
}

NodeImpl::Id TextImpl::id() const
{
    return ID_TEXT;
}

void TextImpl::recalcStyle( StyleChange change )
{
//      qDebug("textImpl::recalcStyle");
    // Create renderer if now needed
    if ( changed() && !m_render) {
        createRendererIfNeeded();
    }
    if (change != NoChange && parentNode()) {
// 	qDebug("DomText::recalcStyle");
	if(m_render)
	    m_render->setStyle(parentNode()->renderer()->style());
    }
    if ( changed() && m_render && m_render->isText() )
	static_cast<RenderText*>(m_render)->setText(str);
    setChanged( false );
}

// DOM Section 1.1.1
bool TextImpl::childTypeAllowed( unsigned short /*type*/ )
{
    return false;
}

TextImpl *TextImpl::createNew(DOMStringImpl *_str)
{
    return new TextImpl(docPtr(),_str);
}

DOMStringImpl* TextImpl::renderString() const
{
    if (renderer())
        return static_cast<RenderText*>(renderer())->string();
    else
        return string();
}

static bool textNeedsEscaping( const NodeImpl *n )
{
    // Exceptions based on "Serializing HTML fragments" section of
    // HTML 5 specification (with some adaptions to reality)
    const NodeImpl *p = n->parentNode();
    if ( !p )
        return true;
    switch ( p->id() ) {
    case ID_IFRAME:
        // follow deviating examples of FF 3.5.6 and Opera 9.6
        // case ID_NOEMBED:
        // case ID_NOFRAMES:
    case ID_NOSCRIPT:
    case ID_PLAINTEXT:
    case ID_SCRIPT:
    case ID_STYLE:
    case ID_XMP:
        return false;
    default:
        return true;
    }
}

DOMString TextImpl::toString() const
{
    return textNeedsEscaping( this ) ? escapeHTML( nodeValue() ) : nodeValue();
}

DOMString TextImpl::toString(long long startOffset, long long endOffset) const
{
    DOMString str = nodeValue();
    if(endOffset >=0 || startOffset >0)
	str = str.copy(); //we are going to modify this, so make a copy.  I hope I'm doing this right.
    if(endOffset >= 0)
        str.truncate(endOffset);
    if(startOffset > 0)    //note the order of these 2 'if' statements so that it works right when n==m_startContainer==m_endContainer
        str.remove(0, startOffset);
    return textNeedsEscaping( this ) ? escapeHTML( str ) : str;
}

// ---------------------------------------------------------------------------

DOMString CDATASectionImpl::nodeName() const
{
  return "#cdata-section";
}

unsigned short CDATASectionImpl::nodeType() const
{
    return Node::CDATA_SECTION_NODE;
}

WTF::PassRefPtr<NodeImpl> CDATASectionImpl::cloneNode(bool /*deep*/)
{
    return document()->createCDATASection(str);
}

// DOM Section 1.1.1
bool CDATASectionImpl::childTypeAllowed( unsigned short /*type*/ )
{
    return false;
}

TextImpl *CDATASectionImpl::createNew(DOMStringImpl *_str)
{
    return new CDATASectionImpl(docPtr(),_str);
}

DOMString CDATASectionImpl::toString() const
{
    return DOMString("<![CDATA[") + nodeValue() + DOMString("]]>");
}

// ---------------------------------------------------------------------------

EditingTextImpl::EditingTextImpl(DocumentImpl *impl, const DOMString &text)
    : TextImpl(impl, text.implementation())
{
}

EditingTextImpl::EditingTextImpl(DocumentImpl *impl)
    : TextImpl(impl)
{
}

EditingTextImpl::~EditingTextImpl()
{
}

bool EditingTextImpl::rendererIsNeeded(RenderStyle */*style*/)
{
    return true;
}

// kate: indent-width 4; replace-tabs on; tab-width 8; space-indent on;
