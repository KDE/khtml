/*
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
 * This file includes excerpts from the Document Object Model (DOM)
 * Level 2 Specification (Candidate Recommendation)
 * http://www.w3.org/TR/2000/CR-DOM-Level-2-20000510/
 * Copyright © 2000 W3C® (MIT, INRIA, Keio), All Rights Reserved.
 *
 */
#ifndef _dom2_traversal_h_
#define _dom2_traversal_h_

#include <khtml_export.h>
#include <dom/dom_node.h>
#include <dom/dom_misc.h>

namespace DOM {
class Node;
class NodeFilter;
class NodeIteratorImpl;
class NodeFilterImpl;
class TreeWalkerImpl;
class CustomNodeFilter;
class CustomNodeFilterImpl;

/**
 * NodeIterators are used to step through a set of nodes, e.g. the set
 * of nodes in a NodeList, the document subtree governed by a
 * particular node, the results of a query, or any other set of nodes.
 * The set of nodes to be iterated is determined by the implementation
 * of the NodeIterator. DOM Level 2 specifies a single NodeIterator
 * implementation for document-order traversal of a document subtree.
 * Instances of these iterators are created by calling
 * DocumentTraversal.createNodeIterator().
 *
 *  Any Iterator that returns nodes may implement the
 * \c NodeIterator interface. Users and vendor libraries may also
 * choose to create Iterators that implement the \c NodeIterator
 * interface.
 *
 */
class KHTML_EXPORT NodeIterator
{
    friend class NodeIteratorImpl;
    friend class Document;
public:
    NodeIterator();
    NodeIterator(const NodeIterator &other);

    NodeIterator & operator = (const NodeIterator &other);

    ~NodeIterator();

    /**
     * The root node of the NodeIterator, as specified when it was created.
     */
    Node root();

    /**
    * This attribute determines which node types are presented via the
    * iterator. The available set of constants is defined in the NodeFilter
    * interface. Nodes not accepted by whatToShow will be skipped, but their
    * children may still be considered. Note that this skip takes precedence
    * over the filter, if any.
    */
    unsigned long whatToShow();

    /**
     * The NodeFilter used to screen nodes.
     */
    NodeFilter filter();

    /**
     * The value of this flag determines whether the children of entity
     * reference nodes are visible to the iterator. If false, they and
     * their descendents will be rejected. Note that this rejection takes
     * precedence over whatToShow and the filter. Also note that this is
     * currently the only situation where NodeIterators may reject a complete
     * subtree rather than skipping individual nodes.
     *
     * To produce a view of the document that has entity references expanded
     * and does not expose the entity reference node itself, use the whatToShow
     * flags to hide the entity reference node and set expandEntityReferences to
     * true when creating the iterator. To produce a view of the document that
     * has entity reference nodes but no entity expansion, use the whatToShow
     * flags to show the entity reference node and set expandEntityReferences to
     * false.
     */
    bool expandEntityReferences();

    /**
     * Returns the next node in the set and advances the position of
     * the Iterator in the set. After a NodeIterator is created, the
     * first call to nextNode() returns the first node in the set.
     *
     * @return The next \c Node in the set being iterated
     * over, or \c null if there are no more members in
     * that set.
     *
     * @exception Exceptions from user code
     * Any exceptions raised by a user-written Filter will propagate
     * through.
     *
     */
    Node nextNode();

    /**
     * Returns the previous node in the set and moves the position of
     * the Iterator backwards in the set.
     *
     * @return The previous \c Node in the set being
     * iterated over, or \c null if there are no more
     * members in that set.
     *
     * @exception Exceptions from user code
     * Any exceptions raised by a user-written Filter will propagate
     * through.
     *
     */
    Node previousNode();

    /**
     * Detaches the NodeIterator from the set which it iterated over,
     * releasing any computational resources and placing the iterator in the
     * INVALID state. After detach has been invoked, calls to nextNode or
     * previousNode will raise the exception INVALID_STATE_ERR.
     */
    void detach();

    /**
     * @internal
     * not part of the DOM
     */
    NodeIteratorImpl *handle() const;
    bool isNull() const;

protected:
    NodeIteratorImpl *impl;
    NodeIterator(NodeIteratorImpl *i);
};


/**
 * Filters are objects that know how to "filter out" nodes. If an
 * Iterator or \c TreeWalker is given a filter, before it
 * returns the next node, it applies the filter. If the filter says to
 * accept the node, the Iterator returns it; otherwise, the Iterator
 * looks for the next node and pretends that the node that was
 * rejected was not there.
 *
 *  The DOM does not provide any filters. Filter is just an interface
 * that users can implement to provide their own filters.
 *
 *  Filters do not need to know how to iterate, nor do they need to
 * know anything about the data structure that is being iterated. This
 * makes it very easy to write filters, since the only thing they have
 * to know how to do is evaluate a single node. One filter may be used
 * with a number of different kinds of Iterators, encouraging code
 * reuse.
 *
 * To create your own custom NodeFilter, define a subclass of
 * CustomNodeFilter which overrides the acceptNode() method and assign
 * an instance of it to the NodeFilter. For more details see the
 * CustomNodeFilter class
 */
class KHTML_EXPORT NodeFilter
{
    friend class NodeIterator;
    friend class NodeIteratorImpl;
    friend class TreeWalker;
    friend class TreeWalkerImpl;
    friend class NodeFilterImpl;
public:
    NodeFilter();
    NodeFilter(const NodeFilter &other);
    NodeFilter(NodeFilterImpl *i);

    virtual NodeFilter & operator = (const NodeFilter &other);

    virtual ~NodeFilter();
    /**
     * The following constants are returned by the acceptNode()
     * method:
     *
     */
    enum AcceptCode {
        FILTER_ACCEPT = 1,
        FILTER_REJECT = 2,
        FILTER_SKIP   = 3
    };

    /**
     * These are the available values for the whatToShow parameter.
     * They are the same as the set of possible types for Node, and
     * their values are derived by using a bit position corresponding
     * to the value of NodeType for the equivalent node type.
     *
     */
    enum ShowCode {
        SHOW_ALL                       = 0xFFFFFFFF,
        SHOW_ELEMENT                   = 0x00000001,
        SHOW_ATTRIBUTE                 = 0x00000002,
        SHOW_TEXT                      = 0x00000004,
        SHOW_CDATA_SECTION             = 0x00000008,
        SHOW_ENTITY_REFERENCE          = 0x00000010,
        SHOW_ENTITY                    = 0x00000020,
        SHOW_PROCESSING_INSTRUCTION    = 0x00000040,
        SHOW_COMMENT                   = 0x00000080,
        SHOW_DOCUMENT                  = 0x00000100,
        SHOW_DOCUMENT_TYPE             = 0x00000200,
        SHOW_DOCUMENT_FRAGMENT         = 0x00000400,
        SHOW_NOTATION                  = 0x00000800
    };

    /**
     * Test whether a specified node is visible in the logical view of
     * a TreeWalker or NodeIterator. This function will be called by
     * the implementation of TreeWalker and NodeIterator; it is not
     * intended to be called directly from user code.
     *
     * @param n The node to check to see if it passes the filter or
     * not.
     *
     * @return a constant to determine whether the node is accepted,
     * rejected, or skipped, as defined <a
     * href="#Traversal-NodeFilter-acceptNode-constants"> above </a> .
     *
     */
    virtual short acceptNode (const Node &n);

    /**
     * @internal
     * not part of the DOM
     */
    virtual NodeFilterImpl *handle() const;
    virtual bool isNull() const;

    void setCustomNodeFilter(CustomNodeFilter *custom);
    CustomNodeFilter *customNodeFilter();
    static NodeFilter createCustom(CustomNodeFilter *custom);

protected:
    NodeFilterImpl *impl;
};

/**
 * CustomNodeFilter can be used to define your own NodeFilter for use
 * with NodeIterators and TreeWalkers. You can create a custom filter
 * by doing the following:
 *
 * class MyCustomNodeFilter {
 *  .....
 *  virtual short acceptNode (const Node &n);
 *  .....
 * }
 *
 * Then in your program:
 *
 * short MyCustomNodeFilter::acceptNode (const Node &n)
 * {
 *   if (condition)
 *     return NodeFilter::FILTER_ACCEPT;
 *   else
 *    ....
 * }
 *
 *
 * MyCustomFilter *filter = new MyCustomFilter();
 * NodeFilter nf = NodeFilter::createCustom(filter);
 * NodeIterator ni = document.createNodeIterator(document,NodeFilter.SHOW_ALL,nf,false);
 *
 * The default implementation of acceptNode() returns NodeFilter::FILTER_ACCEPT
 * for all nodes.
 *
 */

class KHTML_EXPORT CustomNodeFilter : public DomShared {
public:
    CustomNodeFilter();
    virtual ~CustomNodeFilter();
    virtual short acceptNode (const Node &n);
    virtual bool isNull();

    /**
     * @internal
     * not part of the DOM
     *
     * Returns a name specifying the type of custom node filter. Useful for checking
     * if an custom node filter is of a particular sublass.
     *
     */
    virtual DOMString customNodeFilterType();

protected:
    /**
     * @internal
     * Reserved. Do not use in your subclasses.
     */
    CustomNodeFilterImpl *impl;
};

/**
 * \c TreeWalker objects are used to navigate a document
 * tree or subtree using the view of the document defined by its
 * \c whatToShow flags and any filters that are defined
 * for the \c TreeWalker . Any function which performs
 * navigation using a \c TreeWalker will automatically
 * support any view defined by a \c TreeWalker .
 *
 *  Omitting nodes from the logical view of a subtree can result in a
 * structure that is substantially different from the same subtree in
 * the complete, unfiltered document. Nodes that are siblings in the
 * TreeWalker view may be children of different, widely separated
 * nodes in the original view. For instance, consider a Filter that
 * skips all nodes except for Text nodes and the root node of a
 * document. In the logical view that results, all text nodes will be
 * siblings and appear as direct children of the root node, no matter
 * how deeply nested the structure of the original document.
 *
 */
class KHTML_EXPORT TreeWalker
{
    friend class Document;
    friend class TreeWalkerImpl;
public:
    TreeWalker();
    TreeWalker(const TreeWalker &other);

    TreeWalker & operator = (const TreeWalker &other);

    ~TreeWalker();


    /**
     * The root node of the TreeWalker, as specified when it was created.
     */
    Node root();

    /**
     * This attribute determines which node types are presented via the
     * TreeWalker. The available set of constants is defined in the NodeFilter
     * interface. Nodes not accepted by whatToShow will be skipped, but their
     * children may still be considered. Note that this skip takes precedence
     * over the filter, if any.
     */
    unsigned long whatToShow();

    /**
     * The filter used to screen nodes.
     */
    NodeFilter filter();

    /**
     * The value of this flag determines whether the children of entity
     * reference nodes are visible to the TreeWalker. If false, they and their
     * descendents will be rejected. Note that this rejection takes precedence
     * over whatToShow and the filter, if any.
     *
     * To produce a view of the document that has entity references expanded
     * and does not expose the entity reference node itself, use the whatToShow
     * flags to hide the entity reference node and set expandEntityReferences
     * to true when creating the TreeWalker. To produce a view of the document
     * that has entity reference nodes but no entity expansion, use the
     * whatToShow flags to show the entity reference node and set
     * expandEntityReferences to false.
     */
    bool expandEntityReferences();

    /**
     * The node at which the TreeWalker is currently positioned.
     * Alterations to the DOM tree may cause the current node to no longer be
     * accepted by the TreeWalker's associated filter. currentNode may also be
     * explicitly set to any node, whether or not it is within the subtree
     * specified by the root node or would be accepted by the filter and
     * whatToShow flags. Further traversal occurs relative to currentNode even
     * if it is not part of the current view, by applying the filters in the
     * requested direction; if no traversal is possible, currentNode is not changed.
     *
     * @exception DOMException
     * NOT_SUPPORTED_ERR: Raised if an attempt is made to set currentNode to null.
     */
    Node currentNode();

    /**
     * see currentNode
     */
    void setCurrentNode(const Node &_currentNode);

    /**
     * Moves to and returns the parent node of the current node. If
     * there is no parent node, or if the current node is the root
     * node from which this TreeWalker was created, retains the
     * current position and returns null.
     *
     * @return The new parent node, or null if the current node has no
     * parent in the TreeWalker's logical view.
     *
     * @exception Exceptions from user code
     * Any exceptions raised by a user-written Filter will propagate
     * through.
     *
     */
    Node parentNode();

    /**
     * Moves the \c TreeWalker to the first child of the
     * current node, and returns the new node. If the current node has
     * no children, returns \c null , and retains the
     * current node.
     *
     * @return The new node, or \c null if the current
     * node has no children.
     *
     * @exception Exceptions from user code
     * Any exceptions raised by a user-written Filter will propagate
     * through.
     *
     */
    Node firstChild();

    /**
     * Moves the \c TreeWalker to the last child of the
     * current node, and returns the new node. If the current node has
     * no children, returns \c null , and retains the
     * current node.
     *
     * @return The new node, or \c null if the current
     * node has no children.
     *
     * @exception Exceptions from user code
     * Any exceptions raised by a user-written Filter will propagate
     * through.
     *
     */
    Node lastChild();

    /**
     * Moves the \c TreeWalker to the previous sibling of
     * the current node, and returns the new node. If the current node
     * has no previous sibling, returns \c null , and
     * retains the current node.
     *
     * @return The new node, or \c null if the current
     * node has no previous sibling.
     *
     * @exception Exceptions from user code
     * Any exceptions raised by a user-written Filter will propagate
     * through.
     *
     */
    Node previousSibling();

    /**
     * Moves the \c TreeWalker to the next sibling of the
     * current node, and returns the new node. If the current node has
     * no next sibling, returns \c null , and retains the
     * current node.
     *
     * @return The new node, or \c null if the current
     * node has no next sibling.
     *
     * @exception Exceptions from user code
     * Any exceptions raised by a user-written Filter will propagate
     * through.
     *
     */
    Node nextSibling();

    /**
     * Moves the \c TreeWalker to the previous node in
     * document order relative to the current node, and returns the
     * new node. If the current node has no previous node, returns
     * \c null , and retains the current node.
     *
     * @return The new node, or \c null if the current
     * node has no previous node.
     *
     * @exception Exceptions from user code
     * Any exceptions raised by a user-written Filter will propagate
     * through.
     *
     */
    Node previousNode();

    /**
     * Moves the \c TreeWalker to the next node in
     * document order relative to the current node, and returns the
     * new node. If the current node has no next node, returns
     * \c null , and retains the current node.
     *
     * @return The new node, or \c null if the current
     * node has no next node.
     *
     * @exception Exceptions from user code
     * Any exceptions raised by a user-written Filter will propagate
     * through.
     *
     */
    Node nextNode();

    /**
     * @internal
     * not part of the DOM
     */
    TreeWalkerImpl *handle() const;
    bool isNull() const;

protected:
    TreeWalker(TreeWalkerImpl *i);
    TreeWalkerImpl *impl;
};


} // namespace

#endif
