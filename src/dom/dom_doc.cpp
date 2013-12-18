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
 */

#include "dom/dom_exception.h"
#include "dom/dom_xml.h"
#include "dom/dom2_range.h"
#include "dom/dom2_events.h"
#include "dom/dom2_views.h"
#include "dom/dom2_traversal.h"
#include "dom/html_document.h"
#include "html/html_documentimpl.h"

#include "xml/dom_docimpl.h"
#include "xml/dom_elementimpl.h"

#include <QDebug>

namespace DOM {

DOMImplementation::DOMImplementation()
{
    impl = 0;
}

DOMImplementation::DOMImplementation(const DOMImplementation &other)
{
    impl = other.impl;
    if (impl) impl->ref();
}

DOMImplementation::DOMImplementation(DOMImplementationImpl *i)
{
    impl = i;
    if (impl) impl->ref();
}

DOMImplementation &DOMImplementation::operator = (const DOMImplementation &other)
{
    if ( impl != other.impl ) {
        if (impl) impl->deref();
        impl = other.impl;
        if (impl) impl->ref();
    }
    return *this;
}

DOMImplementation::~DOMImplementation()
{
    if (impl) impl->deref();
}

bool DOMImplementation::hasFeature( const DOMString &feature, const DOMString &version )
{
    if (!impl)
	return false; // ### enable throw DOMException(DOMException::NOT_FOUND_ERR);

    return impl->hasFeature(feature,version);
}

DocumentType DOMImplementation::createDocumentType ( const DOMString &qualifiedName,
                                                     const DOMString &publicId,
                                                     const DOMString &systemId )
{
    if (!impl)
	throw DOMException(DOMException::NOT_FOUND_ERR);

    int exceptioncode = 0;
    DocumentTypeImpl *r = impl->createDocumentType(qualifiedName, publicId, systemId, exceptioncode);
    if ( exceptioncode )
        throw DOMException( exceptioncode );
    return r;
}

Document DOMImplementation::createDocument ( const DOMString &namespaceURI,
                                             const DOMString &qualifiedName,
                                             const DocumentType &doctype )
{
    if (!impl)
	throw DOMException(DOMException::NOT_FOUND_ERR);

    int exceptioncode = 0;
    DocumentImpl *r = impl->createDocument(namespaceURI, qualifiedName,
                                           (DocumentTypeImpl*)doctype.handle(),
                                           0, exceptioncode );
    if ( exceptioncode )
        throw DOMException( exceptioncode );
    return r;
}

HTMLDocument DOMImplementation::createHTMLDocument( const DOMString& title )
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    return static_cast<DOMImplementationImpl*>(impl)->createHTMLDocument(title);
}

DOMImplementation DOMImplementation::getInterface(const DOMString &/*feature*/) const
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);

    // This method is a no-op for us.
    return impl;
}

CSSStyleSheet DOMImplementation::createCSSStyleSheet(const DOMString &title, const DOMString &media)
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);

    int exceptioncode = 0;
    CSSStyleSheetImpl *r = impl->createCSSStyleSheet(title.implementation(), media.implementation(),
                                                     exceptioncode);
    if ( exceptioncode )
        throw DOMException( exceptioncode );
    return r;
}

DOMImplementationImpl *DOMImplementation::handle() const
{
    return impl;
}

bool DOMImplementation::isNull() const
{
    return (impl == 0);
}

// ----------------------------------------------------------------------------

Document::Document()
    : Node()
{
    // we always want an implementation
    impl = DOMImplementationImpl::createDocument();
    impl->ref();
}

Document::Document(bool create)
    : Node()
{
    if(create)
    {
	impl = DOMImplementationImpl::createDocument();
	impl->ref();
    }
    else
	impl = 0;
//    qDebug() << "Document::Document(bool)";
}

Document::Document(const Document &other) : Node(other)
{
//    qDebug() << "Document::Document(Document &)";
}

Document::Document(DocumentImpl *i) : Node(i)
{
//    qDebug() << "Document::Document(DocumentImpl)";
}

Document &Document::operator = (const Node &other)
{
    NodeImpl* ohandle = other.handle();
    if ( impl != ohandle ) {
        if (!ohandle || ohandle->nodeType() != DOCUMENT_NODE) {
	    if ( impl ) impl->deref();
            impl = 0;
	} else {
            Node::operator =(other);
	}
    }
    return *this;
}

Document &Document::operator = (const Document &other)
{
    Node::operator =(other);
    return *this;
}

Document::~Document()
{
//    qDebug() << "Document::~Document\n";
}

DocumentType Document::doctype() const
{
    if (impl) return ((DocumentImpl *)impl)->doctype();
    return 0;
}

DOMImplementation Document::implementation() const
{
    if (impl) return ((DocumentImpl *)impl)->implementation();
    return 0;
}

Element Document::documentElement() const
{
    if (impl) return ((DocumentImpl *)impl)->documentElement();
    return 0;
}

Element Document::createElement( const DOMString &tagName )
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);

    int exceptioncode = 0;
    ElementImpl* r = ((DocumentImpl *)impl)->createElement(tagName, &exceptioncode);
    if ( exceptioncode )
        throw DOMException( exceptioncode );
    return r;
}

Element Document::createElementNS( const DOMString &namespaceURI, const DOMString &qualifiedName )
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);

    int exceptioncode = 0;
    ElementImpl* r = ((DocumentImpl *)impl)->createElementNS(namespaceURI,qualifiedName, &exceptioncode);
    if ( exceptioncode )
        throw DOMException( exceptioncode );
    return r;
}

DocumentFragment Document::createDocumentFragment(  )
{
    if (impl) return ((DocumentImpl *)impl)->createDocumentFragment();
    return 0;
}

Text Document::createTextNode( const DOMString &data )
{
    if (impl) return ((DocumentImpl *)impl)->createTextNode( data.implementation() );
    return 0;
}

Comment Document::createComment( const DOMString &data )
{
    if (impl) return ((DocumentImpl *)impl)->createComment( data.implementation() );
    return 0;
}

CDATASection Document::createCDATASection( const DOMString &data )
{
    // ### DOM1 spec says raise exception if html documents - what about XHTML documents?
    if (impl) return ((DocumentImpl *)impl)->createCDATASection( data.implementation() );
    return 0;
}

ProcessingInstruction Document::createProcessingInstruction( const DOMString &target, const DOMString &data )
{
    if (impl) return ((DocumentImpl *)impl)->createProcessingInstruction( target, data.implementation() );
    return 0;
}

Attr Document::createAttribute( const DOMString &name )
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    if (name.isNull()) throw DOMException(DOMException::NOT_FOUND_ERR);
    int exceptioncode = 0;
    AttrImpl* a = impl->document()->createAttribute(name, &exceptioncode);
    if ( exceptioncode )
        throw DOMException( exceptioncode );
    return a;
}

Attr Document::createAttributeNS( const DOMString &namespaceURI, const DOMString &qualifiedName )
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    if (qualifiedName.isNull()) throw DOMException(DOMException::NAMESPACE_ERR);
    int exceptioncode = 0;
    AttrImpl* a = impl->document()->createAttributeNS(namespaceURI, qualifiedName, &exceptioncode);
    if ( exceptioncode )
        throw DOMException( exceptioncode );
    return a;
}

EntityReference Document::createEntityReference( const DOMString &name )
{
    if (impl) return ((DocumentImpl *)impl)->createEntityReference( name );
    return 0;
}

Element Document::getElementById( const DOMString &elementId ) const
{
    if(impl) return ((DocumentImpl *)impl)->getElementById( elementId );
    return 0;
}

NodeList Document::getElementsByTagName( const DOMString &tagName )
{
    if (!impl) return 0;
    return impl->getElementsByTagName( tagName );
}

NodeList Document::getElementsByTagNameNS( const DOMString &namespaceURI, const DOMString &localName )
{
    if (!impl) return 0;
    return impl->getElementsByTagNameNS( namespaceURI, localName );
}

NodeList Document::getElementsByClassName( const DOMString& className )
{
    if (!impl) return 0;
    return impl->getElementsByClassName( className );
}

Node Document::importNode( const Node & importedNode, bool deep )
{
    if (!impl)
	throw DOMException(DOMException::INVALID_STATE_ERR);

    int exceptioncode = 0;
    NodeImpl *r = static_cast<DocumentImpl*>(impl)->importNode(importedNode.handle(), deep, exceptioncode);
    if (exceptioncode)
	throw DOMException(exceptioncode);
    return r;
}

bool Document::isHTMLDocument() const
{
    if (impl) return ((DocumentImpl *)impl)->isHTMLDocument();
    return 0;
}

Range Document::createRange()
{
    if (impl) return ((DocumentImpl *)impl)->createRange();
    return 0;
}

NodeIterator Document::createNodeIterator(Node root, unsigned long whatToShow,
                                    NodeFilter filter, bool entityReferenceExpansion)
{
    if (!impl)
	throw DOMException(DOMException::INVALID_STATE_ERR);

    int exceptioncode = 0;
    NodeIteratorImpl *r = static_cast<DocumentImpl*>(impl)->createNodeIterator(root.handle(),
			  whatToShow,filter.handle(),entityReferenceExpansion,exceptioncode);
    if (exceptioncode)
	throw DOMException(exceptioncode);
    return r;
}

TreeWalker Document::createTreeWalker(Node root, unsigned long whatToShow, NodeFilter filter,
                                bool entityReferenceExpansion)
{
    if (!impl)
	throw DOMException(DOMException::INVALID_STATE_ERR);

     int exceptioncode = 0;

     TreeWalkerImpl *tw = static_cast<DocumentImpl *>(impl)->createTreeWalker(
         root.handle(), whatToShow, filter.handle(), entityReferenceExpansion, exceptioncode);
     if (exceptioncode)
         throw DOMException(exceptioncode);

     return tw;
}

Event Document::createEvent(const DOMString &eventType)
{
    if (!impl)
	throw DOMException(DOMException::INVALID_STATE_ERR);

    int exceptioncode = 0;
    EventImpl *r = ((DocumentImpl *)impl)->createEvent(eventType,exceptioncode);
    if (exceptioncode)
	throw DOMException(exceptioncode);
    return r;
}

AbstractView Document::defaultView() const
{
    if (!impl)
	throw DOMException(DOMException::INVALID_STATE_ERR);

    return static_cast<DocumentImpl*>(impl)->defaultView();
}

StyleSheetList Document::styleSheets() const
{
    if (!impl)
	throw DOMException(DOMException::INVALID_STATE_ERR);

    return static_cast<DocumentImpl*>(impl)->styleSheets();
}

DOMString Document::preferredStylesheetSet()
{
    if (!impl)
        throw DOMException(DOMException::INVALID_STATE_ERR);

    return static_cast<DocumentImpl*>(impl)->preferredStylesheetSet();
}

DOMString Document::selectedStylesheetSet()
{
    if (!impl)
        throw DOMException(DOMException::INVALID_STATE_ERR);

    return static_cast<DocumentImpl*>(impl)->selectedStylesheetSet();
}

void Document::setSelectedStylesheetSet(const DOMString& s)
{
    if (!impl)
        throw DOMException(DOMException::INVALID_STATE_ERR);

    static_cast<DocumentImpl*>(impl)->setSelectedStylesheetSet(s);
}


KHTMLView *Document::view() const
{
    if (!impl) return 0;

    return static_cast<DocumentImpl*>(impl)->view();
}

CSSStyleDeclaration Document::getOverrideStyle(const Element &elt, const DOMString &pseudoElt)
{
    if (!impl)
	throw DOMException(DOMException::INVALID_STATE_ERR);

    CSSStyleDeclarationImpl *r = ((DocumentImpl *)impl)->getOverrideStyle(static_cast<ElementImpl*>(elt.handle()),pseudoElt.implementation());
    return r;
}

bool Document::execCommand(const DOMString &command, bool userInterface, const DOMString &value)
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);
  
    return static_cast<DocumentImpl*>(impl)->execCommand(command, userInterface, value);
}

bool Document::queryCommandEnabled(const DOMString &command)
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);
  
    return static_cast<DocumentImpl*>(impl)->queryCommandEnabled(command);
}

bool Document::queryCommandIndeterm(const DOMString &command)
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);
  
    return static_cast<DocumentImpl*>(impl)->queryCommandIndeterm(command);
}

bool Document::queryCommandState(const DOMString &command)
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);
  
    return static_cast<DocumentImpl*>(impl)->queryCommandState(command);
}

bool Document::queryCommandSupported(const DOMString &command)
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);
  
    return static_cast<DocumentImpl*>(impl)->queryCommandSupported(command);
}

DOMString Document::queryCommandValue(const DOMString &command)
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);
  
    return static_cast<DocumentImpl*>(impl)->queryCommandValue(command);
}

bool Document::async() const
{
    if (!impl)
        throw DOMException(DOMException::INVALID_STATE_ERR);

    return static_cast<DocumentImpl*>( impl )->async(  );
}

void Document::setAsync( bool b )
{
    if (!impl)
        throw DOMException(DOMException::INVALID_STATE_ERR);

    static_cast<DocumentImpl*>( impl )->setAsync( b );
}

void Document::abort()
{
    if (!impl)
        throw DOMException(DOMException::INVALID_STATE_ERR);


    static_cast<DocumentImpl*>( impl )->abort(  );
}

void Document::load( const DOMString &uri )
{
    if (!impl)
        throw DOMException(DOMException::INVALID_STATE_ERR);

    static_cast<DocumentImpl*>( impl )->load( uri );
}

void Document::loadXML( const DOMString &source )
{
    if (!impl)
        throw DOMException(DOMException::INVALID_STATE_ERR);


    static_cast<DocumentImpl*>( impl )->loadXML( source );
}

Element Document::querySelector(const DOMString& query) const
{
    int ec = 0;
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    Element res = impl->querySelector(query, ec).get();
    if (ec)
        throw DOMException(ec);
    return res;
}

NodeList Document::querySelectorAll(const DOMString& query) const
{
    int ec = 0;
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    NodeList res = impl->querySelectorAll(query, ec).get();
    if (ec)
        throw DOMException(ec);
    return res;
}

bool Document::designMode() const {
    if (!impl)
        throw DOMException(DOMException::INVALID_STATE_ERR);

    return static_cast<DocumentImpl*>( impl )->designMode();
}

void Document::setDesignMode(bool enable) {
    if (!impl)
        throw DOMException(DOMException::INVALID_STATE_ERR);

    static_cast<DocumentImpl*>( impl )->setDesignMode( enable );
}

DOMString Document::completeURL(const DOMString& url)
{
    if ( !impl ) return url;
    return static_cast<DocumentImpl*>( impl )->completeURL( url.string() );
}

DOMString Document::toString() const
{
    if (!impl)
	throw DOMException(DOMException::NOT_FOUND_ERR);

    return static_cast<DocumentImpl*>(impl)->toString();
}

void Document::updateRendering()
{
    if ( !impl ) return;
    static_cast<DocumentImpl*>( impl )->updateRendering(  );
}

void Document::addStyleSheet(const StyleSheet &sheet)
{
    if (!impl || sheet.isNull())
        throw DOMException(DOMException::INVALID_STATE_ERR);

    int exceptioncode;
    static_cast<DocumentImpl*>( impl )->addStyleSheet( sheet.handle(), &exceptioncode );
    if (exceptioncode)
        throw DOMException(exceptioncode);
}

void Document::removeStyleSheet(const StyleSheet &sheet)
{
    if (!impl || sheet.isNull())
        throw DOMException(DOMException::INVALID_STATE_ERR);

    int exceptioncode;
    static_cast<DocumentImpl*>( impl )->removeStyleSheet( sheet.handle(), &exceptioncode );
    if (exceptioncode)
        throw DOMException(exceptioncode);
}

// ----------------------------------------------------------------------------

DocumentFragment::DocumentFragment() : Node()
{
}

DocumentFragment::DocumentFragment(const DocumentFragment &other) : Node(other)
{
}

DocumentFragment &DocumentFragment::operator = (const Node &other)
{
    NodeImpl* ohandle = other.handle();
    if ( impl != ohandle ) {
        if (!ohandle || ohandle->nodeType() != DOCUMENT_FRAGMENT_NODE) {
            if ( impl ) impl->deref();
            impl = 0;
        } else {
            Node::operator =(other);
        }
    }
    return *this;
}

DocumentFragment &DocumentFragment::operator = (const DocumentFragment &other)
{
    Node::operator =(other);
    return *this;
}

DocumentFragment::~DocumentFragment()
{
}

Element DocumentFragment::querySelector(const DOMString& query) const
{
    int ec = 0;
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    Element res = impl->querySelector(query, ec).get();
    if (ec)
        throw DOMException(ec);
    return res;
}

NodeList DocumentFragment::querySelectorAll(const DOMString& query) const
{
    int ec = 0;
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    NodeList res = impl->querySelectorAll(query, ec).get();
    if (ec)
        throw DOMException(ec);
    return res;
}

DocumentFragment::DocumentFragment(DocumentFragmentImpl *i) : Node(i)
{
}

// ----------------------------------------------------------------------------

DocumentType::DocumentType()
    : Node()
{
}

DocumentType::DocumentType(const DocumentType &other)
    : Node(other)
{
}

DocumentType::DocumentType(DocumentTypeImpl *impl) : Node(impl)
{
}

DocumentType &DocumentType::operator = (const Node &other)
{
    NodeImpl* ohandle = other.handle();
    if ( impl != ohandle ) {
        if (!ohandle || ohandle->nodeType() != DOCUMENT_TYPE_NODE) {
	    if ( impl ) impl->deref();
            impl = 0;
	} else {
            Node::operator =(other);
	}
    }
    return *this;
}

DocumentType &DocumentType::operator = (const DocumentType &other)
{
    Node::operator =(other);
    return *this;
}

DocumentType::~DocumentType()
{
}

DOMString DocumentType::name() const
{
    if (!impl)
	return DOMString(); // ### enable throw DOMException(DOMException::NOT_FOUND_ERR);

    return static_cast<DocumentTypeImpl*>(impl)->name();
}

NamedNodeMap DocumentType::entities() const
{
    if (!impl)
	return 0; // ### enable throw DOMException(DOMException::NOT_FOUND_ERR);

    return static_cast<DocumentTypeImpl*>(impl)->entities();
}

NamedNodeMap DocumentType::notations() const
{
    if (!impl)
	return 0; // ### enable throw DOMException(DOMException::NOT_FOUND_ERR);

    return static_cast<DocumentTypeImpl*>(impl)->notations();
}

DOMString DocumentType::publicId() const
{
    if (!impl)
	throw DOMException(DOMException::NOT_FOUND_ERR);

    return static_cast<DocumentTypeImpl*>(impl)->publicId();
}

DOMString DocumentType::systemId() const
{
    if (!impl)
	throw DOMException(DOMException::NOT_FOUND_ERR);

    return static_cast<DocumentTypeImpl*>(impl)->systemId();
}

DOMString DocumentType::internalSubset() const
{
    if (!impl)
	throw DOMException(DOMException::NOT_FOUND_ERR);

    return static_cast<DocumentTypeImpl*>(impl)->internalSubset();
}

} // namespace
