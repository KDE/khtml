/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2005, 2009, 2010 Maksim Orlovich (maksim@kde.org)
 *           (C) 2006 Allan Sandfeld Jensen (kde@carewolf.com)
 *           (C) 2007 David Smith (catfish.man@gmail.com)
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
 *
 * The code for ClassNodeListImpl was originally licensed under the following terms
 * (but in this version is available only as above):
 *     Redistribution and use in source and binary forms, with or without
 *     modification, are permitted provided that the following conditions
 *     are met:
 *
 *     1.  Redistributions of source code must retain the above copyright
 *         notice, this list of conditions and the following disclaimer.
 *     2.  Redistributions in binary form must reproduce the above copyright
 *         notice, this list of conditions and the following disclaimer in the
 *         documentation and/or other materials provided with the distribution.
 *     3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *         its contributors may be used to endorse or promote products derived
 *         from this software without specific prior written permission.
 *
 *     THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 *     EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 *     WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 *     DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 *     DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 *     (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *     LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 *     ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *     (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *     THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "dom_nodelistimpl.h"
#include "dom_nodeimpl.h"
#include "dom_docimpl.h"

using namespace DOM;
using namespace khtml;

NodeImpl* DynamicNodeListImpl::item( unsigned long index ) const
{
    unsigned long requestIndex = index;

    m_cache->updateNodeListInfo(m_refNode->document());

    NodeImpl* n = 0;
    bool usedCache = false;
    if (m_cache->current.node) {
        //Compute distance from the requested index to the cache node
        unsigned long cacheDist = qAbs(long(index) - long(m_cache->position));

        if (cacheDist < (unsigned long)index) { //Closer to the cached position
            usedCache = true;
            if (index >= m_cache->position) { //Go ahead
                unsigned long relIndex = index - m_cache->position;
                n = recursiveItem(m_refNode, m_cache->current.node, relIndex);
            } else { //Go backwards
                unsigned long relIndex = m_cache->position - index;
                n = recursiveItemBack(m_refNode, m_cache->current.node, relIndex);
            }
        }
    }

    if (!usedCache)
        n = recursiveItem(m_refNode, m_refNode->firstChild(), index);

    //We always update the cache state, to make starting iteration
    //where it was left off easy.
    m_cache->current.node  = n;
    m_cache->position = requestIndex;
    return n;
}

unsigned long DynamicNodeListImpl::length() const
{
    m_cache->updateNodeListInfo(m_refNode->document());
    if (!m_cache->hasLength) {
        m_cache->length    = calcLength( m_refNode );
        m_cache->hasLength = true;
    }
    return m_cache->length;
}

unsigned long DynamicNodeListImpl::calcLength(NodeImpl *start) const
{
    unsigned long len = 0;
    for(NodeImpl *n = start->firstChild(); n != 0; n = n->nextSibling()) {
        bool recurse = true;
        if (nodeMatches(n, recurse))
                len++;
        if (recurse)
            len+= DynamicNodeListImpl::calcLength(n);
    }

    return len;
}

DynamicNodeListImpl::DynamicNodeListImpl( NodeImpl *n, int type, CacheFactory* factory )
{
    m_refNode = n;
    m_refNode->ref();

    m_cache = m_refNode->document()->acquireCachedNodeListInfo(
                  factory, n, type );
}

DynamicNodeListImpl::~DynamicNodeListImpl()
{
    m_refNode->document()->releaseCachedNodeListInfo(m_cache);
    m_refNode->deref();
}

/**
 Next item in the pre-order walk of tree from node, but not going outside
 absStart
*/
static NodeImpl* helperNext(NodeImpl* node, NodeImpl* absStart)
{
    //Walk up until we wind a sibling to go to.
    while (!node->nextSibling() && node != absStart)
        node = node->parentNode();

    if (node != absStart)
        return node->nextSibling();
    else
        return 0;
}

NodeImpl *DynamicNodeListImpl::recursiveItem ( NodeImpl* absStart, NodeImpl *start, unsigned long &offset ) const
{
    for(NodeImpl *n = start; n != 0; n = helperNext(n, absStart)) {
        bool recurse = true;
        if (nodeMatches(n, recurse))
            if (!offset--)
                return n;

        NodeImpl *depthSearch = recurse ? recursiveItem(n, n->firstChild(), offset) : 0;
        if (depthSearch)
            return depthSearch;
    }

    return 0; // no matching node in this subtree
}

NodeImpl *DynamicNodeListImpl::recursiveItemBack ( NodeImpl* absStart, NodeImpl *start, unsigned long &offset ) const
{
    //### it might be cleaner/faster to split nodeMatches and recursion
    //filtering.
    bool dummy   = true;
    NodeImpl* n  = start;

    do {
        bool recurse = true;

        //Check whether the current node matches.
        if (nodeMatches(n, dummy))
            if (!offset--)
                return n;

        if (n->previousSibling()) {
            //Move to the last node of this whole subtree that we should recurse into
            n       = n->previousSibling();
            recurse = true;

            while (n->lastChild()) {
                (void)nodeMatches(n, recurse);
                if (!recurse)
                   break; //Don't go there
                n = n->lastChild();
            }
        } else {
            //We're done with this whole subtree, so move up
            n = n->parentNode();
        }
    }
    while (n && n != absStart);

    return 0;
}

// ---------------------------------------------------------------------------

DynamicNodeListImpl::Cache* DynamicNodeListImpl::Cache::makeStructuralOnly()
{
   return new Cache(DocumentImpl::TV_Structural); // will check the same ver twice
}

DynamicNodeListImpl::Cache* DynamicNodeListImpl::Cache::makeNameOrID()
{
   return new Cache(DocumentImpl::TV_IDNameHref);
}

DynamicNodeListImpl::Cache* DynamicNodeListImpl::Cache::makeClassName()
{
   return new Cache(DocumentImpl::TV_Class);
}

DynamicNodeListImpl::Cache::Cache(unsigned short relSecondaryVer):relevantSecondaryVer(relSecondaryVer)
{}

DynamicNodeListImpl::Cache::~Cache()
{}

void DynamicNodeListImpl::Cache::clear(DocumentImpl* doc)
{
   hasLength = false;
   current.node = 0;
   version          = doc->domTreeVersion(DocumentImpl::TV_Structural);
   secondaryVersion = doc->domTreeVersion(relevantSecondaryVer);
}

void DynamicNodeListImpl::Cache::updateNodeListInfo(DocumentImpl* doc)
{
    //If version doesn't match, clear
    if (doc->domTreeVersion(DocumentImpl::TV_Structural) != version ||
        doc->domTreeVersion(relevantSecondaryVer) != secondaryVersion)
        clear(doc);
}

// ---------------------------------------------------------------------------
ChildNodeListImpl::ChildNodeListImpl( NodeImpl *n ): DynamicNodeListImpl(n, CHILD_NODES, DynamicNodeListImpl::Cache::makeStructuralOnly)
{}

bool ChildNodeListImpl::nodeMatches( NodeImpl* /*testNode*/, bool& doRecurse ) const
{
    doRecurse = false;
    return true;
}

// ---------------------------------------------------------------------------
TagNodeListImpl::TagNodeListImpl(NodeImpl *n, NamespaceName namespaceName, LocalName localName, PrefixName prefix)
  : DynamicNodeListImpl(n, UNCACHEABLE, DynamicNodeListImpl::Cache::makeStructuralOnly),
    m_namespaceAware(false)
{
    m_namespace = namespaceName;
    m_localName = localName;
    m_prefix = prefix;
}

TagNodeListImpl::TagNodeListImpl( NodeImpl *n, const DOMString &namespaceURI, const DOMString &localName )
  : DynamicNodeListImpl(n, UNCACHEABLE, DynamicNodeListImpl::Cache::makeStructuralOnly),
    m_namespaceAware(true)
{
    if (namespaceURI == "*")
        m_namespace = NamespaceName::fromId(anyNamespace);
    else
        m_namespace = NamespaceName::fromString(namespaceURI);
    if (localName == "*")
        m_localName = LocalName::fromId(anyLocalName);
    else
        m_localName = LocalName::fromString(localName);
    m_prefix = PrefixName::fromId(emptyPrefix);
}


bool TagNodeListImpl::nodeMatches(NodeImpl *testNode, bool& /*doRecurse*/) const
{
    if (testNode->nodeType() != Node::ELEMENT_NODE)
        return false;

    if (m_namespaceAware) {
        return (m_namespace.id() == anyNamespace || m_namespace.id() == namespacePart(testNode->id())) &&
            (m_localName.id() == anyLocalName || m_localName.id() == localNamePart(testNode->id()));
    } else {
        return (m_localName.id() == anyLocalName) || (m_localName.id() == localNamePart(testNode->id()) &&
            m_prefix == static_cast<ElementImpl*>(testNode)->prefixName());
    }
}

// ---------------------------------------------------------------------------
NameNodeListImpl::NameNodeListImpl(NodeImpl *n, const DOMString &t )
  : DynamicNodeListImpl(n, UNCACHEABLE, DynamicNodeListImpl::Cache::makeNameOrID),
    nodeName(t)
{}

bool NameNodeListImpl::nodeMatches( NodeImpl *testNode, bool& /*doRecurse*/ ) const
{
    if ( testNode->nodeType() != Node::ELEMENT_NODE ) return false;
    return static_cast<ElementImpl *>(testNode)->getAttribute(ATTR_NAME) == nodeName;
}

// ---------------------------------------------------------------------------
ClassNodeListImpl::ClassNodeListImpl(NodeImpl* rootNode, const DOMString& classNames)
    : DynamicNodeListImpl(rootNode, UNCACHEABLE, DynamicNodeListImpl::Cache::makeClassName)
{
    m_classNames.parseClassAttribute(classNames, m_refNode->document()->inCompatMode());
}

bool ClassNodeListImpl::nodeMatches(NodeImpl *testNode, bool& doRecurse) const
{
    if (!testNode->isElementNode()) {
        doRecurse = false;
        return false;
    }

    if (!testNode->hasClass())
        return false;

    if (!m_classNames.size())
        return false;

    const ClassNames&  classes = static_cast<ElementImpl*>(testNode)->classNames();
    for (size_t i = 0; i < m_classNames.size(); ++i) {
        if (!classes.contains(m_classNames[i]))
            return false;
    }

    return true;
}

// ---------------------------------------------------------------------------

StaticNodeListImpl::StaticNodeListImpl():
    m_knownNormalization(DocumentOrder) // true vacuously 
{}

StaticNodeListImpl::~StaticNodeListImpl() {}

void StaticNodeListImpl::append(NodeImpl* n)
{
    assert(n);
    m_kids.append(n);
    m_knownNormalization = Unnormalized;
}

NodeImpl* StaticNodeListImpl::item ( unsigned long index ) const
{
    return index < m_kids.size() ? m_kids[index].get() : 0;
}

unsigned long StaticNodeListImpl::length() const
{
    return m_kids.size();
}

static bool nodeLess(const SharedPtr<NodeImpl>& n1, const SharedPtr<DOM::NodeImpl>& n2)
{
    return n1->compareDocumentPosition(n2.get()) & Node::DOCUMENT_POSITION_FOLLOWING;
}

void StaticNodeListImpl::normalizeUpto(NormalizationKind kind)
{
    if (m_knownNormalization == kind || m_knownNormalization == DocumentOrder)
        return;

    if (kind == Unnormalized)
        return;

    // First sort.
    qSort(m_kids.begin(), m_kids.end(), nodeLess);

    // Now get rid of dupes.
    DOM::NodeImpl* last = 0;
    unsigned out = 0;
    for (unsigned in = 0; in < m_kids.size(); ++in) {
        DOM::NodeImpl* cur = m_kids[in].get();
        if (cur != last) {
            m_kids[out] = cur;
            ++out;
        }

        last = cur;
    }
    m_kids.resize(out);

    m_knownNormalization = DocumentOrder;
}

void  StaticNodeListImpl::setKnownNormalization(NormalizationKind kind)
{
    m_knownNormalization = kind;
}

StaticNodeListImpl::NormalizationKind StaticNodeListImpl::knownNormalization() const
{
    return m_knownNormalization;
}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
