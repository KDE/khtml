/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll <knoll@kde.org>
 * Copyright (C) 2000 Frederik Holljen <frederik.holljen@hig.no>
 * Copyright (C) 2001 Peter Kelly <pmk@post.com>
 * Copyright (C) 2008 Maksim Orlovich <maksim@kde.org>
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

#include "dom/dom_exception.h"
#include "xml/dom_docimpl.h"

/**
 Robustness notes: we protect the object we hold on to, to
 prevent crashes. To ensure termination in JS used, we propagate exceptions,
 and rely on the JS CPU guard to set one to force us to bail out.

 ### TODO/CHECKONE
*/

using namespace DOM;

NodeIteratorImpl::NodeIteratorImpl(NodeImpl *_root, unsigned long _whatToShow,
				   NodeFilterImpl* _filter, bool _entityReferenceExpansion)
{
    m_root = _root;
    m_whatToShow = _whatToShow;
    m_filter = _filter;
    m_expandEntityReferences = _entityReferenceExpansion;

    m_referenceNode = _root;
    m_position = ITER_BEFORE_REF;

    m_doc = m_root->document();
    m_doc->attachNodeIterator(this);
    m_doc->ref();

    m_detached = false;
}

NodeIteratorImpl::~NodeIteratorImpl()
{
    m_doc->detachNodeIterator(this);
    m_doc->deref();
}

NodeImpl *NodeIteratorImpl::root()
{
    return m_root.get();
}

unsigned long NodeIteratorImpl::whatToShow()
{
    return m_whatToShow;
}

NodeFilterImpl* NodeIteratorImpl::filter()
{
    return m_filter.get();
}

bool NodeIteratorImpl::expandEntityReferences()
{
    return m_expandEntityReferences;
}

SharedPtr<NodeImpl> NodeIteratorImpl::nextNode( int &exceptioncode, void* &propagatedExceptionObject )
{
    propagatedExceptionObject = 0;
    if (m_detached) {
	exceptioncode = DOMException::INVALID_STATE_ERR;
	return 0;
    }

    SharedPtr<NodeImpl> oldReferenceNode = m_referenceNode;
    Position oldPosition = m_position;
    while (true) {
        SharedPtr<NodeImpl> cand = m_referenceNode;
        // If we're before a node, it's the next node to return, and we'll
        // be positioned after.
        if (m_position == ITER_BEFORE_REF) {
            m_position = ITER_AFTER_REF;
            if (isAccepted(m_referenceNode.get(), propagatedExceptionObject) == NodeFilter::FILTER_ACCEPT)
                return cand;
            if (propagatedExceptionObject) { m_position = oldPosition; m_referenceNode = oldReferenceNode; return 0; }
        } else {
            // Robustness note here: the filter could potentially yank any nodes out,
            // so we can't keep any state in the temporaries. We hence always rely on
            // m_referenceNode, which would be sync'd by the delete handler
            cand = getNextNode(m_referenceNode.get());
            if (!cand)
                return 0;
            m_referenceNode = cand;
            if (isAccepted(cand.get(), propagatedExceptionObject) == NodeFilter::FILTER_ACCEPT)
                return cand;
            if (propagatedExceptionObject) { m_position = oldPosition; m_referenceNode = oldReferenceNode; return 0; }
        }
    }
    return 0;
}

SharedPtr<NodeImpl> NodeIteratorImpl::previousNode( int &exceptioncode, void* &propagatedExceptionObject )
{
    propagatedExceptionObject = 0;
    if (m_detached) {
	exceptioncode = DOMException::INVALID_STATE_ERR;
	return 0;
    }

    SharedPtr<NodeImpl> oldReferenceNode = m_referenceNode;
    Position oldPosition = m_position;
    while (true) {
        SharedPtr<NodeImpl> cand = m_referenceNode;
        if (m_position == ITER_AFTER_REF) {
            m_position = ITER_BEFORE_REF;
            if (isAccepted(m_referenceNode.get(), propagatedExceptionObject) == NodeFilter::FILTER_ACCEPT)
                return cand;

            if (propagatedExceptionObject) { m_position = oldPosition; m_referenceNode = oldReferenceNode; return 0; }
        } else {
            cand = getPrevNode(m_referenceNode.get());
            if (!cand)
                return 0;

            m_referenceNode = cand;
            if (isAccepted(cand.get(), propagatedExceptionObject) == NodeFilter::FILTER_ACCEPT)
                return cand;
            if (propagatedExceptionObject) { m_position = oldPosition; m_referenceNode = oldReferenceNode; return 0; }
        }
    }
    return 0;
}

NodeImpl* NodeIteratorImpl::getNextNode(NodeImpl* from)
{
    return from->traverseNextNode(m_root.get());
}

NodeImpl* NodeIteratorImpl::getPrevNode(NodeImpl* from)
{
    if (from == m_root)
        return 0;
    else
        return from->traversePreviousNode();
}

NodeImpl *NodeIteratorImpl::getLastNode(NodeImpl *in)
{
    while (NodeImpl* cand = in->lastChild())
        in = cand;
    return in;
}

void NodeIteratorImpl::detach(int &/*exceptioncode*/)
{
    m_doc->detachNodeIterator(this);
    m_detached = true;
}


void NodeIteratorImpl::notifyBeforeNodeRemoval(NodeImpl *removed)
{
    // We care about removals if they happen on the path between us and root,
    // and not if it's just the root being yanked out.
    if (removed == m_root)
	return;

    // Scan up from the reference node to root see if the removed node is there..
    NodeImpl* c;
    for (c = m_referenceNode.get(); c != m_root; c = c->parentNode()) {
        if (c == removed)
            break;
    }

    // If nothing was found, we were unaffected.
    if (c == m_root)
        return;

    // Update the position.. Thankfully we don't have to call filters here.
    if (m_position == ITER_BEFORE_REF) {
        // We were positioned before some node in the removed subtree...
        // We want to be positioned before the first node after the
        // removed subtree.
        NodeImpl* next = getNextNode(getLastNode(removed));
        if (next) {
            m_referenceNode = next;
        } else {
            // There is no where to go after this --- pick a node before the tree,
            // which in preorder is the immediate pred. of the remove root.
            m_position = ITER_AFTER_REF;
            m_referenceNode = getPrevNode(removed);
        }
    } else { // Iterator after ref.
        // We were after some node in the removed subtree, we want to be
        // after the node immediately before it. There -must- be one,
        // since at least the root preceeds it!
        m_referenceNode = getPrevNode(removed);
    }
}

short NodeIteratorImpl::isAccepted(NodeImpl *n, void* &propagatedExceptionObject)
{
  // if XML is implemented we have to check expandEntityRerefences in this function
  if( ( ( 1 << ( n->nodeType()-1 ) ) & m_whatToShow) != 0 )
    {
        if(!m_filter.isNull())
            return m_filter->acceptNode(n, propagatedExceptionObject);
        else
	    return NodeFilter::FILTER_ACCEPT;
    }
    return NodeFilter::FILTER_SKIP;
}

// --------------------------------------------------------------


NodeFilterImpl::NodeFilterImpl()
{
    m_customNodeFilter = 0;
}

NodeFilterImpl::~NodeFilterImpl()
{
    if (m_customNodeFilter)
	m_customNodeFilter->deref();
}

bool NodeFilterImpl::isJSFilter() const
{
    return false;
}

short NodeFilterImpl::acceptNode(const Node &n, void*& /*bindingsException*/)
{
    if (m_customNodeFilter)
	return m_customNodeFilter->acceptNode(n);
    else
	return NodeFilter::FILTER_ACCEPT;
}

void NodeFilterImpl::setCustomNodeFilter(CustomNodeFilter *custom)
{
    m_customNodeFilter = custom;
    if (m_customNodeFilter)
	m_customNodeFilter->ref();
}

CustomNodeFilter *NodeFilterImpl::customNodeFilter()
{
    return m_customNodeFilter;
}

// --------------------------------------------------------------

TreeWalkerImpl::TreeWalkerImpl(NodeImpl *n, long _whatToShow, NodeFilterImpl *f,
                               bool entityReferenceExpansion)
{
  m_currentNode = n;
  m_rootNode = n;
  m_whatToShow = _whatToShow;
  m_filter = f;
  if ( m_filter )
      m_filter->ref();
  m_expandEntityReferences = entityReferenceExpansion;
  m_doc = m_rootNode->document();
  m_doc->ref();
}

TreeWalkerImpl::~TreeWalkerImpl()
{
    m_doc->deref();
    if ( m_filter )
        m_filter->deref();
}

NodeImpl *TreeWalkerImpl::getRoot() const
{
    return m_rootNode.get();
}

unsigned long TreeWalkerImpl::getWhatToShow() const
{
    return m_whatToShow;
}

NodeFilterImpl *TreeWalkerImpl::getFilter() const
{
    return m_filter;
}

bool TreeWalkerImpl::getExpandEntityReferences() const
{
    return m_expandEntityReferences;
}

NodeImpl *TreeWalkerImpl::getCurrentNode() const
{
    return m_currentNode.get();
}

void TreeWalkerImpl::setWhatToShow(long _whatToShow)
{
    // do some testing whether this is an accepted value
    m_whatToShow = _whatToShow;
}

void TreeWalkerImpl::setFilter(NodeFilterImpl *_filter)
{
    m_filter->deref();
    m_filter = _filter;
    if ( m_filter )
        m_filter->ref();
}

void TreeWalkerImpl::setExpandEntityReferences(bool value)
{
    m_expandEntityReferences = value;
}

void TreeWalkerImpl::setCurrentNode( NodeImpl *n, int& exceptionCode  )
{
    if ( n )
        m_currentNode = n;
    else
        exceptionCode = DOMException::NOT_SUPPORTED_ERR;
}

NodeImpl *TreeWalkerImpl::parentNode( void*& filterException )
{
    NodePtr n = getParentNode( m_currentNode, filterException );
    if ( n )
        m_currentNode = n;
    return n.get();
}


NodeImpl *TreeWalkerImpl::firstChild( void*& filterException )
{
    NodePtr n = getFirstChild( m_currentNode, filterException );
    if ( n )
        m_currentNode = n;
    return n.get();
}


NodeImpl *TreeWalkerImpl::lastChild( void*& filterException )
{
    NodePtr n = getLastChild(m_currentNode, filterException);
    if( n )
        m_currentNode = n;
    return n.get();
}

NodeImpl *TreeWalkerImpl::previousSibling( void*& filterException )
{
    NodePtr n = getPreviousSibling( m_currentNode, filterException );
    if( n )
        m_currentNode = n;
    return n.get();
}

NodeImpl *TreeWalkerImpl::nextSibling( void*& filterException )
{
    NodePtr n = getNextSibling( m_currentNode, filterException );
    if( n )
        m_currentNode = n;
    return n.get();
}

NodeImpl *TreeWalkerImpl::nextNode( void*& filterException )
{
    NodePtr n = getNextNode(filterException);
    if( n )
        m_currentNode = n;
    return n.get();
}

NodeImpl *TreeWalkerImpl::previousNode( void*& filterException )
{
    NodePtr n = getPreviousNode(filterException);
    if( n )
        m_currentNode = n;
    return n.get();
}

TreeWalkerImpl::NodePtr TreeWalkerImpl::getPreviousNode( void*& filterException )
{
    filterException = 0;
/* 1. last node in my previous sibling's subtree
 * 2. my previous sibling (special case of the above)
 * 3. my parent
 */

    NodePtr n = getPreviousSibling(m_currentNode, filterException);
    if (filterException) return 0;
    if (n) {
        // Find the last kid in the subtree's preorder traversal, if any,
        // by following the lastChild links.
        NodePtr desc = getLastChild(n, filterException);
        if (filterException) return 0;
        while (desc) {
            n    = desc;
            desc = getLastChild(desc, filterException);
            if (filterException) return 0;
        }
        return n;
    }

    return getParentNode(m_currentNode, filterException);
}

TreeWalkerImpl::NodePtr TreeWalkerImpl::getNextNode( void*& filterException )
{
    filterException = 0;
/*  1. my first child
 *  2. my next sibling
 *  3. my parents sibling, or their parents sibling (loop)
 *  4. not found
 */

    NodePtr n = getFirstChild(m_currentNode, filterException);
    if (filterException) return 0;
    if (n) // my first child
        return n;

    n = getNextSibling(m_currentNode, filterException); // my next sibling
    if (filterException) return 0;
    if (n)
        return n;

    NodePtr parent = getParentNode(m_currentNode, filterException);
    if (filterException) return 0;
    while (parent) // parent's sibling
    {
        n = getNextSibling(parent, filterException);
        if (filterException) return 0;
        if (n)
          return n;

        parent = getParentNode(parent, filterException);
        if (filterException) return 0;
    }
    return 0;
}

short TreeWalkerImpl::isAccepted(TreeWalkerImpl::NodePtr n, void*& filterException)
{
    // if XML is implemented we have to check expandEntityRerefences in this function
    if( ( ( 1 << ( n->nodeType()-1 ) ) & m_whatToShow) != 0 )
    {
        if(m_filter)
            return m_filter->acceptNode(n.get(), filterException);
        else
            return NodeFilter::FILTER_ACCEPT;
    }
    return NodeFilter::FILTER_SKIP;
}

/**
 This is quite a bit more complicated than it would at first seem, due to the following note:
 "Note that the structure of a TreeWalker's filtered view of a document may differ significantly from that of the document itself. For example, a TreeWalker with only SHOW_TEXT specified in its whatToShow parameter would present all the Text nodes as if they were siblings of each other yet had no parent."

 My read of this is that e.g. that when a node is  FILTER_SKIP'd, its children are handled
 as the children of its parent. -M.O.
*/

TreeWalkerImpl::NodePtr TreeWalkerImpl::getParentNode(TreeWalkerImpl::NodePtr n, void*& filterException )
{
    filterException = 0;
    // Already on top of root's subtree tree...
    if (n == m_rootNode)
        return 0;

    // Walk up, to find the first visible node != n, until we run out of
    // document or into the root (which we don't have to be inside of!)
    NodePtr cursor = n->parentNode();
    while (cursor) {
        if (isAccepted(cursor, filterException) == NodeFilter::FILTER_ACCEPT)
            return cursor;
        if (filterException) return 0;

        if (cursor == m_rootNode) // We just checked root -- no where else to go up.
            return 0;
        cursor = cursor->parentNode();
    }

    return 0;
}

TreeWalkerImpl::NodePtr TreeWalkerImpl::getFirstChild(TreeWalkerImpl::NodePtr n, void*& filterException )
{
    filterException = 0;
    NodePtr cursor;
    for (cursor = n->firstChild(); cursor; cursor = cursor->nextSibling()) {
        switch (isAccepted(cursor, filterException)) {
        case NodeFilter::FILTER_ACCEPT:
            return cursor;
        case NodeFilter::FILTER_SKIP: {
            // We ignore this candidate proper, but perhaps it has a kid?
            NodePtr cand = getFirstChild(cursor, filterException);
            if (filterException) return 0;
            if (cand)
                return cand;
            break;
        }
        case NodeFilter::FILTER_REJECT:
            // It effectively doesn't exist, or exception
            if (filterException) return 0;
            break;
        }
        // ### this is unclear: what should happen if we "pass through" the root??
    }

    // Nothing found..
    return 0;
}

TreeWalkerImpl::NodePtr TreeWalkerImpl::getLastChild(TreeWalkerImpl::NodePtr n, void*& filterException )
{
    filterException = 0;
    TreeWalkerImpl::NodePtr cursor;
    for (cursor = n->lastChild(); cursor; cursor = cursor->previousSibling()) {
        switch (isAccepted(cursor, filterException)) {
        case NodeFilter::FILTER_ACCEPT:
            return cursor;
        case NodeFilter::FILTER_SKIP: {
            // We ignore this candidate proper, but perhaps it has a kid?
            TreeWalkerImpl::NodePtr cand = getLastChild(cursor, filterException);
            if (filterException) return 0;
            if (cand)
                return cand;
            break;
        }
        case NodeFilter::FILTER_REJECT:
            // It effectively doesn't exist..
            if (filterException) return 0;
            break;
        }
        // ### this is unclear: what should happen if we "pass through" the root??
    }

    // Nothing found..
    return 0;
}

TreeWalkerImpl::NodePtr TreeWalkerImpl::getNextSibling(TreeWalkerImpl::NodePtr n, void*& filterException )
{
    filterException = 0;

    // If we're the root node, clearly we don't have any siblings.
    if (n == m_rootNode)
        return 0;

    // What would can our siblings be? Well, they can be our actual siblings,
    // or if those are 'skip' their first kid. We may also have to walk up
    // through any skipped nodes.
    NodePtr cursor;
    for (cursor = n->nextSibling(); cursor; cursor = cursor->nextSibling()) {
        switch (isAccepted(cursor, filterException)) {
        case NodeFilter::FILTER_ACCEPT:
            return cursor;
        case NodeFilter::FILTER_SKIP: {
            NodePtr cand = getFirstChild(cursor, filterException);
            if (filterException) return 0;
            if (cand)
                return cand;
            break;
        }
        case NodeFilter::FILTER_REJECT:
            if (filterException) return 0;
            break;
        }
    }

    // Now, we have scanned through all of our parent's kids. Our siblings may also be
    // above if we could have skipped the parent, and are not captured by it.
    NodePtr parent = n->parentNode();
    if (!parent || parent == m_rootNode)
        return 0;

    /* "In particular: If the currentNode becomes part of a subtree that would otherwise have
        been Rejected by the filter, that entire subtree may be added as transient members of the
        logical view. You will be able to navigate within that subtree (subject to all the usual
        filtering) until you move upward past the Rejected ancestor. The behavior is as if the Rejected
        node had only been Skipped (since we somehow wound up inside its subtree) until we leave it;
        thereafter, standard filtering applies.*/
    if (isAccepted(parent, filterException) == NodeFilter::FILTER_ACCEPT)
        return 0;
    if (filterException) return 0;

    return getNextSibling(parent, filterException);
}

TreeWalkerImpl::NodePtr TreeWalkerImpl::getPreviousSibling(TreeWalkerImpl::NodePtr n, void*& filterException )
{
    filterException = 0;
    // See the above for all the comments :-)
    if (n == m_rootNode)
        return 0;

    NodePtr cursor;
    for (cursor = n->previousSibling(); cursor; cursor = cursor->previousSibling()) {
        switch (isAccepted(cursor, filterException)) {
        case NodeFilter::FILTER_ACCEPT:
            return cursor;
        case NodeFilter::FILTER_SKIP: {
            NodePtr cand = getLastChild(cursor, filterException);
            if (filterException) return 0;
            if (cand)
                return cand;
            break;
        }
        case NodeFilter::FILTER_REJECT:
            if (filterException) return 0;
            break;
        }
    }

    NodePtr parent = n->parentNode();
    if (!parent || parent == m_rootNode)
        return 0;

    if (isAccepted(parent, filterException) == NodeFilter::FILTER_ACCEPT)
        return 0;
    if (filterException) return 0;

    return getPreviousSibling(parent, filterException);
}

