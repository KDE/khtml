/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2003, 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
 *           (C) 2007 David Smith (catfish.man@gmail.com)
 *           (C) 2005, 2009 Maksim Orlovich (maksim@kde.org)
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
#ifndef _DOM_NodeListImpl_h_
#define _DOM_NodeListImpl_h_

#include "dom/dom_string.h"
#include "misc/shared.h"
#include "misc/htmlnames.h"
#include "ClassNames.h"

namespace DOM {

class NodeImpl;
class DocumentImpl;

class NodeListImpl : public khtml::Shared<NodeListImpl>
{
public:
    // DOM methods & attributes for NodeList
    virtual unsigned long length() const = 0;
    virtual NodeImpl *item ( unsigned long index ) const = 0;
    
    virtual ~NodeListImpl() {}
};

class DynamicNodeListImpl : public NodeListImpl
{
public:
    //Type of the item stored in the cache.
    enum Type {
        UNCACHEABLE, // Too complex to remember in document -- we still track the position
        CHILD_NODES,
        LAST_NODE_LIST = CHILD_NODES
    };

    struct CacheKey
    {
        NodeImpl* baseNode;
        int       type;

        CacheKey(): type(UNCACHEABLE) {}

        CacheKey(NodeImpl* _baseNode, int _type):
            baseNode(_baseNode), type(_type)
        {}

        int hash() const
        {
            return int(reinterpret_cast<quintptr>(baseNode) >> 2) ^
                       (unsigned(type) << 26);
        }

        bool operator==(const CacheKey& other) const
        {
            return baseNode == other.baseNode &&
                   type     == other.type;
        }
    };

    struct Cache: public khtml::Shared<Cache>
    {
        static Cache* makeStructuralOnly();
        static Cache* makeNameOrID();
        static Cache* makeClassName();

        Cache(unsigned short relSecondaryVer);

        CacheKey key;//### We must store this in here due to QCache in Qt3 sucking

        unsigned int version; // structural version.
        unsigned int secondaryVersion;
        union
        {
            NodeImpl*    node;
            unsigned int index;
        } current;
        unsigned int position;
        unsigned int length;
        bool         hasLength;
        unsigned short relevantSecondaryVer;

        void updateNodeListInfo(DocumentImpl* doc);

        virtual void clear(DocumentImpl* doc);
        virtual ~Cache();
    };

    typedef Cache* CacheFactory();

    DynamicNodeListImpl(NodeImpl* node, int type, CacheFactory* factory);
    virtual ~DynamicNodeListImpl();

    // DOM methods & attributes for NodeList
    virtual unsigned long length() const;
    virtual NodeImpl *item ( unsigned long index ) const;

    // Other methods (not part of DOM)

protected:
    virtual unsigned long calcLength(NodeImpl *start) const;
    // helper functions for searching all ElementImpls in a tree

    NodeImpl *recursiveItem    ( NodeImpl* absStart, NodeImpl *start, unsigned long &offset ) const;
    NodeImpl *recursiveItemBack( NodeImpl* absStart, NodeImpl *start, unsigned long &offset ) const;

    // Override this to determine what nodes to return. Set doRecurse to
    // false if the children of this node do not need to be entered.
    virtual bool nodeMatches( NodeImpl *testNode, bool& doRecurse ) const = 0;

    NodeImpl*      m_refNode;
    mutable Cache* m_cache;
};

class ChildNodeListImpl : public DynamicNodeListImpl
{
public:

    ChildNodeListImpl( NodeImpl *n );

protected:
    virtual bool nodeMatches( NodeImpl *testNode, bool& doRecurse ) const;
};


/**
 * NodeList which lists all Nodes in a document with a given tag name
 */
class TagNodeListImpl : public DynamicNodeListImpl
{
public:
    TagNodeListImpl(NodeImpl *n, NamespaceName namespaceName, LocalName localName, PrefixName);
    TagNodeListImpl(NodeImpl *n, const DOMString &namespaceURI, const DOMString &localName);

    // Other methods (not part of DOM)

protected:
    virtual bool nodeMatches(NodeImpl *testNode, bool& doRecurse) const;
    NamespaceName m_namespace;
    LocalName m_localName;
    PrefixName m_prefix;

    bool m_namespaceAware;
};


/**
 * NodeList which lists all Nodes in a Element with a given "name=" tag
 */
class NameNodeListImpl : public DynamicNodeListImpl
{
public:
    NameNodeListImpl( NodeImpl *doc, const DOMString &t );

    // Other methods (not part of DOM)

protected:
    virtual bool nodeMatches( NodeImpl *testNode, bool& doRecurse ) const;

    DOMString nodeName;
};

/** For getElementsByClassName */
class ClassNodeListImpl : public DynamicNodeListImpl {
public:
    ClassNodeListImpl(NodeImpl* rootNode, const DOMString& classNames);

private:
    virtual bool nodeMatches(NodeImpl *testNode, bool& doRecurse) const;

    ClassNames m_classNames;
};


class StaticNodeListImpl : public NodeListImpl
{
public:
    StaticNodeListImpl();
    ~StaticNodeListImpl();

    // Implementation of the NodeList API
    virtual unsigned long length() const;
    virtual NodeImpl *item ( unsigned long index ) const;
    

    // Methods specific to StaticNodeList
    void append(NodeImpl* n);
    NodeImpl* first() { return item(0); }
    bool      isEmpty() const { return length() == 0; }

    // For XPath, we may have collection nodes that are either ordered
    // by the axis, are in random order, or strongly normalized. We represent
    // this knowledge by this enum
    enum NormalizationKind {
       Unnormalized,
       AxisOrder,
       DocumentOrder
    };

    // If the list isn't up to the given level of normalization, put it into
    // document order. Note that if we're asked for AxisOrder but have
    // DocumentOrder already, it's left to be.
    void normalizeUpto(NormalizationKind kind);

    // Reports to list outside knowledge of how normalized the data currently is.
    void setKnownNormalization(NormalizationKind kind);

    NormalizationKind knownNormalization() const;
private:
    WTF::Vector<SharedPtr<NodeImpl> > m_kids;
    NormalizationKind                 m_knownNormalization;
};

} //namespace
#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
