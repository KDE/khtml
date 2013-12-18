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
#ifndef HTML_ELEMENT_H
#define HTML_ELEMENT_H

#include <khtml_export.h>
#include <dom/dom_element.h>

class KHTMLView;

namespace DOM {

class HTMLElementImpl;
class DOMString;
class Element;
class HTMLCollection;

/**
 * All HTML element interfaces derive from this class. Elements that
 * only expose the HTML core attributes are represented by the base
 * \c HTMLElement interface. These elements are as
 * follows:
 *
 *  \li \c HEAD
 *
 *  \li special: <tt> SUB, SUP, SPAN, BDO </tt>
 *
 *  \li font: <tt> TT, I, B, U, S, STRIKE, BIG, SMALL </tt>
 *
 *  \li phrase: <tt> EM, STRONG, DFN, CODE, SAMP, KBD, VAR,
 * CITE, ACRONYM, ABBR</tt>
 *
 *  \li list: <tt> DD, DT </tt>
 *
 *  \li <tt> NOFRAMES, NOSCRIPT </tt>
 *
 *  \li <tt> ADDRESS, CENTER </tt>
 *
 *  Note: The \c style attribute for this
 * interface is reserved for future usage.
 *
 */
class KHTML_EXPORT HTMLElement : public Element
{
    friend class HTMLDocument;
    friend class ::KHTMLView;
    friend class HTMLTableElement;
    friend class HTMLTableRowElement;
    friend class HTMLTableSectionElement;

public:
    HTMLElement();
    HTMLElement(const HTMLElement &other);
    HTMLElement(const Node &other) : Element()
         {(*this)=other;}

protected:
    HTMLElement(HTMLElementImpl *impl);
public:

    HTMLElement & operator = (const HTMLElement &other);
    HTMLElement & operator = (const Node &other);

    ~HTMLElement();

    /**
     * The element's identifier. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/global.html#adef-id">
     * id attribute definition </a> in HTML 4.0.
     *
     */
    DOMString id() const;

    /**
     * see id
     */
    void setId( const DOMString & );

    /**
     * The element's advisory title. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/global.html#adef-title">
     * title attribute definition </a> in HTML 4.0.
     *
     */
    DOMString title() const;

    /**
     * see title
     */
    void setTitle( const DOMString & );

    /**
     * Language code defined in RFC 1766. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/dirlang.html#adef-lang">
     * lang attribute definition </a> in HTML 4.0.
     *
     */
    DOMString lang() const;

    /**
     * see lang
     */
    void setLang( const DOMString & );

    /**
     * Specifies the base direction of directionally neutral text and
     * the directionality of tables. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/dirlang.html#adef-dir">
     * dir attribute definition </a> in HTML 4.0.
     *
     */
    DOMString dir() const;

    /**
     * see dir
     */
    void setDir( const DOMString & );

    /**
     * The class attribute of the element. This attribute has been
     * renamed due to conflicts with the "class" keyword exposed by
     * many languages. See the <a
     * href="http://www.w3.org/TR/REC-html40/struct/global.html#adef-class">
     * class attribute definition </a> in HTML 4.0.
     *
     */
    DOMString className() const;

    /**
     * see className
     */
    void setClassName( const DOMString & );

    /**
     * The HTML code contained in this element.
     * This function is not part of the DOM specifications as defined by the w3c.
     */
    DOMString innerHTML() const;

    /**
     * Set the HTML content of this node.
     *
     * @exception DOMException
     * NO_MODIFICATION_ALLOWED_ERR: Raised if there is the element does not allow
     * children.
     */
    void setInnerHTML( const DOMString &html );

    /**
     * The text contained in this element.
     * This function is not part of the DOM specifications as defined by the w3c.
     */
    DOMString innerText() const;

    /**
     * Set the text content of this node.
     *
     * @exception DOMException
     * NO_MODIFICATION_ALLOWED_ERR: Raised if there is the element does not allow
     * children.
     */
    void setInnerText( const DOMString &text );

    /**
     * Retrieves a collection of nodes that are direct descendants of this node.
     * IE-specific extension.
     */
    HTMLCollection children() const;

    /**
     * Retrieves a collection of all nodes that descend from this node.
     * IE-specific extension.
     */
    HTMLCollection all() const;

    /**
     * Returns whether this element is editable.
     *
     * This function is not part of the DOM specifications as defined by the w3c.
     */
    bool isContentEditable() const;

    /**
     * Returns the kind of editability that applies to this element.
     *
     * The returned string is one of:
     * \li true: This element has been set to be editable.
     * \li false: This element has been set not to be editable.
     * \li inherit: This element inherits its editability from the parent.
     *
     * This function is not part of the DOM specifications as defined by the w3c.
     */
    DOMString contentEditable() const;

    /**
     * Sets the editability of this element.
     *
     * This function is not part of the DOM specifications as defined by the w3c.
     * @param enabled may be one of:
     * \li true: make element editable
     * \li false: make element not editable
     * \li inherit: make element inherit editability from parent.
     */
    void setContentEditable(const DOMString &enabled);

    /*
     * @internal
     */
    void removeCSSProperty( const DOMString& property );

    /*
     * @internal
     */
    void addCSSProperty( const DOMString &property, const DOMString &value );

protected:
    /*
     * @internal
     */
    void assignOther( const Node &other, int elementId );
};

} //namespace

#endif
