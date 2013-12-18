/**
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
#include "xml/dom_docimpl.h"
#include "xml/dom_elementimpl.h"
#include "html/html_formimpl.h"

using namespace DOM;

Attr::Attr() : Node()
{
}

Attr::Attr(const Attr &other) : Node(other)
{
}

Attr::Attr( AttrImpl *_impl )
{
    impl= _impl;
    if (impl) impl->ref();
}

Attr &Attr::operator = (const Node &other)
{
    NodeImpl* ohandle = other.handle();
    if ( impl != ohandle ) {
        if (!ohandle || !ohandle->isAttributeNode()) {
            if (impl) impl->deref();
            impl = 0;
        } else {
            Node::operator =(other);
        }
    }
    return *this;
}

Attr &Attr::operator = (const Attr &other)
{
    Node::operator =(other);
    return *this;
}

Attr::~Attr()
{
}

DOMString Attr::name() const
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    return ((AttrImpl *)impl)->name();
}

bool Attr::specified() const
{
  if (impl) return ((AttrImpl *)impl)->specified();
  return 0;
}

Element Attr::ownerElement() const
{
  if (!impl) return 0;
  return static_cast<AttrImpl*>(impl)->ownerElement();
}

DOMString Attr::value() const
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    return impl->nodeValue();
}

void Attr::setValue( const DOMString &newValue )
{
  if (!impl)
    return;

  int exceptioncode = 0;
  ((AttrImpl *)impl)->setValue(newValue,exceptioncode);
  if (exceptioncode)
    throw DOMException(exceptioncode);
}

// ---------------------------------------------------------------------------

Element::Element() : Node()
{
}

Element::Element(const Element &other) : Node(other)
{
}

Element::Element(ElementImpl *impl) : Node(impl)
{
}

Element &Element::operator = (const Node &other)
{
    NodeImpl* ohandle = other.handle();
    if ( impl != ohandle ) {
        if (!ohandle || !ohandle->isElementNode()) {
            if (impl) impl->deref();
            impl = 0;
	} else {
            Node::operator =(other);
	}
    }
    return *this;
}

Element &Element::operator = (const Element &other)
{
    Node::operator =(other);
    return *this;
}

Element::~Element()
{
}

DOMString Element::tagName() const
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    return static_cast<ElementImpl*>(impl)->tagName();
}

DOMString Element::getAttribute( const DOMString &name )
{
    // ### getAttribute() and getAttributeNS() are supposed to return the empty string if the attribute
    // does not exist. However, there are a number of places around khtml that expect a null string
    // for nonexistent attributes. These need to be changed to use hasAttribute() instead.
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    if (!name.implementation()) throw DOMException(DOMException::NOT_FOUND_ERR);

    return static_cast<ElementImpl*>(impl)->getAttribute(name);
}

void Element::setAttribute( const DOMString &name, const DOMString &value )
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    int exceptioncode = 0;
    static_cast<ElementImpl*>(impl)->setAttribute(name, value, exceptioncode);
    if ( exceptioncode )
        throw DOMException( exceptioncode );
}

void Element::removeAttribute( const DOMString &name )
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);

    int exceptioncode = 0;
    static_cast<ElementImpl*>(impl)->removeAttribute(name, exceptioncode);
    // it's allowed to remove attributes that don't exist.
    if ( exceptioncode && exceptioncode != DOMException::NOT_FOUND_ERR )
        throw DOMException( exceptioncode );
}

Attr Element::getAttributeNode( const DOMString &name )
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    if (!name.implementation()) throw DOMException(DOMException::NOT_FOUND_ERR);

    return static_cast<ElementImpl*>(impl)->getAttributeNode(name);
}

Attr Element::setAttributeNode( const Attr &newAttr )
{
    if (!impl || newAttr.isNull())
        throw DOMException(DOMException::NOT_FOUND_ERR);
    // WRONG_DOCUMENT_ERR and INUSE_ATTRIBUTE_ERR are already tested & thrown by setNamedItem

    int exceptioncode = 0;
    Attr r = static_cast<ElementImpl*>(impl)->setAttributeNode(
        static_cast<AttrImpl*>(newAttr.handle()), exceptioncode);
    if ( exceptioncode )
        throw DOMException( exceptioncode );
    return r;
}

Attr Element::removeAttributeNode( const Attr &oldAttr )
{
    int exceptioncode = 0;
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);

    Attr ret = static_cast<ElementImpl*>(impl)->removeAttributeNode(
      static_cast<AttrImpl*>(oldAttr.handle()), exceptioncode);
    if (exceptioncode)
        throw DOMException(exceptioncode);

    return ret;
}

NodeList Element::getElementsByTagName( const DOMString &tagName )
{
    if (!impl) return 0;
    return static_cast<ElementImpl*>(impl)->getElementsByTagName( tagName );
}

NodeList Element::getElementsByTagNameNS( const DOMString &namespaceURI,
                                          const DOMString &localName )
{
    if (!impl) return 0;
    return static_cast<ElementImpl*>(impl)->getElementsByTagNameNS( namespaceURI, localName );
}

NodeList Element::getElementsByClassName( const DOMString& className )
{
    if (!impl) return 0;
    return impl->getElementsByClassName( className );
}

DOMString Element::getAttributeNS( const DOMString &namespaceURI,
                                   const DOMString &localName)
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    ElementImpl* e = static_cast<ElementImpl*>(impl);
    int exceptioncode = 0;
    DOMString ret = e->getAttributeNS(namespaceURI, localName, exceptioncode);
    if (exceptioncode)
        throw DOMException(exceptioncode);
    return ret;
}

void Element::setAttributeNS( const DOMString &namespaceURI,
                              const DOMString &qualifiedName,
                              const DOMString &value)
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);

    int exceptioncode = 0;
    static_cast<ElementImpl*>(impl)->setAttributeNS(namespaceURI, qualifiedName, value, exceptioncode);
    if ( exceptioncode )
        throw DOMException( exceptioncode );
}

void Element::removeAttributeNS( const DOMString &namespaceURI,
                                 const DOMString &localName )
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);

    int exceptioncode = 0;
    static_cast<ElementImpl*>(impl)->removeAttributeNS(namespaceURI, localName, exceptioncode);
    if ( exceptioncode )
        throw DOMException( exceptioncode );
}

Attr Element::getAttributeNodeNS( const DOMString &namespaceURI,
                                  const DOMString &localName )
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);

    int exceptioncode = 0;
    Attr r = static_cast<ElementImpl*>(impl)->getAttributeNodeNS(namespaceURI, localName, exceptioncode);
    if (exceptioncode)
        throw DOMException( exceptioncode );
    return r;
}

Attr Element::setAttributeNodeNS( const Attr &newAttr )
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);

    int exceptioncode = 0;
    Attr r = static_cast<ElementImpl*>(impl)->setAttributeNodeNS(
      static_cast<AttrImpl*>(newAttr.handle()), exceptioncode);
    return r;
}

bool Element::hasAttribute( const DOMString& name )
{
    if (!impl) return false; // ### throw ?
    return static_cast<ElementImpl*>(impl)->hasAttribute(name);
}

bool Element::hasAttributeNS( const DOMString &namespaceURI,
                              const DOMString &localName )
{
    if (!impl) return false;
    return static_cast<ElementImpl*>(impl)->hasAttributeNS(namespaceURI, localName);
}

bool Element::isHTMLElement() const
{
    if(!impl) return false;
    return ((ElementImpl *)impl)->isHTMLElement();
}

Element Element::form() const
{
    if (!impl || !impl->isGenericFormElement()) return 0;
    return static_cast<HTMLGenericFormElementImpl*>(impl)->form();
    ElementImpl* f = static_cast<HTMLGenericFormElementImpl*>( impl )->form();

    if( f && f->implicitNode() )
        return 0;
    return f;
}

CSSStyleDeclaration Element::style()
{
    if (impl) return ((ElementImpl *)impl)->getInlineStyleDecls();
    return 0;
}

Element Element::firstElementChild() const
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    return static_cast<ElementImpl*>(impl)->firstElementChild();
}

Element Element::lastElementChild() const
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    return static_cast<ElementImpl*>(impl)->lastElementChild();
}

Element Element::previousElementSibling() const
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    return static_cast<ElementImpl*>(impl)->previousElementSibling();
}

Element Element::nextElementSibling() const
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    return static_cast<ElementImpl*>(impl)->nextElementSibling();
}

unsigned long Element::childElementCount() const
{
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    return static_cast<ElementImpl*>(impl)->childElementCount();
}

Element Element::querySelector(const DOMString& query) const
{
    int ec = 0;
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    Element res = impl->querySelector(query, ec).get();
    if (ec)
        throw DOMException(ec);
    return res;
}

NodeList Element::querySelectorAll(const DOMString& query) const
{
    int ec = 0;
    if (!impl) throw DOMException(DOMException::NOT_FOUND_ERR);
    NodeList res = impl->querySelectorAll(query, ec).get();
    if (ec)
        throw DOMException(ec);
    return res;
}

static inline bool isExtender(ushort c)
{ return c > 0x00B6 &&
        (c == 0x00B7 || c == 0x02D0 || c == 0x02D1 || c == 0x0387 ||
         c == 0x0640 || c == 0x0E46 || c == 0x0EC6 || c == 0x3005 ||
         (c >= 0x3031 && c <= 0x3035) ||
         (c >= 0x309D && c <= 0x309E) ||
         (c >= 0x30FC && c <= 0x30FE)
        );
}

bool Element::khtmlValidAttrName(const DOMString &name)
{
    // Check if name is valid
    // http://www.w3.org/TR/2000/REC-xml-20001006#NT-Name
    DOMStringImpl* _name = name.implementation();
    QChar ch = _name->s[0];
    if ( !ch.isLetter() && ch != '_' && ch != ':' )
        return false; // first char isn't valid
    for ( uint i = 0; i < _name->l; ++i )
    {
        ch = _name->s[i];
        if ( !ch.isLetter() && !ch.isDigit() && ch != '.'
             && ch != '-' && ch != '_' && ch != ':'
             && ch.category() != QChar::Mark_SpacingCombining
             && !isExtender(ch.unicode()) )
            return false;
    }
    return true;
}

bool Element::khtmlValidPrefix(const DOMString &name)
{
    // Null prefix is ok. If not null, reuse code from khtmlValidAttrName
    return !name.implementation() || khtmlValidAttrName(name);
}

bool Element::khtmlValidQualifiedName(const DOMString &name)
{
    return khtmlValidAttrName(name);
}

bool Element::khtmlMalformedQualifiedName(const DOMString &name)
{
    // #### see XML Namespaces spec for possibly more

    // ### does this disctinction make sense?
    if (name.isNull())
	return true;
    if (name.isEmpty())
	return false;

    // a prefix is optional but both prefix as well as local part
    // cannot be empty
    int colonpos = name.find(':');
    if (colonpos == 0 || colonpos == name.length() - 1)
	return true;

    return false;
}

bool Element::khtmlMalformedPrefix(const DOMString &/*name*/)
{
    // ####
    return false;
}
