/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright 1999 Lars Knoll (knoll@kde.org)
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
 * This file includes excerpts from the Document Object Model (DOM)
 * Level 1 Specification (Recommendation)
 * http://www.w3.org/TR/REC-DOM-Level-1/
 * Copyright © World Wide Web Consortium , (Massachusetts Institute of
 * Technology , Institut National de Recherche en Informatique et en
 * Automatique , Keio University ). All Rights Reserved.
 *
 */
#ifndef _DOM_ELEMENT_h_
#define _DOM_ELEMENT_h_

#include <khtml_export.h>
#include <dom/dom_node.h>
#include <dom/css_value.h>

namespace DOM {

class DOMString;
class AttrImpl;
class Element;
class ElementImpl;
class NamedAttrMapImpl;
class DocumentImpl;

/**
 * The \c Attr interface represents an attribute in an
 * \c Element object. Typically the allowable values for
 * the attribute are defined in a document type definition.
 *
 *  \c Attr objects inherit the \c Node
 * interface, but since they are not actually child nodes of the
 * element they describe, the DOM does not consider them part of the
 * document tree. Thus, the \c Node attributes
 * \c parentNode , \c previousSibling , and
 * \c nextSibling have a null value for \c Attr
 * objects. The DOM takes the view that attributes are properties of
 * elements rather than having a separate identity from the elements
 * they are associated with; this should make it more efficient to
 * implement such features as default attributes associated with all
 * elements of a given type. Furthermore, \c Attr nodes
 * may not be immediate children of a \c DocumentFragment
 * . However, they can be associated with \c Element nodes
 * contained within a \c DocumentFragment . In short,
 * users and implementors of the DOM need to be aware that \c Attr
 * nodes have some things in common with other objects
 * inheriting the \c Node interface, but they also are
 * quite distinct.
 *
 *  The attribute's effective value is determined as follows: if this
 * attribute has been explicitly assigned any value, that value is the
 * attribute's effective value; otherwise, if there is a declaration
 * for this attribute, and that declaration includes a default value,
 * then that default value is the attribute's effective value;
 * otherwise, the attribute does not exist on this element in the
 * structure model until it has been explicitly added. Note that the
 * \c nodeValue attribute on the \c Attr
 * instance can also be used to retrieve the string version of the
 * attribute's value(s).
 *
 *  In XML, where the value of an attribute can contain entity
 * references, the child nodes of the \c Attr node provide
 * a representation in which entity references are not expanded. These
 * child nodes may be either \c Text or
 * \c EntityReference nodes. Because the attribute type may be
 * unknown, there are no tokenized attribute values.
 *
 */
class KHTML_EXPORT Attr : public Node
{
    friend class Element;
    friend class Document;
    friend class DocumentImpl;
    friend class HTMLDocument;
    friend class ElementImpl;
    friend class NamedAttrMapImpl;
    friend class AttrImpl;

public:
    Attr();
    Attr(const Node &other) : Node()
        {(*this)=other;}
    Attr(const Attr &other);

    Attr & operator = (const Node &other);
    Attr & operator = (const Attr &other);

    ~Attr();

    /**
     * Returns the name of this attribute.
     *
     */
    DOMString name() const;

    /**
     * If this attribute was explicitly given a value in the original
     * document, this is \c true ; otherwise, it is
     * \c false . Note that the implementation is in charge of
     * this attribute, not the user. If the user changes the value of
     * the attribute (even if it ends up having the same value as the
     * default value) then the \c specified flag is
     * automatically flipped to \c true . To re-specify
     * the attribute as the default value from the DTD, the user must
     * delete the attribute. The implementation will then make a new
     * attribute available with \c specified set to
     * \c false and the default value (if one exists).
     *
     *  In summary: 
     *  \li If the attribute has an assigned
     * value in the document then \c specified is
     * \c true , and the value is the assigned value.
     *
     *  \li If the attribute has no assigned value in the
     * document and has a default value in the DTD, then
     * \c specified is \c false , and the value is
     * the default value in the DTD.
     *
     *  \li If the attribute has no assigned value in the
     * document and has a value of #IMPLIED in the DTD, then the
     * attribute does not appear in the structure model of the
     * document.
     *
     *
     *
     */
    bool specified() const;

    /**
     * On retrieval, the value of the attribute is returned as a
     * string. Character and general entity references are replaced
     * with their values.
     *
     *  On setting, this creates a \c Text node with the
     * unparsed contents of the string.
     *
     */
    DOMString value() const;

    /**
     * see value
     */
    void setValue( const DOMString & );

    /**
     * Introduced in DOM Level 2
     *
     * The Element node this attribute is attached to or null if this attribute
     * is not in use.
     */
    Element ownerElement() const;

protected:

    Attr( AttrImpl *_impl );
};

class NodeList;
class Attr;
class DOMString;

/**
 * By far the vast majority of objects (apart from text) that authors
 * encounter when traversing a document are \c Element
 * nodes. Assume the following XML document: &lt;elementExample
 * id=&quot;demo&quot;&gt; &lt;subelement1/&gt;
 * &lt;subelement2&gt;&lt;subsubelement/&gt;&lt;/subelement2&gt;
 * &lt;/elementExample&gt;
 *
 *  When represented using DOM, the top node is an \c Element
 * node for &quot;elementExample&quot;, which contains two
 * child \c Element nodes, one for &quot;subelement1&quot;
 * and one for &quot;subelement2&quot;. &quot;subelement1&quot;
 * contains no child nodes.
 *
 *  Elements may have attributes associated with them; since the
 * \c Element interface inherits from \c Node
 * , the generic \c Node interface method
 * \c getAttributes may be used to retrieve the set of all
 * attributes for an element. There are methods on the \c Element
 * interface to retrieve either an \c Attr object
 * by name or an attribute value by name. In XML, where an attribute
 * value may contain entity references, an \c Attr object
 * should be retrieved to examine the possibly fairly complex sub-tree
 * representing the attribute value. On the other hand, in HTML, where
 * all attributes have simple string values, methods to directly
 * access an attribute value can safely be used as a convenience.
 *
 */
class KHTML_EXPORT Element : public Node
{
    friend class Document;
    friend class DocumentFragment;
    friend class HTMLDocument;
//    friend class AttrImpl;
    friend class Attr;

public:
    Element();
    Element(const Node &other) : Node()
        {(*this)=other;}
    Element(const Element &other);

    Element & operator = (const Node &other);
    Element & operator = (const Element &other);

    ~Element();

    /**
     * The name of the element. For example, in: &lt;elementExample
     * id=&quot;demo&quot;&gt; ... &lt;/elementExample&gt; ,
     * \c tagName has the value \c &quot;elementExample&quot;
     * . Note that this is case-preserving in XML, as are all
     * of the operations of the DOM. The HTML DOM returns the
     * \c tagName of an HTML element in the canonical uppercase
     * form, regardless of the case in the source HTML document.
     *
     */
    DOMString tagName() const;

    /**
     * Retrieves an attribute value by name.
     *
     * @param name The name of the attribute to retrieve.
     *
     * @return The \c Attr value as a string, or the empty
     * string if that attribute does not have a specified or default
     * value.
     *
     */
    DOMString getAttribute ( const DOMString &name );

    /**
     * Adds a new attribute. If an attribute with that name is already
     * present in the element, its value is changed to be that of the
     * value parameter. This value is a simple string, it is not
     * parsed as it is being set. So any markup (such as syntax to be
     * recognized as an entity reference) is treated as literal text,
     * and needs to be appropriately escaped by the implementation
     * when it is written out. In order to assign an attribute value
     * that contains entity references, the user must create an
     * \c Attr node plus any \c Text and
     * \c EntityReference nodes, build the appropriate subtree,
     * and use \c setAttributeNode to assign it as the
     * value of an attribute.
     *
     * @param name The name of the attribute to create or alter.
     *
     * @param value Value to set in string form.
     *
     * @return
     *
     * @exception DOMException
     * INVALID_CHARACTER_ERR: Raised if the specified name contains an
     * invalid character.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     *
     */
    void setAttribute ( const DOMString &name, const DOMString &value );

    /**
     * Removes an attribute by name. If the removed attribute has a
     * default value it is immediately replaced.
     *
     * @param name The name of the attribute to remove.
     *
     * @return
     *
     * @exception DOMException
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     *
     */
    void removeAttribute ( const DOMString &name );

    /**
     * Retrieves an \c Attr node by name.
     *
     * @param name The name of the attribute to retrieve.
     *
     * @return The \c Attr node with the specified
     * attribute name or \c null if there is no such
     * attribute.
     *
     */
    Attr getAttributeNode ( const DOMString &name );

    /**
     * Adds a new attribute. If an attribute with that name is already
     * present in the element, it is replaced by the new one.
     *
     * @param newAttr The \c Attr node to add to the
     * attribute list.
     *
     * @return If the \c newAttr attribute replaces an
     * existing attribute with the same name, the previously existing
     * \c Attr node is returned, otherwise \c null
     * is returned.
     *
     * @exception DOMException
     * WRONG_DOCUMENT_ERR: Raised if \c newAttr was
     * created from a different document than the one that created the
     * element.
     *
     *  NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     *
     *  INUSE_ATTRIBUTE_ERR: Raised if \c newAttr is
     * already an attribute of another \c Element object.
     * The DOM user must explicitly clone \c Attr nodes to
     * re-use them in other elements.
     *
     */
    Attr setAttributeNode ( const Attr &newAttr );

    /**
     * Removes the specified attribute.
     *
     * @param oldAttr The \c Attr node to remove from the
     * attribute list. If the removed \c Attr has a
     * default value it is immediately replaced.
     *
     * @return The \c Attr node that was removed.
     *
     * @exception DOMException
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     *
     *  NOT_FOUND_ERR: Raised if \c oldAttr is not an
     * attribute of the element.
     *
     */
    Attr removeAttributeNode ( const Attr &oldAttr );

    /**
     * Returns a \c NodeList of all descendant elements
     * with a given tag name, in the order in which they would be
     * encountered in a preorder traversal of the \c Element
     * tree.
     *
     * @param name The name of the tag to match on. The special value
     * "*" matches all tags.
     *
     * @return A list of matching \c Element nodes.
     *
     */
    NodeList getElementsByTagName ( const DOMString &name );

    /**
     * Introduced in DOM Level 2
     * Returns a NodeList of all the descendant Elements with a given local
     * name and namespace URI in the order in which they are encountered in a
     * preorder traversal of this Element tree.
     *
     * @param namespaceURI The namespace URI of the elements to match on. The
     * special value "*" matches all namespaces.
     *
     * @param localName The local name of the elements to match on. The special
     * value "*" matches all local names.
     *
     * @return A new NodeList object containing all the matched Elements.
     */
    NodeList getElementsByTagNameNS ( const DOMString &namespaceURI,
                                      const DOMString &localName );

    /**
     * Introduced in HTML 5.
     * No Exceptions.
     *
     * Returns a \c NodeList of all the \c Element 's
     * with a given class name in the order in which they
     * would be encountered in a preorder traversal of the
     * \c Document tree.
     *
     * @param tagname An unordered set of unique space-separated
     * tokens representing classes.
     *
     * @return A new \c NodeList object containing all the
     * matched \c Element s.
     *
     * @since 4.1
     */
    NodeList getElementsByClassName ( const DOMString &className );

    /**
     * Introduced in DOM Level 2.
     *
     * No Exceptions.
     *
     * Retrieves an attribute value by local name and namespace URI. HTML-only
     * DOM implementations do not need to implement this method.
     *
     * @param namespaceURI The namespace URI of the attribute to retrieve.
     *
     * @param localName The local name of the attribute to retrieve.
     *
     * @return The Attr value as a string, or the empty string if that
     * attribute does not have a specified or default value.
     */
    DOMString getAttributeNS ( const DOMString &namespaceURI,
                               const DOMString &localName );

    /**
     * Introduced in DOM Level 2
     *
     * Adds a new attribute. If an attribute with the same local name and
     * namespace URI is already present on the element, its prefix is changed
     * to be the prefix part of the qualifiedName, and its value is changed to
     * be the value parameter. This value is a simple string; it is not parsed
     * as it is being set. So any markup (such as syntax to be recognized as an
     * entity reference) is treated as literal text, and needs to be
     * appropriately escaped by the implementation when it is written out. In
     * order to assign an attribute value that contains entity references, the
     * user must create an Attr node plus any Text and EntityReference nodes,
     * build the appropriate subtree, and use setAttributeNodeNS or
     * setAttributeNode to assign it as the value of an attribute.
     *
     * HTML-only DOM implementations do not need to implement this method.
     *
     * @param namespaceURI The namespace URI of the attribute to create or
     * alter.
     *
     * @param qualifiedName The qualified name of the attribute to create or
     * alter.
     *
     * @param value The value to set in string form.
     *
     * @exception DOMException
     * INVALID_CHARACTER_ERR: Raised if the specified qualified name contains
     * an illegal character.
     *
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     *
     * NAMESPACE_ERR: Raised if the qualifiedName is malformed, if the
     * qualifiedName has a prefix and the namespaceURI is null, if the
     * qualifiedName has a prefix that is "xml" and the namespaceURI is
     * different from "http://www.w3.org/XML/1998/namespace", or if the
     * qualifiedName is "xmlns" and the namespaceURI is different from
     * "http://www.w3.org/2000/xmlns/".
     */
    void setAttributeNS ( const DOMString &namespaceURI,
                          const DOMString &qualifiedName,
                          const DOMString &value );

    /**
     * Introduced in DOM Level 2
     *
     * Removes an attribute by local name and namespace URI. If the removed
     * attribute has a default value it is immediately replaced. The replacing
     * attribute has the same namespace URI and local name, as well as the
     * original prefix.
     *
     * HTML-only DOM implementations do not need to implement this method.
     *
     * @param namespaceURI The namespace URI of the attribute to remove.
     *
     * @param localName The local name of the attribute to remove.
     *
     * @exception DOMException
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     */
    void removeAttributeNS ( const DOMString &namespaceURI,
                             const DOMString &localName );

    /**
     * Introduced in DOM Level 2
     *
     * Retrieves an Attr node by local name and namespace URI. HTML-only DOM
     * implementations do not need to implement this method.
     *
     * @param namespaceURI The namespace URI of the attribute to retrieve.
     *
     * @param localName The local name of the attribute to retrieve.
     *
     * @return The Attr node with the specified attribute local name and
     * namespace URI or null if there is no such attribute.
     */
    Attr getAttributeNodeNS ( const DOMString &namespaceURI,
                              const DOMString &localName );

    /**
     * Introduced in DOM Level 2
     *
     * Adds a new attribute. If an attribute with that local name and that
     * namespace URI is already present in the element, it is replaced by the
     * new one.
     *
     * HTML-only DOM implementations do not need to implement this method.
     *
     * @param newAttr The Attr node to add to the attribute list.
     *
     * @return If the newAttr attribute replaces an existing attribute with the
     * same local name and namespace URI, the replaced Attr node is returned,
     * otherwise null is returned.
     *
     * @exception DOMException
     * WRONG_DOCUMENT_ERR: Raised if newAttr was created from a different
     * document than the one that created the element.
     *
     * NO_MODIFICATION_ALLOWED_ERR: Raised if this node is readonly.
     *
     * INUSE_ATTRIBUTE_ERR: Raised if newAttr is already an attribute of
     * another Element object. The DOM user must explicitly clone Attr nodes to
     * re-use them in other elements.
     */
    Attr setAttributeNodeNS ( const Attr &newAttr );

    /**
     * Returns true when an attribute with a given name is specified on this
     * element or has a default value, false otherwise.
     * Introduced in DOM Level 2.
     *
     * @param name The name of the attribute to look for.
     *
     * @return true if an attribute with the given name is specified on this
     * element or has a default value, false otherwise.
     */
    bool hasAttribute( const DOMString& name );

    /**
     * Introduced in DOM Level 2
     *
     * Returns true when an attribute with a given local name and namespace URI
     * is specified on this element or has a default value, false otherwise.
     * HTML-only DOM implementations do not need to implement this method.
     *
     * @param namespaceURI The namespace URI of the attribute to look for.
     *
     * @param localName The local name of the attribute to look for.
     *
     * @return true if an attribute with the given local name and namespace URI
     * is specified or has a default value on this element, false otherwise.
     */
    bool hasAttributeNS ( const DOMString &namespaceURI,
                          const DOMString &localName );

    /**
     * Introduced in DOM Level 2
     * This method is from the CSSStyleDeclaration interface
     *
     * The style attribute
     */
    CSSStyleDeclaration style (  );

    /**
     * Introduced in  DOM level 3
     * This method is part of the ElementTraversal interface
     *
     *  The first child node which is of nodeType ELEMENT_NODE.
     *
     */
    Element firstElementChild ( ) const;

    /**
     * Introduced in  DOM level 3
     * This method is part of the ElementTraversal interface
     *
     *  @return The last child node of that element which is of nodeType ELEMENT_NODE.
     *
     */
    Element lastElementChild ( ) const;

    /**
     * Introduced in  DOM level 3
     * This method is part of the ElementTraversal interface
     *
     *  @return  The sibling node of that element which most immediately precedes that element in document order,
     *           and which is of nodeType ELEMENT_NODE
     *
     */
    Element previousElementSibling ( ) const;

    /**
     * Introduced in  DOM level 3
     * This method is part of the ElementTraversal interface
     *
     *  @return  The sibling node of that element which most immediately follows that element in document order,
     *           and which is of nodeType ELEMENT_NODE
     *
     */
    Element nextElementSibling ( ) const;

    /**
     * Introduced in  DOM level 3
     * This method is part of the ElementTraversal interface
     *
     *  @return The current number of child nodes of that element which are of nodeType ELEMENT_NODE
     *
     */
    unsigned long childElementCount ( ) const;

    /**
     * Introduced in Selectors Level 1.
     *
     * Returns the first (in document order) element in this element's subtree
     * matching the given CSS selector @p query.
     *
     * @since 4.5
     */
    Element querySelector(const DOMString& query) const;

    /**
     * Introduced in Selectors Level 1.
     *
     * Returns all (in document order) elements in this element's subtree
     * matching the given CSS selector @p query. Note that the returned NodeList
     * is static and not live, and will not be updated when the document
     * changes
     *
     * @since 4.5
     */
    NodeList querySelectorAll(const DOMString& query) const;
    
    /**
     * not part of the official DOM
     *
     * This method will always reflect the editability setting of this
     * element as specified by a direct or indirect (that means, inherited)
     * assignment to contentEditable or the respective CSS rule, even if
     * design mode is active.
     *
     * @return whether this element is editable.
     * @see setContentEditable
     */
    bool contentEditable() const;

    /**
     * not part of the official DOM
     *
     * This element can be made editable by setting its contentEditable
     * property to @p true. The setting will be inherited to its children
     * as well.
     *
     * Setting or clearing contentEditable when design mode is active will
     * take no effect. However, its status will still be propagated to all
     * child elements.
     *
     * @param enabled @p true to make this element editable, @p false
     * otherwise.
     * @see DOM::Document::designMode
     */
    void setContentEditable(bool enabled);

    /**
     * @internal
     * not part of the DOM
     */
    bool isHTMLElement() const;

    /**
     * KHTML extension to DOM
     * This method returns the associated form element.
     * returns null if this element is not a form-like element
     * or if this elment is not in the scope of a form element.
     */
    Element form() const;

    static bool khtmlValidAttrName(const DOMString &name);
    static bool khtmlValidPrefix(const DOMString &name);
    static bool khtmlValidQualifiedName(const DOMString &name);

    static bool khtmlMalformedQualifiedName(const DOMString &name);
    static bool khtmlMalformedPrefix(const DOMString &name);
protected:
    Element(ElementImpl *_impl);
};

} //namespace
#endif
