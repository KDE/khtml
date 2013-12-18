/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll <knoll@kde.org>
 * Copyright (C) 2000 Frederik Holljen <frederik.holljen@hig.no>
 * Copyright (C) 2001 Peter Kelly <pmk@post.com>
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

#ifndef _DOM2_TraversalImpl_h_
#define _DOM2_TraversalImpl_h_

#include "dom/dom_node.h"
#include "dom/dom_misc.h"
#include "misc/shared.h"
#include "dom/dom2_traversal.h"


namespace DOM {

class NodeImpl;
class DocumentImpl;

class NodeIteratorImpl : public khtml::Shared<NodeIteratorImpl>
{
public:
    NodeIteratorImpl(NodeImpl *_root, unsigned long _whatToShow, NodeFilterImpl* _filter, bool _entityReferenceExpansion);
    ~NodeIteratorImpl();


    NodeImpl *root();
    unsigned long whatToShow();
    NodeFilterImpl* filter();
    bool expandEntityReferences();

    SharedPtr<NodeImpl> nextNode(int &exceptioncode, void* &propagatedExceptionObject);
    SharedPtr<NodeImpl> previousNode(int &exceptioncode, void* &propagatedExceptionObject);
    void detach(int &exceptioncode);

    // pre-order traversal wrt to a node, captured w/in root
    NodeImpl *getNextNode(NodeImpl *in);
    NodeImpl *getPrevNode(NodeImpl *in);
    NodeImpl *getLastNode(NodeImpl *in); //Last node in a tree..

    /**
     * This function has to be called if you delete a node from the
     * document tree and you want the Iterator to react if there
     * are any changes concerning it.
     */
    void notifyBeforeNodeRemoval(NodeImpl *removed);

    short isAccepted(NodeImpl *n, void* &propagatedExceptionObject);
protected:
    SharedPtr<NodeImpl> m_root; // must be kept alive for root() to be safe.
    long m_whatToShow;
    SharedPtr<NodeFilterImpl> m_filter;
    bool m_expandEntityReferences;

    typedef enum { ITER_BEFORE_REF, ITER_AFTER_REF } Position;
    Position m_position;
    SharedPtr<NodeImpl> m_referenceNode;
    bool m_detached;
    DocumentImpl *m_doc;
};

class NodeFilterImpl : public khtml::Shared<NodeFilterImpl>
{
public:
    NodeFilterImpl();
    virtual ~NodeFilterImpl();

    virtual bool  isJSFilter() const;
    virtual short acceptNode(const Node &n, void*& bindingsException);

    void setCustomNodeFilter(CustomNodeFilter *custom);
    CustomNodeFilter *customNodeFilter();
protected:
    CustomNodeFilter *m_customNodeFilter;

};

class TreeWalkerImpl : public khtml::Shared<TreeWalkerImpl>
{
public:
    TreeWalkerImpl();
    TreeWalkerImpl(const TreeWalkerImpl &other);
    TreeWalkerImpl(NodeImpl *n, NodeFilter f);
    TreeWalkerImpl(NodeImpl *n, long _whatToShow, NodeFilterImpl *f,
                   bool entityReferenceExpansion);
    TreeWalkerImpl & operator = (const TreeWalkerImpl &other);


    ~TreeWalkerImpl();

    NodeImpl *getRoot() const;

    unsigned long getWhatToShow() const;

    NodeFilterImpl *getFilter() const;

    bool getExpandEntityReferences() const;

    NodeImpl *getCurrentNode() const;

    void setCurrentNode( NodeImpl *_currentNode, int& exceptionCode );

    NodeImpl *parentNode( void*& filterException );

    NodeImpl *firstChild( void*& filterException );

    NodeImpl *lastChild ( void*& filterException );

    NodeImpl *previousSibling ( void*& filterException );

    NodeImpl *nextSibling( void*& filterException );

    NodeImpl *previousNode( void*& filterException );

    NodeImpl *nextNode( void*& filterException );


    /**
     * Sets which node types are to be presented via the TreeWalker
     */
    void setWhatToShow(long _whatToShow);
    void setFilter(NodeFilterImpl *_filter);
    void setExpandEntityReferences(bool value);

    typedef SharedPtr<NodeImpl> NodePtr; // lazy Maks...

    // These methods attempt to find the next node in given direction from
    // the given reference point, w/o affecting the current node.
    NodePtr getParentNode(NodePtr n, void*& filterException);
    NodePtr getFirstChild(NodePtr n, void*& filterException);
    NodePtr getLastChild(NodePtr n, void*& filterException);
    NodePtr getPreviousSibling(NodePtr n, void*& filterException);
    NodePtr getNextSibling(NodePtr n, void*& filterException);

    NodePtr getNextNode(void*& filterException);
    NodePtr getPreviousNode(void*& filterException);

    short isAccepted(NodePtr n, void*& filterException);

protected:
    /**
     * This attribute determines which node types are presented via
     * the TreeWalker.
     *
     */
    long m_whatToShow;

    /**
     * The filter used to screen nodes.
     *
     */
    NodeFilterImpl *m_filter;

    /**
     * The value of this flag determines whether entity reference
     * nodes are expanded. To produce a view of the document that has
     * entity references expanded and does not expose the entity
     * reference node itself, use the whatToShow flags to hide the
     * entity reference node and set expandEntityReferences to true
     * when creating the iterator. To produce a view of the document
     * that has entity reference nodes but no entity expansion, use
     * the whatToShow flags to show the entity reference node and set
     * expandEntityReferences to true.
     *
     * This is not implemented (always true)
     */
    bool m_expandEntityReferences;

    /**
     * The current node.
     *
     *  The value must not be null. Attempting to set it to null will
     * raise a NOT_SUPPORTED_ERR exception. When setting a node, the
     * whatToShow flags and any Filter associated with the TreeWalker
     * are not checked. The currentNode may be set to any Node of any
     * type.
     *
     */
    SharedPtr<NodeImpl> m_currentNode;
    SharedPtr<NodeImpl> m_rootNode;
    DocumentImpl *m_doc; // always alive as long as the root is...
};


} // namespace

#endif

