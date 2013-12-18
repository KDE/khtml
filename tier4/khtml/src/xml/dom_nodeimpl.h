/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2003, 2004, 2005, 2006, 2007 Apple Inc. All rights reserved.
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
#ifndef _DOM_NodeImpl_h_
#define _DOM_NodeImpl_h_

#include "dom/dom_misc.h"
#include "dom/dom_string.h"
#include "dom/dom_node.h"
#include "misc/helper.h"
#include "misc/shared.h"
#include "misc/idstring.h"
#include "wtf/PassRefPtr.h"
#include "misc/htmlnames.h"
#include "dom/QualifiedName.h"
#include "xml/dom2_eventsimpl.h"

template <class type> class QList;
class KHTMLView;
class QRect;
class QMouseEvent;
class QKeyEvent;

namespace khtml {
    class RenderStyle;
    class RenderObject;
    class RenderArena;
    class RenderPosition;
}

namespace DOM {

class NodeListImpl;
class NamedNodeMapImpl;
class DocumentImpl;
class ElementImpl;
class RegisteredEventListener;
class EventImpl;
class Selection;

class NodeImpl : public EventTargetImpl
{
    friend class DocumentImpl;
public:
    NodeImpl(DocumentImpl *doc);
    virtual ~NodeImpl();

    //stuff for WebCore DOM & SVG
    virtual bool hasTagName(const QualifiedName& /*name*/) const { return false; }

    // EventTarget
    virtual Type eventTargetType() const { return DOM_NODE; }
    // covariant override
    NodeImpl* parent() const { return parentNode(); }

    // DOM methods & attributes for Node
    virtual DOMString nodeName() const;
    virtual DOMString nodeValue() const;
    virtual void setNodeValue( const DOMString &_nodeValue, int &exceptioncode );
    virtual unsigned short nodeType() const;
    NodeImpl *parentNode() const { return static_cast<NodeImpl*>(m_parent); }
    NodeImpl *previousSibling() const { return m_previous; }
    NodeImpl *nextSibling() const { return m_next; }
    virtual NodeListImpl *childNodes();
    virtual NodeImpl *firstChild() const;
    virtual NodeImpl *lastChild() const;

    virtual bool hasAttributes() const;
    //OwnerDocument as specified by the DOM. Do not use for other purposes, it's weird!
    DocumentImpl *ownerDocument() const;
    NodeListImpl* getElementsByTagName  ( const DOMString &tagName );
    NodeListImpl* getElementsByTagNameNS( const DOMString &namespaceURI, const DOMString &localName );

    // HTML 5
    NodeListImpl* getElementsByClassName(const DOMString &name);
    
    // DOM3. See the wrapper (DOM::Node for the constants used in the return value
    unsigned compareDocumentPosition(const DOM::NodeImpl* other);

    // WA Selector API L1. It's specified only for some types, but we provide it here
    WTF::PassRefPtr<DOM::ElementImpl>  querySelector(const DOM::DOMString& query, int& ec);
    WTF::PassRefPtr<DOM::NodeListImpl> querySelectorAll(const DOM::DOMString& query, int& ec);

    // insertBefore, replaceChild and appendChild also close newChild
    // unlike the speed optimized addChild (which is used by the parser)
    virtual NodeImpl *insertBefore ( NodeImpl *newChild, NodeImpl *refChild, int &exceptioncode );

    /* These two methods may delete the old node, so make sure to reference it if you need it */
    virtual void replaceChild ( NodeImpl *newChild, NodeImpl *oldChild, int &exceptioncode );
    virtual void removeChild ( NodeImpl *oldChild, int &exceptioncode );
    virtual NodeImpl *appendChild ( NodeImpl *newChild, int &exceptioncode );
    virtual void remove(int &exceptioncode);
    virtual bool hasChildNodes (  ) const;
    virtual WTF::PassRefPtr<NodeImpl> cloneNode ( bool deep ) = 0;
    virtual DOMString localName() const;
    virtual DOMString prefix() const;
    virtual DOMString namespaceURI() const;
    virtual void setPrefix(const DOMString &_prefix, int &exceptioncode );
    void normalize ();
    static bool isSupported(const DOMString &feature, const DOMString &version);

    // Other methods (not part of DOM)
    virtual bool isElementNode() const { return false; }
    virtual bool isHTMLElement() const { return false; }
    virtual bool isAttributeNode() const { return false; }
    virtual bool isTextNode() const { return false; }
    virtual bool isDocumentNode() const { return false; }
    virtual bool isXMLElementNode() const { return false; }
    virtual bool isGenericFormElement() const { return false; }
    virtual bool containsOnlyWhitespace() const { return false; }
    bool isBlockFlow() const;

    // methods for WebCore api compat (SVG)
    virtual bool isSVGElement() const { return false; }
    virtual bool isShadowNode() const { return false; }
    virtual NodeImpl* shadowParentNode() { return 0; }

    DOMString textContent() const;
    void setTextContent(const DOMString& text, int& ec);

    // helper functions not being part of the DOM
    // Attention: they assume that the caller did the consistency checking!
    void setPreviousSibling(NodeImpl *previous) { m_previous = previous; }
    void setNextSibling(NodeImpl *next) { m_next = next; }

    virtual void setFirstChild(NodeImpl *child);
    virtual void setLastChild(NodeImpl *child);

    /** (Not part of the official DOM)
     * Returns the next leaf node.
     *
     * Using this function delivers leaf nodes as if the whole DOM tree
     * were a linear chain of its leaf nodes.
     * @return next leaf node or 0 if there are no more.
     */
    NodeImpl *nextLeafNode() const;

    /** (Not part of the official DOM)
     * Returns the previous leaf node.
     *
     * Using this function delivers leaf nodes as if the whole DOM tree
     * were a linear chain of its leaf nodes.
     * @return previous leaf node or 0 if there are no more.
     */
    NodeImpl *previousLeafNode() const;

    bool isEditableBlock() const;
    ElementImpl *enclosingBlockFlowElement() const;
    ElementImpl *rootEditableElement() const;

    bool inSameRootEditableElement(NodeImpl *);
    bool inSameContainingBlockFlowElement(NodeImpl *);

    khtml::RenderPosition positionForCoordinates(int x, int y) const;
    bool isPointInsideSelection(int x, int y, const Selection &) const;

    // used by the parser. Doesn't do as many error checkings as
    // appendChild(), and returns the node into which will be parsed next.
    virtual NodeImpl *addChild(NodeImpl *newChild);

    typedef quint32 Id;
    // id() is used to easily and exactly identify a node. It
    // is optimized for quick comparison and low memory consumption.
    // its value depends on the owner document of the node and is
    // categorized in the following way:
    // 1..ID_LAST_TAG: the node inherits HTMLElementImpl and is
    //                 part of the HTML namespace.
    //                 The HTML namespace is either the global
    //                 one (no namespace) or the XHTML namespace
    //                 depending on the owner document's doctype
    // ID_LAST_TAG+1..0xffff: non-HTML elements in the global namespace
    // others       non-HTML elements in a namespace.
    //                 the upper 16 bit identify the namespace
    //                 the lower 16 bit identify the local part of the
    //                 qualified element name.
    virtual Id id() const { return 0; }

    enum MouseEventType {
        MousePress,
        MouseRelease,
        MouseClick,
        MouseDblClick,
        MouseMove,
        MouseWheel
    };

    struct MouseEvent
    {
        MouseEvent( int _button, MouseEventType _type,
                    const DOMString &_url = DOMString(), const DOMString& _target = DOMString(),
                    NodeImpl *_innerNode = 0, NodeImpl *_innerNonSharedNode = 0)
            {
                button = _button; type = _type;
                url = _url; target = _target;
                innerNode = _innerNode;
		innerNonSharedNode = _innerNonSharedNode;
            }

        int button;
        MouseEventType type;
        DOMString url; // url under mouse or empty
        DOMString target;
        Node innerNode;
	Node innerNonSharedNode;
    };

    // for LINK and STYLE
    // will increase/decrease the document's pending sheet count if appropriate
    virtual bool checkAddPendingSheet() { return true; }
    virtual bool checkRemovePendingSheet() { return true; }

    bool hasID() const      { return m_hasId; }
    bool hasClass() const   { return m_hasClass; }
    bool hasCombinedStyle() const   { return m_hasCombinedStyle; }
    bool active() const     { return m_active; }
    bool focused() const { return m_focused; }
    bool hovered() const    { return m_hovered; }
    bool attached() const   { return m_attached; }
    bool closed() const     { return m_closed; }
    bool changed() const    { return m_changed; }
    bool hasChangedChild() const { return m_hasChangedChild; }
    bool hasAnchor() const { return m_hasAnchor; }
    bool inDocument() const { return m_inDocument; }
    bool implicitNode() const { return m_implicit; }
    bool htmlCompat() const { return m_htmlCompat; }
    void setHasID(bool b=true) { m_hasId = b; }
    void setHasClass(bool b=true) { m_hasClass = b; }
    void setHasChangedChild( bool b = true ) { m_hasChangedChild = b; }
    void setInDocument(bool b=true) { m_inDocument = b; }
    void setHTMLCompat(bool b) { m_htmlCompat = b; }
    bool hasHoverDependency() { return m_hasHoverDependency; }
    void setHasHoverDependency(bool b = true) { m_hasHoverDependency = b; }
    void setNeedsStyleAttributeUpdate(bool b = true) { m_needsStyleAttributeUpdate = b; }
    virtual void setFocus(bool b=true) { m_focused = b; }
    virtual void setActive(bool b=true) { m_active = b; }
    virtual void setHovered(bool b=true) { m_hovered = b; }
    virtual void setChanged(bool b=true);
    // for WebCore API compatibility
    void setAttached(bool b=true) { m_attached = b; }

    // for descending restyle when ID or CLASS changes
    bool changedAscendentAttribute() const { return m_changedAscendentAttribute; }
    void setChangedAscendentAttribute(bool b) { m_changedAscendentAttribute = b; }
 
    virtual short tabIndex() const { return 0; }

    enum FocusType {
        FT_Any,
        FT_Mouse,
        FT_Tab
    };

    // Elements that are focusable by default should override this.
    // Warning: if they're in a language that supports tabIndex (e.g. HTML),
    // they must call back to the base class whenever hasTabIndex() is set.
    virtual bool isFocusableImpl(FocusType) const { return false; }
    bool isFocusable() const { return isFocusableImpl(FT_Any); }
    bool isMouseFocusable() const { return isFocusableImpl(FT_Mouse); }
    bool isTabFocusable() const { return isFocusableImpl(FT_Tab); }

    virtual bool isInline() const;

    virtual bool isContentEditable() const;
    virtual void getCaret(int offset, bool override, int &_x, int &_y, int &width, int &height);
    virtual QRect getRect() const;

    enum StyleChange { NoChange, NoInherit, Inherit, Detach, Force };
    virtual void recalcStyle( StyleChange = NoChange ) {}
    static StyleChange diff( khtml::RenderStyle *s1, khtml::RenderStyle *s2 );
    static bool pseudoDiff( khtml::RenderStyle *s1, khtml::RenderStyle *s2, unsigned int pid);

    virtual bool affectedByNoInherit() const;

    unsigned long nodeIndex() const;
    // Returns the document that this node is associated with. This is guaranteed to always be non-null, as opposed to
    // DOM's ownerDocument() which is null for Document nodes (and sometimes DocumentType nodes).
    DocumentImpl* document() const { return m_document.get(); }
    void setDocument(DocumentImpl* doc);

    virtual DocumentImpl* eventTargetDocument();

    void dispatchEvent(EventImpl *evt, int &exceptioncode, bool tempEvent = false);

    // takes care of bubbling and the like. The target is generally 'this',
    // unless the event specifies something special like Window as the target,
    // in which case that's used for dispatch.
    void dispatchGenericEvent( EventImpl *evt, int &exceptioncode);
    
    // return true if event not prevented
    bool dispatchHTMLEvent(int _id, bool canBubbleArg, bool cancelableArg);

    // Window events are special in that they're only dispatched on Window, and not
    // the current node.
    void dispatchWindowEvent(int _id, bool canBubbleArg, bool cancelableArg);
    void dispatchWindowEvent(EventImpl* evt);
    
    void dispatchMouseEvent(QMouseEvent *e, int overrideId = 0, int overrideDetail = 0);
    void dispatchUIEvent(int _id, int detail = 0);
    void dispatchSubtreeModifiedEvent();
    // return true if defaultPrevented (i.e. event should be swallowed)
    // this matches the logic in KHTMLView.
    bool dispatchKeyEvent(QKeyEvent *key, bool keypress);

    virtual bool isReadOnly();
    virtual bool childTypeAllowed( unsigned short /*type*/ ) { return false; }
    virtual unsigned long childNodeCount();
    virtual NodeImpl *childNode(unsigned long index);

    /**
     * Does a pre-order traversal of the tree to find the node next node after this one. This uses the same order that
     * the tags appear in the source file.
     *
     * @param stayWithin If not null, the traversal will stop once the specified node is reached. This can be used to
     * restrict traversal to a particular sub-tree.
     *
     * @return The next node, in document order
     *
     * see traversePreviousNode()
     */
    NodeImpl *traverseNextNode(NodeImpl *stayWithin = 0) const;

    /**
     * Does a reverse pre-order traversal to find the node that comes before the current one in document order
     *
     * see traverseNextNode()
     */
    NodeImpl *traversePreviousNode() const;

    DocumentImpl *docPtr() const { return m_document.get(); }

    NodeImpl *previousEditable() const;
    NodeImpl *nextEditable() const;
    //bool isEditable() const;

    khtml::RenderObject *renderer() const { return m_render; }
    khtml::RenderObject *nextRenderer();
    khtml::RenderObject *previousRenderer();
    void setRenderer(khtml::RenderObject* renderer) { m_render = renderer; }

    void checkSetPrefix(const DOMString &_prefix, int &exceptioncode);
    void checkAddChild(NodeImpl *newChild, int &exceptioncode);
    bool isAncestor( NodeImpl *other ) const;
    virtual bool childAllowed( NodeImpl *newChild );

    // Used to determine whether range offsets use characters or node indices.
    virtual bool offsetInCharacters() const { return false; }
    // Number of DOM 16-bit units contained in node. Note that rendered text length can be different - e.g. because of
    // css-transform:capitalize breaking up precomposed characters and ligatures.
    virtual int maxCharacterOffset() const { return 0; }

    virtual long maxOffset() const;
    virtual long caretMinOffset() const;
    virtual long caretMaxOffset() const;
    virtual unsigned long caretMaxRenderedOffset() const;

    // -----------------------------------------------------------------------------
    // Integration with rendering tree

    /**
     * Attaches this node to the rendering tree. This calculates the style to be applied to the node and creates an
     * appropriate RenderObject which will be inserted into the tree (except when the style has display: none). This
     * makes the node visible in the KHTMLView.
     */
    virtual void attach();

    /**
     * Detaches the node from the rendering tree, making it invisible in the rendered view. This method will remove
     * the node's rendering object from the rendering tree and delete it.
     */
    virtual void detach();

    /**
     * Notifies the node that no more children will be added during parsing.
     * After a node has been closed all changes must go through the DOM interface.
     */
    virtual void close();

    virtual void structureChanged() {}
    virtual void backwardsStructureChanged() {}

    void createRendererIfNeeded();
    virtual khtml::RenderStyle *styleForRenderer(khtml::RenderObject *parent);
    virtual bool rendererIsNeeded(khtml::RenderStyle *);
    virtual khtml::RenderObject *createRenderer(khtml::RenderArena *, khtml::RenderStyle *);

    virtual khtml::RenderStyle* computedStyle();

    // -----------------------------------------------------------------------------
    // Methods for maintaining the state of the element between history navigation

    /**
     * Indicates whether or not this type of node maintains its state. If so, the state of the node will be stored when
     * the user goes to a different page using the state() method, and restored using the restoreState() method if the
     * user returns (e.g. using the back button). This is used to ensure that user-changeable elements such as form
     * controls maintain their contents when the user returns to a previous page in the history.
     */
    virtual bool maintainsState();

    /**
     * Returns the state of this node represented as a string. This string will be passed to restoreState() if the user
     * returns to the page.
     *
     * @return State information about the node represented as a string
     */
    virtual QString state();

    /**
     * Sets the state of the element based on a string previosuly returned by state(). This is used to initialize form
     * controls with their old values when the user returns to the page in their history.
     *
     * @param state A string representation of the node's previously-stored state
     */
    virtual void restoreState(const QString &state);

    // -----------------------------------------------------------------------------
    // Notification of document structure changes

    /**
     * Notifies the node that it has been inserted into the document. This is called during document parsing, and also
     * when a node is added through the DOM methods insertBefore(), appendChild() or replaceChild(). Note that this only
     * happens when the node becomes part of the document tree, i.e. only when the document is actually an ancestor of
     * the node. The call happens _after_ the node has been added to the tree.
     *
     * This is similar to the DOMNodeInsertedIntoDocument DOM event, but does not require the overhead of event
     * dispatching.
     */
    virtual void insertedIntoDocument();

    /**
     * Notifies the node that it is no longer part of the document tree, i.e. when the document is no longer an ancestor
     * node.
     *
     * This is similar to the DOMNodeRemovedFromDocument DOM event, but does not require the overhead of event
     * dispatching, and is called _after_ the node is removed from the tree.
     */
    virtual void removedFromDocument();

    /**
     * Notifies the node that its list of children have changed (either by adding or removing child nodes), or a child
     * node that is of the type CDATA_SECTION_NODE, TEXT_NODE or COMMENT_NODE has changed its value.
     */
    virtual void childrenChanged();

    virtual DOMString toString() const = 0;
    /**
     * Sometimes we need to get the string between two points on the DOM graph.  Use this function to do this.
     * For example, when the user copies some selected text to the clipboard as html.
     * @param selectionStart Where to start the selection.  If selectionStart != this, it is assumed we are after the start point
     * @param selectionEnd   Where to end the selection.  If selectionEnd != this, it is assumed we are before the end point (unless found is true)
     * @param startOffset    Number of characters into the text in selectionStart that the start of the selection is.
     * @param endOffset      Number of characters into the text in selectionEnd that the end of the selection is.
     * @param found          When this is set to true, don't print anymore but closing tags.
     * @return An html formatted string for this node and its children between the selectionStart and selectionEnd.
     */
    virtual DOMString selectionToString(NodeImpl * selectionStart,
                                        NodeImpl * selectionEnd,
                                        int startOffset,
                                        int endOffset,
                                        bool &found) const
    { Q_UNUSED(selectionStart);
      Q_UNUSED(selectionEnd);
      Q_UNUSED(startOffset);
      Q_UNUSED(endOffset);
      Q_UNUSED(found);
      return toString();
    }

    // FOR SVG Events support (WebCore API compatibility)
    QList<RegisteredEventListener>* localEventListeners() {
        return listenerList().listeners;
    }

    DOMString lookupNamespaceURI( const DOMString& prefix );

private: // members
    khtml::DocPtr<DocumentImpl> m_document;
    NodeImpl *m_previous;
    NodeImpl *m_next;

    NodeImpl* findNextElementAncestor( NodeImpl* node );
protected:
    khtml::RenderObject *m_render;

    bool m_hasId : 1;
    bool m_attached : 1;
    bool m_closed : 1;
    bool m_changed : 1;
    bool m_hasChangedChild : 1;
    bool m_changedAscendentAttribute : 1;
    bool m_inDocument : 1;
    bool m_hasAnchor : 1;

    bool m_hovered : 1;
    bool m_focused : 1;
    bool m_active : 1;
    bool m_implicit : 1; // implicitely generated by the parser
    bool m_htmlCompat : 1; // true if element was created in HTML compat mode
    bool m_hasClass : 1;   // true if element has a class property, as relevant to CSS
    bool m_hasCombinedStyle : 1; // true if element has inline styles and presentational styles
    bool m_hasHoverDependency : 1; // true if element has hover dependency on itself

    bool m_elementHasRareData : 1;
    mutable bool m_needsStyleAttributeUpdate : 1; // true if |style| attribute is out of sync (i.e. CSSOM modified our inline styles)

    // 14 bits left
};

// this is the full Node Implementation with parents and children.
class NodeBaseImpl : public NodeImpl
{
public:
    NodeBaseImpl(DocumentImpl *doc)
        : NodeImpl(doc), _first(0), _last(0) {}
    virtual ~NodeBaseImpl();

    // DOM methods overridden from  parent classes
    virtual NodeImpl *firstChild() const;
    virtual NodeImpl *lastChild() const;
    virtual NodeImpl *insertBefore ( NodeImpl *newChild, NodeImpl *refChild, int &exceptioncode );
    virtual void replaceChild ( NodeImpl *newChild, NodeImpl *oldChild, int &exceptioncode );
    virtual void removeChild ( NodeImpl *oldChild, int &exceptioncode );
    virtual NodeImpl *appendChild ( NodeImpl *newChild, int &exceptioncode );
    virtual bool hasChildNodes (  ) const;

    // Other methods (not part of DOM)
    virtual void removeChildren();
    void cloneChildNodes(NodeImpl *clone);

    virtual void setFirstChild(NodeImpl *child);
    virtual void setLastChild(NodeImpl *child);
    virtual NodeImpl *addChild(NodeImpl *newChild);
    virtual void attach();
    virtual void detach();


    bool getUpperLeftCorner(int &xPos, int &yPos) const;
    bool getLowerRightCorner(int &xPos, int &yPos) const;

    virtual void setFocus(bool=true);
    virtual void setActive(bool=true);
    virtual void setHovered(bool=true);
    virtual unsigned long childNodeCount();
    virtual NodeImpl *childNode(unsigned long index);

protected:
    NodeImpl *_first;
    NodeImpl *_last;

    // helper functions for inserting children:

    // ### this should vanish. do it in dom/ !
    // check for same source document:
    bool checkSameDocument( NodeImpl *newchild, int &exceptioncode );
    // check for being child:
    bool checkIsChild( NodeImpl *oldchild, int &exceptioncode );
    // ###

    // find out if a node is allowed to be our child
    void dispatchChildInsertedEvents( NodeImpl *child, int &exceptioncode );
    void dispatchChildRemovalEvents( NodeImpl *child, int &exceptioncode );
};

// Generic NamedNodeMap interface
// Other classes implement this for more specific situations e.g. attributes
// of an element
class NamedNodeMapImpl : public khtml::Shared<NamedNodeMapImpl>
{
public:
    NamedNodeMapImpl();
    virtual ~NamedNodeMapImpl();

    // DOM methods & attributes for NamedNodeMap
    virtual NodeImpl *getNamedItem(NodeImpl::Id id, const PrefixName& prefix = emptyPrefixName, bool nsAware = false) = 0;
    virtual Node removeNamedItem(NodeImpl::Id id, const PrefixName& prefix, bool nsAware, int &exceptioncode) = 0;
    virtual Node setNamedItem(NodeImpl* arg, const PrefixName& prefix, bool nsAware, int &exceptioncode) = 0;

    //The DOM-style wrappers
    NodeImpl* getNamedItem( const DOMString &name );
    Node setNamedItem( const Node &arg, int& exceptioncode );
    Node removeNamedItem( const DOMString &name, int& exceptioncode );
    Node getNamedItemNS( const DOMString &namespaceURI, const DOMString &localName );
    Node setNamedItemNS( const Node &arg, int& exceptioncode );
    Node removeNamedItemNS( const DOMString &namespaceURI, const DOMString &localName, int& exceptioncode );

    virtual NodeImpl *item(unsigned index) = 0;
    virtual unsigned length() const = 0;

    virtual bool isReadOnly() { return false; }
    virtual bool htmlCompat() { return false; }
};

// Generic read-only NamedNodeMap implementation
// Used for e.g. entities and notations in DocumentType.
// You can add nodes using addNode
class GenericRONamedNodeMapImpl : public NamedNodeMapImpl
{
public:
    GenericRONamedNodeMapImpl(DocumentImpl* doc);
    virtual ~GenericRONamedNodeMapImpl();

    // DOM methods & attributes for NamedNodeMap

    virtual NodeImpl *getNamedItem(NodeImpl::Id id, const PrefixName& prefix = emptyPrefixName, bool nsAware = false);
    virtual Node removeNamedItem(NodeImpl::Id id, const PrefixName& prefix, bool nsAware, int &exceptioncode);
    virtual Node setNamedItem(NodeImpl* arg, const PrefixName& prefix, bool nsAware, int &exceptioncode);

    virtual NodeImpl *item(unsigned index);
    virtual unsigned length() const;

    virtual bool isReadOnly() { return true; }

    void addNode(NodeImpl *n);

protected:
    DocumentImpl* m_doc;
    QList<NodeImpl*> *m_contents;
};

} //namespace
#endif
