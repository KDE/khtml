/**
 * This file is part of the DOM implementation for KDE.
 *
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2001-2003 Dirk Mueller (mueller@kde.org)
 * (C) 2000 Gunnstein Lye (gunnstein@netcom.no)
 * (C) 2000 Frederik Holljen (frederik.holljen@hig.no)
 * (C) 2001 Peter Kelly (pmk@post.com)
 * Copyright (C) 2003-2008 Apple Computer, Inc.
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

#include "dom2_rangeimpl.h"

#include <dom/dom_exception.h>
#include "dom_docimpl.h"
#include "dom_textimpl.h"
#include "dom_xmlimpl.h"
#include <html/html_elementimpl.h>

using namespace DOM;


RangeImpl::RangeImpl(DocumentImpl *_ownerDocument)
{
    m_ownerDocument = _ownerDocument;
    m_ownerDocument->ref();
    m_startContainer = _ownerDocument;
    m_startContainer->ref();
    m_endContainer = _ownerDocument;
    m_endContainer->ref();
    m_startOffset = 0;
    m_endOffset = 0;
    m_detached = false;
}

RangeImpl::RangeImpl(DocumentImpl *_ownerDocument,
              NodeImpl *_startContainer, long _startOffset,
              NodeImpl *_endContainer, long _endOffset)
{
    m_ownerDocument = _ownerDocument;
    m_ownerDocument->ref();
    m_startContainer = _startContainer;
    m_startContainer->ref();
    m_startOffset = _startOffset;
    m_endContainer = _endContainer;
    m_endContainer->ref();
    m_endOffset = _endOffset;
    m_detached = false;
}

RangeImpl::~RangeImpl()
{
    m_ownerDocument->deref();
    int exceptioncode = 0;
    if (!m_detached)
        detach(exceptioncode);
}

NodeImpl *RangeImpl::startContainer(int &exceptioncode) const
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return 0;
    }

    return m_startContainer;
}

long RangeImpl::startOffset(int &exceptioncode) const
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return 0;
    }

    return m_startOffset;
}

NodeImpl *RangeImpl::endContainer(int &exceptioncode) const
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return 0;
    }

    return m_endContainer;
}

long RangeImpl::endOffset(int &exceptioncode) const
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return 0;
    }

    return m_endOffset;
}

NodeImpl *RangeImpl::commonAncestorContainer(int &exceptioncode)
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return 0;
    }

    NodeImpl *com = commonAncestorContainer(m_startContainer,m_endContainer);
    if (!com) //  should never happen
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
    return com;
}

NodeImpl *RangeImpl::commonAncestorContainer(NodeImpl *containerA, NodeImpl *containerB)
{
    NodeImpl *parentStart;

    for (parentStart = containerA; parentStart; parentStart = parentStart->parentNode()) {
        NodeImpl *parentEnd = containerB;
        while( parentEnd && (parentStart != parentEnd) )
            parentEnd = parentEnd->parentNode();

        if(parentStart == parentEnd)  break;
    }

    return parentStart;
}

bool RangeImpl::collapsed(int &exceptioncode) const
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return 0;
    }

    return (m_startContainer == m_endContainer && m_startOffset == m_endOffset);
}

void RangeImpl::setStart( NodeImpl *refNode, long offset, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    if (!refNode) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return;
    }

    if (refNode->document() != m_ownerDocument) {
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return;
    }

    checkNodeWOffset( refNode, offset, exceptioncode );
    if (exceptioncode)
        return;

    setStartContainer(refNode);
    m_startOffset = offset;

    // check if different root container
    NodeImpl *endRootContainer = m_endContainer;
    while (endRootContainer->parentNode())
        endRootContainer = endRootContainer->parentNode();
    NodeImpl *startRootContainer = m_startContainer;
    while (startRootContainer->parentNode())
        startRootContainer = startRootContainer->parentNode();
    if (startRootContainer != endRootContainer)
        collapse(true,exceptioncode);
    // check if new start after end
    else if (compareBoundaryPoints(m_startContainer,m_startOffset,m_endContainer,m_endOffset) > 0)
        collapse(true,exceptioncode);
}

void RangeImpl::setEnd( NodeImpl *refNode, long offset, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    if (!refNode) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return;
    }

    if (refNode->document() != m_ownerDocument) {
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return;
    }

    checkNodeWOffset( refNode, offset, exceptioncode );
    if (exceptioncode)
        return;

    setEndContainer(refNode);
    m_endOffset = offset;

    // check if different root container
    NodeImpl *endRootContainer = m_endContainer;
    while (endRootContainer->parentNode())
        endRootContainer = endRootContainer->parentNode();
    NodeImpl *startRootContainer = m_startContainer;
    while (startRootContainer->parentNode())
        startRootContainer = startRootContainer->parentNode();
    if (startRootContainer != endRootContainer)
        collapse(false,exceptioncode);
    // check if new end before start
    if (compareBoundaryPoints(m_startContainer,m_startOffset,m_endContainer,m_endOffset) > 0)
        collapse(false,exceptioncode);
}

void RangeImpl::collapse( bool toStart, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    if( toStart )   // collapse to start
    {
        setEndContainer(m_startContainer);
        m_endOffset = m_startOffset;
    }
    else            // collapse to end
    {
        setStartContainer(m_endContainer);
        m_startOffset = m_endOffset;
    }
}

short RangeImpl::compareBoundaryPoints( Range::CompareHow how, RangeImpl *sourceRange, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return 0;
    }

    if (!sourceRange) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return 0;
    }

    NodeImpl *thisCont = commonAncestorContainer(exceptioncode);
    NodeImpl *sourceCont = sourceRange->commonAncestorContainer(exceptioncode);
    if (exceptioncode)
        return 0;

    if (thisCont->document() != sourceCont->document()) {
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return 0;
    }

    NodeImpl *thisTop = thisCont;
    NodeImpl *sourceTop = sourceCont;
    while (thisTop->parentNode())
	thisTop = thisTop->parentNode();
    while (sourceTop->parentNode())
	sourceTop = sourceTop->parentNode();
    if (thisTop != sourceTop) { // in different DocumentFragments
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return 0;
    }

    switch(how)
    {
    case Range::START_TO_START:
        return compareBoundaryPoints( m_startContainer, m_startOffset,
                                      sourceRange->startContainer(exceptioncode), sourceRange->startOffset(exceptioncode) );
        break;
    case Range::START_TO_END:
        return compareBoundaryPoints( m_endContainer, m_endOffset,
                                      sourceRange->startContainer(exceptioncode), sourceRange->startOffset(exceptioncode) );
        break;
    case Range::END_TO_END:
        return compareBoundaryPoints( m_endContainer, m_endOffset,
                                      sourceRange->endContainer(exceptioncode), sourceRange->endOffset(exceptioncode) );
        break;
    case Range::END_TO_START:
        return compareBoundaryPoints( m_startContainer, m_startOffset,
                                      sourceRange->endContainer(exceptioncode), sourceRange->endOffset(exceptioncode) );
        break;
    default:
        exceptioncode = DOMException::SYNTAX_ERR;
        return 0;
    }
}

short RangeImpl::compareBoundaryPoints( NodeImpl *containerA, long offsetA, NodeImpl *containerB, long offsetB )
{
    // see DOM2 traversal & range section 2.5

    // case 1: both points have the same container
    if( containerA == containerB )
    {
        if( offsetA == offsetB )  return 0;    // A is equal to B
        if( offsetA < offsetB )  return -1;    // A is before B
        else  return 1;                        // A is after B
    }

    // case 2: node C (container B or an ancestor) is a child node of A
    NodeImpl *c = containerB;
    while (c && c->parentNode() != containerA)
        c = c->parentNode();
    if (c) {
        int offsetC = 0;
        NodeImpl* n = containerA->firstChild();
        while (n != c) {
            offsetC++;
            n = n->nextSibling();
        }

        if( offsetA <= offsetC )  return -1;    // A is before B
        else  return 1;                        // A is after B
    }

    // case 3: node C (container A or an ancestor) is a child node of B
    c = containerA;
    while (c && c->parentNode() != containerB)
        c = c->parentNode();
    if (c) {
        int offsetC = 0;
        NodeImpl* n = containerB->firstChild();
        while (n != c) {
            offsetC++;
            n = n->nextSibling();
        }

        if( offsetC < offsetB )  return -1;    // A is before B
        else  return 1;                        // A is after B
    }

    // case 4: containers A & B are siblings, or children of siblings
    // ### we need to do a traversal here instead
    NodeImpl *cmnRoot = commonAncestorContainer(containerA,containerB);
    if (!cmnRoot) return -1; // Whatever...
    NodeImpl *childA = containerA;
    while (childA->parentNode() != cmnRoot)
        childA = childA->parentNode();
    NodeImpl *childB = containerB;
    while (childB->parentNode() != cmnRoot)
        childB = childB->parentNode();

    NodeImpl *n = cmnRoot->firstChild();
    int i = 0;
    int childAOffset = -1;
    int childBOffset = -1;
    while (childAOffset < 0 || childBOffset < 0) {
        if (n == childA)
            childAOffset = i;
        if (n == childB)
            childBOffset = i;
        n = n->nextSibling();
        i++;
    }

    if( childAOffset == childBOffset )  return 0;    // A is equal to B
    if( childAOffset < childBOffset )   return -1;    // A is before B
    else  return 1;                        // A is after B
}

bool RangeImpl::boundaryPointsValid(  )
{
    short valid =  compareBoundaryPoints( m_startContainer, m_startOffset,
                                          m_endContainer, m_endOffset );
    if( valid == 1 )  return false;
    else  return true;

}

void RangeImpl::deleteContents( int &exceptioncode ) {
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    checkDeleteExtract(exceptioncode);
    if (exceptioncode)
	return;

    processContents(DELETE_CONTENTS,exceptioncode);
}

DocumentFragmentImpl *RangeImpl::processContents ( ActionType action, int &exceptioncode )
{
    // ### when mutation events are implemented, we will have to take into account
    // situations where the tree is being transformed while we delete - ugh!

    // ### perhaps disable node deletion notification for this range while we do this?

    // shortcut for special case
    if (collapsed(exceptioncode) || exceptioncode) {
	if (action == CLONE_CONTENTS || action == EXTRACT_CONTENTS)
	    return new DocumentFragmentImpl(m_ownerDocument);
        return 0;
    }

    NodeImpl *cmnRoot = commonAncestorContainer(exceptioncode);
    if (exceptioncode)
        return 0;

    // what is the highest node that partially selects the start of the range?
    NodeImpl *partialStart = 0;
    if (m_startContainer != cmnRoot) {
	partialStart = m_startContainer;
	while (partialStart->parentNode() != cmnRoot)
	    partialStart = partialStart->parentNode();
    }

    // what is the highest node that partially selects the end of the range?
    NodeImpl *partialEnd = 0;
    if (m_endContainer != cmnRoot) {
	partialEnd = m_endContainer;
	while (partialEnd->parentNode() != cmnRoot)
	    partialEnd = partialEnd->parentNode();
    }

    DocumentFragmentImpl *fragment = 0;
    if (action == EXTRACT_CONTENTS || action == CLONE_CONTENTS)
        fragment = new DocumentFragmentImpl(m_ownerDocument);

    // Simple case: the start and end containers are the same. We just grab
    // everything >= start offset and < end offset
    if (m_startContainer == m_endContainer) {
        if(m_startContainer->nodeType() == Node::TEXT_NODE ||
           m_startContainer->nodeType() == Node::CDATA_SECTION_NODE ||
           m_startContainer->nodeType() == Node::COMMENT_NODE) {

            if (action == EXTRACT_CONTENTS || action == CLONE_CONTENTS) {
                RefPtr<CharacterDataImpl> c = static_pointer_cast<CharacterDataImpl>(m_startContainer->cloneNode(true));
                c->deleteData(m_endOffset,static_cast<CharacterDataImpl*>(m_startContainer)->length()-m_endOffset,exceptioncode);
                c->deleteData(0,m_startOffset,exceptioncode);
                fragment->appendChild(c.get(),exceptioncode);
            }
            if (action == EXTRACT_CONTENTS || action == DELETE_CONTENTS)
                static_cast<CharacterDataImpl*>(m_startContainer)->deleteData(m_startOffset,m_endOffset-m_startOffset,exceptioncode);
        }
        else if (m_startContainer->nodeType() == Node::PROCESSING_INSTRUCTION_NODE) {
            // ### operate just on data ?
        }
        else {
            NodeImpl *n = m_startContainer->firstChild();
            unsigned long i;
            for (i = 0; n && i < m_startOffset; i++) // skip until m_startOffset
                n = n->nextSibling();
            while (n && i < m_endOffset) { // delete until m_endOffset
                NodeImpl *next = n->nextSibling();
                if (action == EXTRACT_CONTENTS)
                    fragment->appendChild(n,exceptioncode); // will remove n from its parent
                else if (action == CLONE_CONTENTS)
                    fragment->appendChild(n->cloneNode(true).get(),exceptioncode);
                else
                    m_startContainer->removeChild(n,exceptioncode);
                n = next;
                i++;
            }
        }
        collapse(true,exceptioncode);
        return fragment;
    }

    // Complex case: Start and end containers are different.
    // There are three possiblities here:
    // 1. Start container == cmnRoot (End container must be a descendant)
    // 2. End container == cmnRoot (Start container must be a descendant)
    // 3. Neither is cmnRoot, they are both descendants
    //
    // In case 3, we grab everything after the start (up until a direct child
    // of cmnRoot) into leftContents, and everything before the end (up until
    // a direct child of cmnRoot) into rightContents. Then we process all
    // cmnRoot children between leftContents and rightContents
    //
    // In case 1 or 2, we skip either processing of leftContents or rightContents,
    // in which case the last lot of nodes either goes from the first or last
    // child of cmnRoot.
    //
    // These are deleted, cloned, or extracted (i.e. both) depending on action.

    RefPtr<NodeImpl> leftContents = 0;
    if (m_startContainer != cmnRoot) {
        // process the left-hand side of the range, up until the last ancestor of
        // m_startContainer before cmnRoot
        if(m_startContainer->nodeType() == Node::TEXT_NODE ||
           m_startContainer->nodeType() == Node::CDATA_SECTION_NODE ||
           m_startContainer->nodeType() == Node::COMMENT_NODE) {

            if (action == EXTRACT_CONTENTS || action == CLONE_CONTENTS) {
                RefPtr<CharacterDataImpl> c = static_pointer_cast<CharacterDataImpl>(m_startContainer->cloneNode(true));
                c->deleteData(0,m_startOffset,exceptioncode);
                leftContents = c;
            }
            if (action == EXTRACT_CONTENTS || action == DELETE_CONTENTS)
                static_cast<CharacterDataImpl*>(m_startContainer)->deleteData(
                    m_startOffset,static_cast<CharacterDataImpl*>(m_startContainer)->length()-m_startOffset,exceptioncode);
        }
        else if (m_startContainer->nodeType() == Node::PROCESSING_INSTRUCTION_NODE) {
            // ### operate just on data ?
            // leftContents = ...
        }
        else {
            if (action == EXTRACT_CONTENTS || action == CLONE_CONTENTS)
		leftContents = m_startContainer->cloneNode(false);
            NodeImpl *n = m_startContainer->firstChild();
            for (unsigned long i = 0; n && i < m_startOffset; i++) // skip until m_startOffset
                n = n->nextSibling();
            while (n) { // process until end
		NodeImpl *next = n->nextSibling();
                if (action == EXTRACT_CONTENTS)
                    leftContents->appendChild(n,exceptioncode); // will remove n from m_startContainer
                else if (action == CLONE_CONTENTS)
                    leftContents->appendChild(n->cloneNode(true).get(),exceptioncode);
                else
                    m_startContainer->removeChild(n,exceptioncode);
                n = next;
            }
        }

        NodeImpl *leftParent = m_startContainer->parentNode();
        NodeImpl *n = m_startContainer->nextSibling();
        for (; leftParent != cmnRoot; leftParent = leftParent->parentNode()) {
            if (action == EXTRACT_CONTENTS || action == CLONE_CONTENTS) {
		RefPtr<NodeImpl> leftContentsParent = leftParent->cloneNode(false);
		leftContentsParent->appendChild(leftContents.get(),exceptioncode);
		leftContents = leftContentsParent;
	    }

            NodeImpl *next;
            for (; n; n = next) {
                next = n->nextSibling();
                if (action == EXTRACT_CONTENTS)
                    leftContents->appendChild(n,exceptioncode); // will remove n from leftParent
                else if (action == CLONE_CONTENTS)
                    leftContents->appendChild(n->cloneNode(true).get(),exceptioncode);
                else
                    leftParent->removeChild(n,exceptioncode);
            }
            n = leftParent->nextSibling();
        }
    }

    RefPtr<NodeImpl> rightContents = 0;
    if (m_endContainer != cmnRoot) {
        // delete the right-hand side of the range, up until the last ancestor of
        // m_endContainer before cmnRoot
        if(m_endContainer->nodeType() == Node::TEXT_NODE ||
           m_endContainer->nodeType() == Node::CDATA_SECTION_NODE ||
           m_endContainer->nodeType() == Node::COMMENT_NODE) {

            if (action == EXTRACT_CONTENTS || action == CLONE_CONTENTS) {
                RefPtr<CharacterDataImpl> c =
			    static_pointer_cast<CharacterDataImpl>(m_endContainer->cloneNode(true));
                c->deleteData(m_endOffset,static_cast<CharacterDataImpl*>(m_endContainer)->length()-m_endOffset,exceptioncode);
                rightContents = c;
            }
            if (action == EXTRACT_CONTENTS || action == DELETE_CONTENTS)
                static_cast<CharacterDataImpl*>(m_endContainer)->deleteData(0,m_endOffset,exceptioncode);
        }
        else if (m_startContainer->nodeType() == Node::PROCESSING_INSTRUCTION_NODE) {
            // ### operate just on data ?
            // rightContents = ...
        }
        else {
	    if (action == EXTRACT_CONTENTS || action == CLONE_CONTENTS)
		rightContents = m_endContainer->cloneNode(false);
            NodeImpl *n = m_endContainer->firstChild();
            if (n && m_endOffset) {
                for(unsigned long i = 0; i+1 < m_endOffset; i++) { // skip to m_endOffset
                    NodeImpl* next = n->nextSibling(); 
                    if (!next) 
                        break; 
                    n = next; 
                }
                NodeImpl *prev;
                for (; n; n = prev) {
                    prev = n->previousSibling();
                    if (action == EXTRACT_CONTENTS)
                        rightContents->insertBefore(n,rightContents->firstChild(),exceptioncode); // will remove n from its parent
                    else if (action == CLONE_CONTENTS)
                        rightContents->insertBefore(n->cloneNode(true).get(),rightContents->firstChild(),exceptioncode);
                    else
                        m_endContainer->removeChild(n,exceptioncode);
                }
            }
        }

        NodeImpl *rightParent = m_endContainer->parentNode();
        NodeImpl *n = m_endContainer->previousSibling();
        for (; rightParent != cmnRoot; rightParent = rightParent->parentNode()) {
        	if (action == EXTRACT_CONTENTS || action == CLONE_CONTENTS) {
	            RefPtr<NodeImpl> rightContentsParent = rightParent->cloneNode(false);
	            rightContentsParent->appendChild(rightContents.get(),exceptioncode);
	            rightContents = rightContentsParent;
            }

            NodeImpl *prev;
            for (; n; n = prev ) {
                prev = n->previousSibling();
                if (action == EXTRACT_CONTENTS)
                    rightContents->insertBefore(n,rightContents->firstChild(),exceptioncode); // will remove n from its parent
                else if (action == CLONE_CONTENTS)
                    rightContents->insertBefore(n->cloneNode(true).get(),rightContents->firstChild(),exceptioncode);
                else
                    rightParent->removeChild(n,exceptioncode);

            }
            n = rightParent->previousSibling();
        }
    }

    // delete all children of cmnRoot between the start and end container

    NodeImpl *processStart; // child of cmnRooot
    if (m_startContainer == cmnRoot) {
        unsigned long i;
        processStart = m_startContainer->firstChild();
        for (i = 0; i < m_startOffset; i++)
            processStart = processStart->nextSibling();
    }
    else {
        processStart = m_startContainer;
        while (processStart->parentNode() != cmnRoot)
            processStart = processStart->parentNode();
        processStart = processStart->nextSibling();
    }
    NodeImpl *processEnd; // child of cmnRooot
    if (m_endContainer == cmnRoot) {
        unsigned long i;
        processEnd = m_endContainer->firstChild();
        for (i = 0; i < m_endOffset; i++)
            processEnd = processEnd->nextSibling();
    }
    else {
        processEnd = m_endContainer;
        while (processEnd->parentNode() != cmnRoot)
            processEnd = processEnd->parentNode();
    }

    // Now add leftContents, stuff in between, and rightContents to the fragment
    // (or just delete the stuff in between)

    if ((action == EXTRACT_CONTENTS || action == CLONE_CONTENTS) && leftContents)
      fragment->appendChild(leftContents.get(),exceptioncode);

    NodeImpl *next;
    NodeImpl *n;
    if (processStart) {
        for (n = processStart; n && n != processEnd; n = next) {
            next = n->nextSibling();

            if (action == EXTRACT_CONTENTS)
                fragment->appendChild(n,exceptioncode); // will remove from cmnRoot
            else if (action == CLONE_CONTENTS)
                fragment->appendChild(n->cloneNode(true).get(),exceptioncode);
            else
                cmnRoot->removeChild(n,exceptioncode);
        }
    }

    if ((action == EXTRACT_CONTENTS || action == CLONE_CONTENTS) && rightContents)
      fragment->appendChild(rightContents.get(),exceptioncode);

    // collapse to the proper position - see spec section 2.6
    if (action == EXTRACT_CONTENTS || action == DELETE_CONTENTS) {
	if (!partialStart && !partialEnd)
	    collapse(true,exceptioncode);
	else if (partialStart) {
	    setStartContainer(partialStart->parentNode());
	    setEndContainer(partialStart->parentNode());
	    m_startOffset = m_endOffset = partialStart->nodeIndex()+1;
	}
	else if (partialEnd) {
	    setStartContainer(partialEnd->parentNode());
	    setEndContainer(partialEnd->parentNode());
	    m_startOffset = m_endOffset = partialEnd->nodeIndex();
	}
    }
    return fragment;
}


DocumentFragmentImpl *RangeImpl::extractContents( int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return 0;
    }

    checkDeleteExtract(exceptioncode);
    if (exceptioncode)
	return 0;

    return processContents(EXTRACT_CONTENTS,exceptioncode);
}

DocumentFragmentImpl *RangeImpl::cloneContents( int &exceptioncode  )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return 0;
    }

    return processContents(CLONE_CONTENTS,exceptioncode);
}

void RangeImpl::insertNode( NodeImpl *newNode, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }
    
    if (!newNode) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return;
    }

    // NO_MODIFICATION_ALLOWED_ERR: Raised if an ancestor container of either boundary-point of
    // the Range is read-only.
    NodeImpl *n = m_startContainer;
    while (n && !n->isReadOnly())
        n = n->parentNode();
    if (n) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    n = m_endContainer;
    while (n && !n->isReadOnly())
        n = n->parentNode();
    if (n) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    // WRONG_DOCUMENT_ERR: Raised if newParent and the container of the start of the Range were
    // not created from the same document.
    if (newNode->document() != m_startContainer->document()) {
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return;
    }


    // HIERARCHY_REQUEST_ERR: Raised if the container of the start of the Range is of a type that
    // does not allow children of the type of newNode or if newNode is an ancestor of the container.

    // an extra one here - if a text node is going to split, it must have a parent to insert into
    if (m_startContainer->nodeType() == Node::TEXT_NODE && !m_startContainer->parentNode()) {
        exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
        return;
    }

    // In the case where the container is a text node, we check against the container's parent, because
    // text nodes get split up upon insertion.
    NodeImpl *checkAgainst;
    if (m_startContainer->nodeType() == Node::TEXT_NODE)
	checkAgainst = m_startContainer->parentNode();
    else
	checkAgainst = m_startContainer;

    if (newNode->nodeType() == Node::DOCUMENT_FRAGMENT_NODE) {
	// check each child node, not the DocumentFragment itself
    	NodeImpl *c;
    	for (c = newNode->firstChild(); c; c = c->nextSibling()) {
	    if (!checkAgainst->childTypeAllowed(c->nodeType())) {
		exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
		return;
	    }
    	}
    }
    else {
	if (!checkAgainst->childTypeAllowed(newNode->nodeType())) {
	    exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
	    return;
	}
    }

    for (n = m_startContainer; n; n = n->parentNode()) {
        if (n == newNode) {
            exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
            return;
        }
    }

    // INVALID_NODE_TYPE_ERR: Raised if newNode is an Attr, Entity, Notation, or Document node.
    if (newNode->nodeType() == Node::ATTRIBUTE_NODE ||
        newNode->nodeType() == Node::ENTITY_NODE ||
        newNode->nodeType() == Node::NOTATION_NODE ||
        newNode->nodeType() == Node::DOCUMENT_NODE) {
        exceptioncode = RangeException::INVALID_NODE_TYPE_ERR + RangeException::_EXCEPTION_OFFSET;
        return;
    }

    long endOffsetDelta = 0;
    if (m_startContainer->nodeType() == Node::TEXT_NODE ||
        m_startContainer->nodeType() == Node::CDATA_SECTION_NODE )
    {
        // ### leaks on exceptions
        TextImpl *newText = static_cast<TextImpl*>(m_startContainer)->splitText(m_startOffset,exceptioncode);
        if (exceptioncode)
            return;

        if (m_startContainer == m_endContainer) {
            endOffsetDelta = -(long)m_startOffset;
            setEndContainer(newText);
            // ### what about border cases where start or end are at
            // ### the margins of the text?
        }

        m_startContainer->parentNode()->insertBefore( newNode, newText, exceptioncode );
        if (exceptioncode)
            return;
    } else {
        if (m_startContainer == m_endContainer) {
            bool isFragment = newNode->nodeType() == Node::DOCUMENT_FRAGMENT_NODE;
            endOffsetDelta = isFragment ? newNode->childNodeCount() : 1;
        }

        m_startContainer->insertBefore( newNode, m_startContainer->childNode( m_startOffset ), exceptioncode );
	if (exceptioncode)
	    return;
    }
    m_endOffset += endOffsetDelta;
}

DOMString RangeImpl::toString( int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return DOMString();
    }

    DOMString text = "";
    NodeImpl *n = m_startContainer;

    /* This function converts a dom range to the plain text string that the user would see in this
     * portion of rendered html.
     *
     * There are several ways ranges can be used.
     *
     * The simplest is the start and endContainer is a text node.  The start and end offset is the
     * number of characters into the text to remove/truncate.
     *
     * The next case is the start and endContainer is, well, a container, such a P tag or DIV tag.
     * In this case the start and end offset is the number of children into the container to start
     * from and end at.
     *
     * The other cases are different arrangements of the first two.
     *
     * pseudo code:
     *
     * if start container is not text:
     *     count through the children to find where we start (m_startOffset children)
     *
     * loop from the start position:
     *     if the current node is text, add the text to our variable 'text', truncating/removing if at the end/start.
     *
     *     if the node has children, step to the first child.
     *     if the node has no children but does have siblings, step to the next sibling
     *     until we find a sibling, go to next the parent but:
     *         make sure this sibling isn't past the end of where we are supposed to go. (position > endOffset and the parent is the endContainer)
     *
     */


    if( m_startContainer == m_endContainer && m_startOffset >= m_endOffset)
	return text;


    if(n->firstChild()) {
	n = n->firstChild();
   	int current_offset = m_startOffset;
	while(current_offset-- && n) {
	    n = n->nextSibling();
	}
    }

    while(n) {
        if(n == m_endContainer && m_endOffset == 0)
            break;
        if(n->nodeType() == DOM::Node::TEXT_NODE ||
           n->nodeType() == DOM::Node::CDATA_SECTION_NODE) {

            DOMString str = static_cast<TextImpl *>(n)->string();
	    if( n == m_endContainer || n == m_startContainer)
	        str = str.copy();  //copy if we are going to modify.

            if (n == m_endContainer)
                str.truncate(m_endOffset);
            if (n == m_startContainer)
                str.remove(0,m_startOffset);
	    text += str;
	    if (n == m_endContainer)
                break;
        }


	NodeImpl *next = n->firstChild();
	if(!next)
            next = n->nextSibling();

        while( !next && n->parentNode() ) {
            if (n == m_endContainer) return text;
            n = n->parentNode();
            if (n == m_endContainer) return text;
            next = n->nextSibling();
        }

        if(n->parentNode() == m_endContainer) {
            if(!next) break;
	    unsigned long current_offset = 0;
	    NodeImpl *it = n;
	    while((it = it->previousSibling())) ++current_offset;
	    if(current_offset >= m_endOffset) {
	        break;
	    }
	}

        n = next;
    }
    return text;
}

DOMString RangeImpl::toHTML( int &exceptioncode )
{
    bool hasHtmlTag = false;
    bool hasBodyTag = false;
    //FIXME:  What is this section of code below exactly?  Do I want it here?
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return DOMString();
    }
    DOMString text = "";
    NodeImpl *n = m_startContainer;
    int num_tables=0;
    bool in_li = false; //whether we have an li in the text, without an ol/ul
    int depth_difference = 0;
    int lowest_depth_difference = 0;

    if( m_startContainer == m_endContainer && m_startOffset >= m_endOffset)
	return text;

    while(n) {
        /* First, we could have an tag <tagname key=value>otherstuff</tagname> */
	if(n->nodeType() == DOM::Node::ELEMENT_NODE) {
	    int elementId = static_cast<ElementImpl *>(n)->id();
            if(elementId == ID_TABLE) num_tables++;
	    if(elementId == ID_BODY) hasBodyTag = true;
	    if(elementId == ID_HTML) hasHtmlTag = true;
	    if(elementId == ID_LI) in_li=true;
	    if(num_tables==0 && ( elementId == ID_TD || elementId == ID_TR || elementId == ID_TH || elementId == ID_TBODY || elementId == ID_TFOOT || elementId == ID_THEAD)) num_tables++;
	    if(!( !n->hasChildNodes() && (elementId == ID_H1 || elementId == ID_H2 || elementId == ID_H3 || elementId == ID_H4 || elementId ==ID_H5))) {  //Don't add <h1/>  etc.  Just skip these nodes just to make the output html a bit nicer.
	       text += static_cast<ElementImpl *>(n)->openTagStartToString(true /*safely expand img urls*/); // adds "<tagname key=value"
	       if(n->hasChildNodes()) {
	            depth_difference++;
   	            text += ">";
                } else {
          	    text += "/>";
	        }
	    }
	} else
        if(n->nodeType() == DOM::Node::TEXT_NODE ||
           n->nodeType() == DOM::Node::CDATA_SECTION_NODE) {
            if(n->nodeType() == DOM::Node::CDATA_SECTION_NODE) text += "<![CDATA[ ";
	    long long startOffset = (n == m_startContainer)?(long long)m_startOffset:-1;
	    long long endOffset = (n == m_endContainer)?(long long) m_endOffset:-1;
            text += static_cast<TextImpl *>(n)->toString(startOffset, endOffset); //Note this should always work since CDataImpl inherits TextImpl
	    if(n->nodeType() == DOM::Node::CDATA_SECTION_NODE) text += " ]]>";
	    if(n == m_endContainer) {
		    break;
	    }
        }
        if(n->parentNode() == m_endContainer && !n->nextSibling()) {
            break;
        }

        //if (n == m_endContainer) break;
        NodeImpl *next = n->firstChild();
	if(next) {
	    if(n == m_startContainer) {
                //This is the start of our selection, so we have to move to where we have started selecting.
		//For example, if 'n' is "hello <img src='hello.png'> how are you? <img src='goodbye.png'>"
		//then this has four children.  If our selection started on the image, then we need to start from there.
		unsigned long current_offset = 0;
		while(current_offset < m_startOffset && next) {
		    next = next->nextSibling();
		    ++current_offset;
		}
	    }
	} else {
            next = n->nextSibling();

	    if(n->parentNode() == m_endContainer) {
	        unsigned long current_offset = 1;
	        NodeImpl *it = n;
	        while((it = it->previousSibling())) ++current_offset;

	        if(current_offset >= m_endOffset) {
	            break;
	        }
	    }
	}

        while( !next && n->parentNode() ) {
            n = n->parentNode();
  	    if(n->nodeType() == DOM::Node::ELEMENT_NODE) {
  		text += "</";
	        text += static_cast<ElementImpl *>(n)->nonCaseFoldedTagName();
	        int elementId = static_cast<ElementImpl *>(n)->id();
                if(elementId == ID_TABLE) num_tables--;
	        depth_difference--;
		if(lowest_depth_difference > depth_difference) lowest_depth_difference=depth_difference;
	        if(num_tables==0 && ( elementId == ID_TD || elementId == ID_TR || elementId == ID_TH || elementId == ID_TBODY || elementId == ID_TFOOT || elementId == ID_THEAD)) num_tables--;
	        if(elementId == ID_OL || elementId == ID_UL) in_li=false;
 	        text += ">";
	    }
            next = n->nextSibling();
        }
        n = next;
    }

    	//We have the html in the selection.  But now we need to properly add the opening and closing tags.
    //For example say we have:   "Hello <b>Mr. John</b> How are you?"  and we select "John" or even
    //"John</b> How"  and copy.  We want to return "<b>John</b>" and "<b>John</b> How" respectively

    //To do this, we need to go up the tree from the start, and prepend those tags.
    //Imagine our selection was this:
    //
    //  hello</b></p><p>there
    //
    //  The difference in depths between the start and end is -1, and the lowest depth
    //  difference from the starting point is -2
    //
    //  So from the start of the selection, we want to go down to the lowest_depth_difference
    //  and prepend those tags.  (<p><b>)
    //
    //  From the end of the selection, we want to also go down to the lowest_depth_difference.
    //  We know the depth of the end of the selection - i.e. depth_difference.
    //
    //
    n = m_startContainer;
    int startdepth = 0; //by definition - we are counting from zero.
    while((n = n->parentNode()) && startdepth>lowest_depth_difference) {
      if(n->nodeType() == DOM::Node::ELEMENT_NODE) { //This should always be true.. right?
	  switch (static_cast<ElementImpl *>(n)->id()) {
	      case ID_TABLE:
		 num_tables--;
		 break;
	      case ID_BODY:
	         hasBodyTag = true;
		 break;
	      case ID_HTML:
	         hasHtmlTag = true;
		 break;
	      case ID_LI:
		 in_li = true;
		 break;
	  }
          text = static_cast<ElementImpl *>(n)->openTagStartToString(true /*expand img urls*/)+DOMString(">") +text; // prepends "<tagname key=value>"
      }
      startdepth--;
    }
    n = m_endContainer;
    while( depth_difference>lowest_depth_difference && (n = n->parentNode())) {
      if(n->nodeType() == DOM::Node::ELEMENT_NODE) { //This should always be true.. right?
	  switch (static_cast<ElementImpl *>(n)->id()) {
	      case ID_TABLE:
		num_tables++;
		break;
	      case ID_OL:
	      case ID_UL:
		in_li=false;
		break;
	  }
	  text += "</";
	  text += static_cast<ElementImpl *>(n)->nonCaseFoldedTagName();
 	  text += ">";
      }
      depth_difference--;
    }

    // Now our text string is the same depth on both sides, with nothing lower (in other words all the
    // tags in it match up.)  This also means that the end value for n in the first loop is a sibling of the
    // end value for n in the second loop.
    //
    // We now need to go down the tree, and for certain tags, add them in on both ends of the text.
    // For example, if have:  "<b>hello</b>"  and we select "ll", then we want to go down the tree and
    // add "<b>" and "</b>" to it, to produce "<b>ll</b>".
    //
    // I just guessed at which tags you'd want to keep (bold, italic etc) and which you wouldn't (tables etc).
    // It's just wild guessing.  feel free to change.
    //
    // Note we can carry on with the value of n
    if(n) {
      while((n = n->parentNode())) {
          if(n->nodeType() == DOM::Node::ELEMENT_NODE) { //This should always be true.. right?
	      int elementId = static_cast<ElementImpl *>(n)->id();
	      switch (elementId) {
 	        case ID_TABLE:
		case ID_TD:
		case ID_TR:
		case ID_TH:
		case ID_TBODY:
		case ID_TFOOT:
		case ID_THEAD:
		    if(num_tables>0) {
		        if(elementId == ID_TABLE) num_tables--;
			text = static_cast<ElementImpl *>(n)->openTagStartToString(true /*expand img urls*/)+DOMString(">") +text;
			text += "</";
			text += static_cast<ElementImpl *>(n)->nonCaseFoldedTagName();
			text += ">";

		    }
		    break;

                case ID_LI:
		    if(!in_li) break;
                    text = static_cast<ElementImpl *>(n)->openTagStartToString(true /*expand img urls*/)+DOMString(">") +text;
		    text += "</";
		    text += static_cast<ElementImpl *>(n)->nonCaseFoldedTagName();
		    text += ">";
                    break;

		case ID_UL:
		case ID_OL:
		    if(!in_li) break;
		    in_li = false;
		case ID_B:
		case ID_I:
		case ID_U:
		case ID_FONT:
		case ID_S:
		case ID_STRONG:
		case ID_STRIKE:
		case ID_DEL:
		case ID_A:
		case ID_H1:
		case ID_H2:
		case ID_H3:
		case ID_H4:
		case ID_H5:
		    //should small, etc be here?   so hard to decide.  this is such a hack :(
		    //There's probably tons of others you'd want here.
                    text = static_cast<ElementImpl *>(n)->openTagStartToString(true /*expand img urls*/)+DOMString(">") +text;
		    text += "</";
		    text += static_cast<ElementImpl *>(n)->nonCaseFoldedTagName();
		    text += ">";
                    break;
	      }
	  }
      }
    }


    if(!hasBodyTag) text = DOMString("<body>") + text + DOMString("</body>");
    else if(!hasHtmlTag) {
      text = DOMString("<html xmlns=\"http://www.w3.org/1999/xhtml\">\n"
		      "<head>\n"
		      "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=UTF-8\" />\n"
		      "<meta name=\"Generator\" content=\"KHTML, the KDE Web Page Viewer\" />\n"
		      "</head>\n") +
	    text +
	    DOMString("</html>");
    }
    text = DOMString("<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.0 Strict//EN\" \"DTD/xhtml1-strict.dtd\">\n") + text;

    return text;

}

DocumentFragment RangeImpl::createContextualFragment ( const DOMString &html, int &exceptioncode )
{
   if (m_detached) {
	exceptioncode = DOMException::INVALID_STATE_ERR;
	return DocumentFragment();
    }

    DOM::NodeImpl* start = m_startContainer;

    if (start->isDocumentNode())
	start = static_cast<DocumentImpl*>(start)->documentElement();

    if (!start || !start->isHTMLElement()) {
	exceptioncode = DOMException::NOT_SUPPORTED_ERR;
	return DocumentFragment();
    }

    HTMLElementImpl *e = static_cast<HTMLElementImpl *>(start);
    DocumentFragment fragment = e->createContextualFragment(html);
    if (fragment.isNull()) {
	exceptioncode = DOMException::NOT_SUPPORTED_ERR;
	return DocumentFragment();
    }

    return fragment;
}


void RangeImpl::detach( int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    if (m_startContainer)
        m_startContainer->deref();
    m_startContainer = 0;
    if (m_endContainer)
        m_endContainer->deref();
    m_endContainer = 0;
    m_detached = true;
}

bool RangeImpl::isDetached() const
{
    return m_detached;
}

void RangeImpl::checkNodeWOffset( NodeImpl *n, int offset, int &exceptioncode) const
{
    if( offset < 0 ) {
        exceptioncode = DOMException::INDEX_SIZE_ERR;
    }

    switch (n->nodeType()) {
	case Node::ENTITY_NODE:
	case Node::NOTATION_NODE:
	case Node::DOCUMENT_TYPE_NODE:
            exceptioncode = RangeException::INVALID_NODE_TYPE_ERR + RangeException::_EXCEPTION_OFFSET;
	    break;
        case Node::TEXT_NODE:
        case Node::COMMENT_NODE:
        case Node::CDATA_SECTION_NODE:
            if ( (unsigned long)offset > static_cast<CharacterDataImpl*>(n)->length() )
                exceptioncode = DOMException::INDEX_SIZE_ERR;
            break;
        case Node::PROCESSING_INSTRUCTION_NODE:
            // ### are we supposed to check with just data or the whole contents?
            if ( (unsigned long)offset > static_cast<ProcessingInstructionImpl*>(n)->data().length() )
                exceptioncode = DOMException::INDEX_SIZE_ERR;
            break;
        default:
            if ( (unsigned long)offset > n->childNodeCount() )
                exceptioncode = DOMException::INDEX_SIZE_ERR;
            break;
    }
}

void RangeImpl::checkNodeBA( NodeImpl *n, int &exceptioncode ) const
{
    // INVALID_NODE_TYPE_ERR: Raised if the root container of refNode is not an
    // Attr, Document or DocumentFragment node or if refNode is a Document,
    // DocumentFragment, Attr, Entity, or Notation node.
    NodeImpl *root = n;
    while (root->parentNode())
	root = root->parentNode();
    if (!(root->nodeType() == Node::ATTRIBUTE_NODE ||
          root->nodeType() == Node::DOCUMENT_NODE ||
          root->nodeType() == Node::DOCUMENT_FRAGMENT_NODE)) {
        exceptioncode = RangeException::INVALID_NODE_TYPE_ERR + RangeException::_EXCEPTION_OFFSET;
        return;
    }

    if( n->nodeType() == Node::DOCUMENT_NODE ||
        n->nodeType() == Node::DOCUMENT_FRAGMENT_NODE ||
        n->nodeType() == Node::ATTRIBUTE_NODE ||
        n->nodeType() == Node::ENTITY_NODE ||
        n->nodeType() == Node::NOTATION_NODE )
        exceptioncode = RangeException::INVALID_NODE_TYPE_ERR  + RangeException::_EXCEPTION_OFFSET;

}

RangeImpl *RangeImpl::cloneRange(int &exceptioncode)
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return 0;
    }

    return new RangeImpl(m_ownerDocument,m_startContainer,m_startOffset,m_endContainer,m_endOffset);
}

void RangeImpl::setStartAfter( NodeImpl *refNode, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    if (!refNode) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return;
    }

    if (refNode->document() != m_ownerDocument) {
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return;
    }

    checkNodeBA( refNode, exceptioncode );
    if (exceptioncode)
        return;

    setStart( refNode->parentNode(), refNode->nodeIndex()+1, exceptioncode );
}

void RangeImpl::setEndBefore( NodeImpl *refNode, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    if (!refNode) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return;
    }

    if (refNode->document() != m_ownerDocument) {
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return;
    }

    checkNodeBA( refNode, exceptioncode );
    if (exceptioncode)
        return;

    setEnd( refNode->parentNode(), refNode->nodeIndex(), exceptioncode );
}

void RangeImpl::setEndAfter( NodeImpl *refNode, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    if (!refNode) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return;
    }

    if (refNode->document() != m_ownerDocument) {
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return;
    }

    checkNodeBA( refNode, exceptioncode );
    if (exceptioncode)
        return;

    setEnd( refNode->parentNode(), refNode->nodeIndex()+1, exceptioncode );

}

void RangeImpl::selectNode( NodeImpl *refNode, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    if (!refNode) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return;
    }

    // INVALID_NODE_TYPE_ERR: Raised if an ancestor of refNode is an Entity, Notation or
    // DocumentType node or if refNode is a Document, DocumentFragment, Attr, Entity, or Notation
    // node.
    NodeImpl *anc;
    for (anc = refNode->parentNode(); anc; anc = anc->parentNode()) {
        if (anc->nodeType() == Node::ENTITY_NODE ||
            anc->nodeType() == Node::NOTATION_NODE ||
            anc->nodeType() == Node::DOCUMENT_TYPE_NODE) {

            exceptioncode = RangeException::INVALID_NODE_TYPE_ERR + RangeException::_EXCEPTION_OFFSET;
            return;
        }
    }

    if (refNode->nodeType() == Node::DOCUMENT_NODE ||
        refNode->nodeType() == Node::DOCUMENT_FRAGMENT_NODE ||
        refNode->nodeType() == Node::ATTRIBUTE_NODE ||
        refNode->nodeType() == Node::ENTITY_NODE ||
        refNode->nodeType() == Node::NOTATION_NODE) {

        exceptioncode = RangeException::INVALID_NODE_TYPE_ERR + RangeException::_EXCEPTION_OFFSET;
        return;
    }

    setStartBefore( refNode, exceptioncode );
    if (exceptioncode)
        return;
    setEndAfter( refNode, exceptioncode );
}

void RangeImpl::selectNodeContents( NodeImpl *refNode, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    if (!refNode) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return;
    }

    // INVALID_NODE_TYPE_ERR: Raised if refNode or an ancestor of refNode is an Entity, Notation
    // or DocumentType node.
    NodeImpl *n;
    for (n = refNode; n; n = n->parentNode()) {
        if (n->nodeType() == Node::ENTITY_NODE ||
            n->nodeType() == Node::NOTATION_NODE ||
            n->nodeType() == Node::DOCUMENT_TYPE_NODE) {

            exceptioncode = RangeException::INVALID_NODE_TYPE_ERR + RangeException::_EXCEPTION_OFFSET;
            return;
        }
    }

    setStartContainer(refNode);
    m_startOffset = 0;
    setEndContainer(refNode);
    m_endOffset = maxEndOffset();
}

void RangeImpl::surroundContents( NodeImpl *newParent, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    if( !newParent ) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return;
    }

    // INVALID_NODE_TYPE_ERR: Raised if node is an Attr, Entity, DocumentType, Notation,
    // Document, or DocumentFragment node.
    if( newParent->nodeType() == Node::ATTRIBUTE_NODE ||
        newParent->nodeType() == Node::ENTITY_NODE ||
        newParent->nodeType() == Node::NOTATION_NODE ||
        newParent->nodeType() == Node::DOCUMENT_TYPE_NODE ||
        newParent->nodeType() == Node::DOCUMENT_NODE ||
        newParent->nodeType() == Node::DOCUMENT_FRAGMENT_NODE) {
        exceptioncode = RangeException::INVALID_NODE_TYPE_ERR + RangeException::_EXCEPTION_OFFSET;
        return;
    }

    // NO_MODIFICATION_ALLOWED_ERR: Raised if an ancestor container of either boundary-point of
    // the Range is read-only.
    if (readOnly()) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    NodeImpl *n = m_startContainer;
    while (n && !n->isReadOnly())
        n = n->parentNode();
    if (n) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    n = m_endContainer;
    while (n && !n->isReadOnly())
        n = n->parentNode();
    if (n) {
        exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    // WRONG_DOCUMENT_ERR: Raised if newParent and the container of the start of the Range were
    // not created from the same document.
    if (newParent->document() != m_startContainer->document()) {
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return;
    }

    // HIERARCHY_REQUEST_ERR: Raised if the container of the start of the Range is of a type that
    // does not allow children of the type of newParent or if newParent is an ancestor of the container
    // or if node would end up with a child node of a type not allowed by the type of node.

    // If m_startContainer is a character data node, it will be split and it will be its parent that will 
    // need to accept newParent (or in the case of a comment, it logically "would" be inserted into the parent,
    // although this will fail below for another reason).

    NodeImpl* parentOfNewParent = m_startContainer;

    if (parentOfNewParent->offsetInCharacters())
        parentOfNewParent = parentOfNewParent->parentNode();

    if (!parentOfNewParent->childTypeAllowed(newParent->nodeType())) {
        exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
        return;
    }

    for (n = m_startContainer; n; n = n->parentNode()) {
        if (n == newParent) {
            exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
            return;
        }
    }

    // ### check if node would end up with a child node of a type not allowed by the type of node

    // BAD_BOUNDARYPOINTS_ERR: Raised if the Range partially selects a non-text node.
    if (m_startContainer->nodeType() != Node::TEXT_NODE &&
        m_startContainer->nodeType() != Node::CDATA_SECTION_NODE) {

        if (m_startOffset > 0 && m_startOffset < maxStartOffset()) {
            exceptioncode = RangeException::BAD_BOUNDARYPOINTS_ERR + RangeException::_EXCEPTION_OFFSET;
            return;
        }
    }

    if (m_endContainer->nodeType() != Node::TEXT_NODE &&
        m_endContainer->nodeType() != Node::CDATA_SECTION_NODE) {

        if (m_endOffset > 0 && m_endOffset < maxEndOffset()) {
            exceptioncode = RangeException::BAD_BOUNDARYPOINTS_ERR + RangeException::_EXCEPTION_OFFSET;
            return;
        }
    }

    while (newParent->firstChild()) {
    	newParent->removeChild(newParent->firstChild(),exceptioncode);
	if (exceptioncode)
	    return;
    }
    DocumentFragmentImpl *fragment = extractContents(exceptioncode);
    if (exceptioncode)
        return;
    insertNode( newParent, exceptioncode );
    if (exceptioncode)
        return;
    newParent->appendChild( fragment, exceptioncode );
    if (exceptioncode)
        return;
    selectNode( newParent, exceptioncode );
}


unsigned long RangeImpl::maxStartOffset() const
{
    if (!m_startContainer)
        return 0;
    if (!m_startContainer->offsetInCharacters())
        return m_startContainer->childNodeCount();
    return m_startContainer->maxCharacterOffset();
}

unsigned long RangeImpl::maxEndOffset() const
{
    if (!m_endContainer)
        return 0;
    if (!m_endContainer->offsetInCharacters())
        return m_endContainer->childNodeCount();
    return m_endContainer->maxCharacterOffset();
}
     

void RangeImpl::setStartBefore( NodeImpl *refNode, int &exceptioncode )
{
    if (m_detached) {
        exceptioncode = DOMException::INVALID_STATE_ERR;
        return;
    }

    if (!refNode) {
        exceptioncode = DOMException::NOT_FOUND_ERR;
        return;
    }

    if (refNode->document() != m_ownerDocument) {
        exceptioncode = DOMException::WRONG_DOCUMENT_ERR;
        return;
    }

    checkNodeBA( refNode, exceptioncode );
    if (exceptioncode)
        return;

    setStart( refNode->parentNode(), refNode->nodeIndex(), exceptioncode );
}

void RangeImpl::setStartContainer(NodeImpl *_startContainer)
{
    if (m_startContainer == _startContainer)
        return;

    if (m_startContainer)
        m_startContainer->deref();
    m_startContainer = _startContainer;
    if (m_startContainer)
        m_startContainer->ref();
}

void RangeImpl::setEndContainer(NodeImpl *_endContainer)
{
    if (m_endContainer == _endContainer)
        return;

    if (m_endContainer)
        m_endContainer->deref();
    m_endContainer = _endContainer;
    if (m_endContainer)
        m_endContainer->ref();
}

void RangeImpl::checkDeleteExtract(int &exceptioncode) {

    NodeImpl *start;
    if (m_startContainer->nodeType() != Node::TEXT_NODE &&
	m_startContainer->nodeType() != Node::CDATA_SECTION_NODE &&
	m_startContainer->nodeType() != Node::COMMENT_NODE &&
	m_startContainer->nodeType() != Node::PROCESSING_INSTRUCTION_NODE) {

	start = m_startContainer->childNode(m_startOffset);
	if (!start) {
	    if (m_startContainer->lastChild())
		start = m_startContainer->lastChild()->traverseNextNode();
	    else
		start = m_startContainer->traverseNextNode();
	}
    }
    else
	start = m_startContainer;

    NodeImpl *end;
    if (m_endContainer->nodeType() != Node::TEXT_NODE &&
	m_endContainer->nodeType() != Node::CDATA_SECTION_NODE &&
	m_endContainer->nodeType() != Node::COMMENT_NODE &&
	m_endContainer->nodeType() != Node::PROCESSING_INSTRUCTION_NODE) {

	end = m_endContainer->childNode(m_endOffset);
	if (!end) {
	    if (m_endContainer->lastChild())
		end = m_endContainer->lastChild()->traverseNextNode();
	    else
		end = m_endContainer->traverseNextNode();
	}
    }
    else
	end = m_endContainer;

    NodeImpl *n;
    for (n = start; n && n != end; n = n->traverseNextNode()) {
	if (n->isReadOnly()) {
	    exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
	    return;
	}
	if (n->nodeType() == Node::DOCUMENT_TYPE_NODE) { // ### is this for only directly under the DF, or anywhere?
	    exceptioncode = DOMException::HIERARCHY_REQUEST_ERR;
	    return;
	}
    }

    if (containedByReadOnly()) {
	exceptioncode = DOMException::NO_MODIFICATION_ALLOWED_ERR;
	return;
    }
}


bool RangeImpl::containedByReadOnly() {
    NodeImpl *n;
    for (n = m_startContainer; n; n = n->parentNode()) {
	if (n->isReadOnly())
	    return true;
    }
    for (n = m_endContainer; n; n = n->parentNode()) {
	if (n->isReadOnly())
	    return true;
    }
    return false;
}

// kate: indent-width 4; replace-tabs off; tab-width 8; space-indent off;
