/**
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright 1999 Lars Knoll (knoll@kde.org)
 * Copyright 2000 Frederik Holljen (frederik.holljen@hig.no)
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

#include "dom/dom_exception.h"
#include "dom/dom_string.h"
#include "xml/dom_nodeimpl.h"
#include "xml/dom2_traversalimpl.h"

using namespace DOM;


NodeIterator::NodeIterator()
{
    impl = 0;
}

NodeIterator::NodeIterator(const NodeIterator &other)
{
    impl = other.impl;
    if (impl) impl->ref();
}

NodeIterator::NodeIterator(NodeIteratorImpl *i)
{
    impl = i;
    if (impl) impl->ref();
}

NodeIterator &NodeIterator::operator = (const NodeIterator &other)
{
    if ( impl != other.impl ) {
	if (impl) impl->deref();
	impl = other.impl;
	if (impl) impl->ref();
    }
    return *this;
}

NodeIterator::~NodeIterator()
{
    if (impl) impl->deref();
}

Node NodeIterator::root()
{
    if (impl) return impl->root();
    return 0;
}

unsigned long NodeIterator::whatToShow()
{
    if (impl) return impl->whatToShow();
    return 0;
}

NodeFilter NodeIterator::filter()
{
    if (impl) return impl->filter();
    return 0;
}

bool NodeIterator::expandEntityReferences()
{
    if (impl) return impl->expandEntityReferences();
    return 0;
}

Node NodeIterator::nextNode(  )
{
    void* dummy;
    if (!impl)
	throw DOMException(DOMException::INVALID_STATE_ERR);

    int exceptioncode = 0;
    SharedPtr<NodeImpl> r = impl->nextNode(exceptioncode, dummy);
    if (exceptioncode)
	throw DOMException(exceptioncode);
    return r.get();
}

Node NodeIterator::previousNode(  )
{
    void* dummy; // ### rely on C++ exception propagation --- might not be safe
                 // we could probably proxy the DOM exceptions at the very least
    if (!impl)
	throw DOMException(DOMException::INVALID_STATE_ERR);

    int exceptioncode = 0;
    SharedPtr<NodeImpl> r = impl->previousNode(exceptioncode, dummy);
    if (exceptioncode)
	throw DOMException(exceptioncode);
    return r.get();
}

void NodeIterator::detach()
{
    if (!impl)
	throw DOMException(DOMException::INVALID_STATE_ERR);

    int exceptioncode = 0;
    impl->detach(exceptioncode);
    if (exceptioncode)
	throw DOMException(exceptioncode);
}

NodeIteratorImpl *NodeIterator::handle() const
{
    return impl;
}

bool NodeIterator::isNull() const
{
    return (impl == 0);
}

// -----------------------------------------------------------

NodeFilter::NodeFilter()
{
    impl = 0;
}

NodeFilter::NodeFilter(const NodeFilter &other)
{
    impl = other.impl;
    if (impl) impl->ref();
}

NodeFilter::NodeFilter(NodeFilterImpl *i)
{
    impl = i;
    if (impl) impl->ref();
}

NodeFilter &NodeFilter::operator = (const NodeFilter &other)
{
    if ( impl != other.impl ) {
	if (impl) impl->deref();
	impl = other.impl;
	if (impl) impl->ref();
    }
    return *this;
}

NodeFilter::~NodeFilter()
{
    if (impl) impl->deref();
}

short NodeFilter::acceptNode(const Node &n)
{
    void* dummy;
    if (impl) return impl->acceptNode(n, dummy);
    return 0;
}

void NodeFilter::setCustomNodeFilter(CustomNodeFilter *custom)
{
    if (impl) impl->setCustomNodeFilter(custom);
}

CustomNodeFilter *NodeFilter::customNodeFilter()
{
    if (impl) return impl->customNodeFilter();
    return 0;
}

NodeFilterImpl *NodeFilter::handle() const
{
    return impl;
}

bool NodeFilter::isNull() const
{
    return (impl == 0);
}

NodeFilter NodeFilter::createCustom(CustomNodeFilter *custom)
{
    NodeFilterImpl *i = new NodeFilterImpl();
    i->setCustomNodeFilter(custom);
    return i;
}

// --------------------------------------------------------------
CustomNodeFilter::CustomNodeFilter()
{
    impl = 0;
}

CustomNodeFilter::~CustomNodeFilter()
{
}

short CustomNodeFilter::acceptNode (const Node &/*n*/)
{
    return NodeFilter::FILTER_ACCEPT;
}

bool CustomNodeFilter::isNull()
{
    return false;
}

DOMString CustomNodeFilter::customNodeFilterType()
{
    return "";
}

// --------------------------------------------------------------

TreeWalker::TreeWalker()
{
    impl = 0;
}

TreeWalker::TreeWalker(const TreeWalker &other)
{
    impl = other.impl;
    if (impl) impl->ref();
}

TreeWalker::TreeWalker(TreeWalkerImpl *i)
{
    impl = i;
    if (impl) impl->ref();
}

TreeWalker & TreeWalker::operator = (const TreeWalker &other)
{
    if ( impl != other.impl ) {
	if (impl) impl->deref();
	impl = other.impl;
	if (impl) impl->ref();
    }

    return *this;
}

TreeWalker::~TreeWalker()
{
    if (impl) impl->deref();
}

Node TreeWalker::root()
{
    if (impl) return impl->getRoot();
    return 0;
}

unsigned long TreeWalker::whatToShow()
{
    if (impl) return impl->getWhatToShow();
    return 0;
}

NodeFilter TreeWalker::filter()
{
    if (impl) return impl->getFilter();
    return 0;
}

bool TreeWalker::expandEntityReferences()
{
    if (impl) return impl->getExpandEntityReferences();
    return false;
}

Node TreeWalker::currentNode()
{
    if (impl) return impl->getCurrentNode();
    return 0;
}

void TreeWalker::setCurrentNode(const Node& _currentNode)
{
    int exceptionCode = 0;
    if (impl) impl->setCurrentNode(_currentNode.handle(), exceptionCode);
    if (exceptionCode)
        throw DOMException(exceptionCode);
}

Node TreeWalker::parentNode()
{
    void *dummy;
    if (impl) return impl->parentNode(dummy);
    return 0;
}

Node TreeWalker::firstChild()
{
    void *dummy;
    if (impl) return impl->firstChild(dummy);
    return 0;
}

Node TreeWalker::lastChild()
{
    void *dummy;
    if (impl) return impl->lastChild(dummy);
    return 0;
}

Node TreeWalker::previousSibling()
{
    void *dummy;
    if (impl) return impl->previousSibling(dummy);
    return 0;
}

Node TreeWalker::nextSibling()
{
    void *dummy;
    if (impl) return impl->nextSibling(dummy);
    return 0;
}

Node TreeWalker::previousNode()
{
    void *dummy;
    if (impl) return impl->previousNode(dummy);
    return 0;
}

Node TreeWalker::nextNode()
{
    void *dummy;
    if (impl) return impl->nextNode(dummy);
    return 0;
}

TreeWalkerImpl *TreeWalker::handle() const
{
    return impl;
}

bool TreeWalker::isNull() const
{
    return (impl == 0);
}

