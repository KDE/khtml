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

#ifndef HTML_DOCUMENT_H
#define HTML_DOCUMENT_H

#include <khtml_export.h>

#include <dom/dom_doc.h>
#include <dom/dom_string.h>

class KHTMLView;
class KHTMLPart;

namespace DOM {

class HTMLDocumentImpl;
class DOMImplementation;
class HTMLCollection;
class NodeList;
class Element;
class HTMLElement;

/**
 * An \c HTMLDocument is the root of the HTML hierarchy
 * and holds the entire content. Beside providing access to the
 * hierarchy, it also provides some convenience methods for accessing
 * certain sets of information from the document.
 *
 *  The following properties have been deprecated in favor of the
 * corresponding ones for the BODY element:
 *
 *  \li \c alinkColor
 *
 *  \li \c background
 *
 *  \li \c bgColor
 *
 *  \li \c fgColor
 *
 *  \li \c linkColor
 *
 *  \li \c vlinkColor
 *
 *
 */
class KHTML_EXPORT HTMLDocument : public Document
{
    friend class ::KHTMLView;
    friend class ::KHTMLPart;
    friend class DOMImplementation;
public:
    HTMLDocument();
    /**
     * The parent is the widget the document should render itself in.
     * Rendering information (like sizes, etc...) is only created if
     * parent != 0
     */
    HTMLDocument(KHTMLView *parent);
    HTMLDocument(const HTMLDocument &other);
    HTMLDocument(const Node &other) : Document(false)
         {(*this)=other;}
protected:
    HTMLDocument(HTMLDocumentImpl *impl);
public:

    HTMLDocument & operator = (const HTMLDocument &other);
    HTMLDocument & operator = (const Node &other);

    ~HTMLDocument();

    /**
     * The title of a document as specified by the \c TITLE
     * element in the head of the document.
     *
     */
    DOMString title() const;

    /**
     * see title
     */
    void setTitle( const DOMString & );

    /**
     * Returns the URI of the page that linked to this page. The value
     * is an empty string if the user navigated to the page directly
     * (not through a link, but, for example, via a bookmark).
     */
    DOMString referrer() const;

    /**
     * The domain name of the server that served the document, or a
     * null string if the server cannot be identified by a domain
     * name.
     *
     */
    DOMString domain() const;

    /**
     * The absolute URI of the document.
     */
    DOMString URL() const;

    /**
     * The element that contains the content for the document. In
     * documents with \c BODY contents, returns the
     * \c BODY element, and in frameset documents, this returns
     * the outermost \c FRAMESET element.
     *
     */
    HTMLElement body() const;

    /**
     * see body
     */
    void setBody(const HTMLElement &);

    /**
     * A collection of all the \c IMG elements in a
     * document. The behavior is limited to \c IMG
     * elements for backwards compatibility.
     *
     */
    HTMLCollection images() const;

    /**
     * A collection of all the \c OBJECT elements that
     * include applets and \c APPLET ( deprecated )
     * elements in a document.
     *
     */
    HTMLCollection applets() const;

    /**
     * A collection of all \c AREA elements and anchor (
     * \c A ) elements in a document with a value for the
     * \c href attribute.
     *
     */
    HTMLCollection links() const;

    /**
     * A collection of all the forms of a document.
     *
     */
    HTMLCollection forms() const;

    /**
     * A collection of all the layers of a document.
     *
     */
    HTMLCollection layers() const;

    /**
     * A collection of all the scripts in the document.
     *
     */
    HTMLCollection scripts() const;

    /**
     * A collection of all the anchor ( \c A ) elements in
     * a document with a value for the \c name attribute.
     * Note. For reasons of backwards compatibility, the returned set
     * of anchors only contains those anchors created with the
     * \c name attribute, not those created with the \c id
     * attribute.
     *
     */
    HTMLCollection anchors() const;

    /**
     * The cookies associated with this document. If there are none,
     * the value is an empty string. Otherwise, the value is a string:
     * a semicolon-delimited list of "name, value" pairs for all the
     * cookies associated with the page. For example,
     * \c name=value;expires=date .
     *
     */
    DOMString cookie() const;

    /**
     * see cookie
     */
    void setCookie( const DOMString & );

    /**
     * Note. This method and the ones following allow a user to add to
     * or replace the structure model of a document using strings of
     * unparsed HTML. At the time of writing alternate methods for
     * providing similar functionality for both HTML and XML documents
     * were being considered. The following methods may be deprecated
     * at some point in the future in favor of a more general-purpose
     * mechanism.
     *
     *  Open a document stream for writing. If a document exists in
     * the target, this method clears it.
     *
     * @return
     *
     */
    void open (  );

    /**
     * Closes a document stream opened by \c open() and
     * forces rendering.
     *
     * @return
     *
     */
    void close (  );

    /**
     * Write a string of text to a document stream opened by
     * \c open() . The text is parsed into the document's
     * structure model.
     *
     * @param text The string to be parsed into some structure in the
     * document structure model.
     *
     * @return
     *
     */
    void write ( const DOMString &text );

    /**
     * Write a string of text followed by a newline character to a
     * document stream opened by \c open() . The text is
     * parsed into the document's structure model.
     *
     * @param text The string to be parsed into some structure in the
     * document structure model.
     *
     * @return
     *
     */
    void writeln ( const DOMString &text );

    /**
     * Returns the (possibly empty) collection of elements whose
     * \c name value is given by \c elementName .
     *
     * @param elementName The \c name attribute value for
     * an element.
     *
     * @return The matching elements.
     *
     */
    NodeList getElementsByName ( const DOMString &elementName );

    /**
     * not part of the DOM
     *
     * converts the given (potentially relative) URL in a
     * full-qualified one, using the baseURL / document URL for
     * the missing parts.
     */
    DOMString completeURL( const DOMString& url) const;

    /**
     * Not part of the DOM
     *
     * The date the document was last modified.
     */
    DOMString lastModified() const;

    /**
     * Not part of the DOM
     *
     * A collection of all the \c IMG, \c OBJECT,
     * \c AREA, \c A, forms and anchor elements of
     * a document.
     */
    HTMLCollection all() const;
};

} //namespace

#endif
