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
// --------------------------------------------------------------------------

#include "html_image.h"
#include "dom_doc.h"
#include "html_misc.h"

#include <html/html_imageimpl.h>
#include <html/html_miscimpl.h>
#include <xml/dom_docimpl.h>

using namespace DOM;

HTMLAreaElement::HTMLAreaElement() : HTMLElement()
{
}

HTMLAreaElement::HTMLAreaElement(const HTMLAreaElement &other) : HTMLElement(other)
{
}

HTMLAreaElement::HTMLAreaElement(HTMLAreaElementImpl *impl) : HTMLElement(impl)
{
}

HTMLAreaElement &HTMLAreaElement::operator = (const Node &other)
{
    assignOther( other, ID_AREA );
    return *this;
}

HTMLAreaElement &HTMLAreaElement::operator = (const HTMLAreaElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLAreaElement::~HTMLAreaElement()
{
}

DOMString HTMLAreaElement::accessKey() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_ACCESSKEY);
}

void HTMLAreaElement::setAccessKey( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_ACCESSKEY, value);
}

DOMString HTMLAreaElement::alt() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_ALT);
}

void HTMLAreaElement::setAlt( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_ALT, value);
}

DOMString HTMLAreaElement::coords() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_COORDS);
}

void HTMLAreaElement::setCoords( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_COORDS, value);
}

DOMString HTMLAreaElement::href() const
{
    if(!impl) return DOMString();
    DOMString href = static_cast<ElementImpl*>(impl)->getAttribute(ATTR_HREF);
    return !href.isNull() ? impl->document()->completeURL(href.string()) : href;
}

void HTMLAreaElement::setHref( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_HREF, value);
}

bool HTMLAreaElement::noHref() const
{
    if(!impl) return 0;
    return !((ElementImpl *)impl)->getAttribute(ATTR_NOHREF).isNull();
}

void HTMLAreaElement::setNoHref( bool _noHref )
{
    if(impl)
    {
	DOMString str;
	if( _noHref )
	    str = "";
	((ElementImpl *)impl)->setAttribute(ATTR_NOHREF, str);
    }
}

DOMString HTMLAreaElement::shape() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_SHAPE);
}

void HTMLAreaElement::setShape( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_SHAPE, value);
}

long HTMLAreaElement::tabIndex() const
{
    if(!impl) return 0;
    return ((ElementImpl *)impl)->getAttribute(ATTR_TABINDEX).toInt();
}

void HTMLAreaElement::setTabIndex( long _tabIndex )
{
    if(impl) {
	DOMString value(QString::number(_tabIndex));
        ((ElementImpl *)impl)->setAttribute(ATTR_TABINDEX,value);
    }
}

DOMString HTMLAreaElement::target() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_TARGET);
}

void HTMLAreaElement::setTarget( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_TARGET, value);
}

// --------------------------------------------------------------------------

HTMLImageElement::HTMLImageElement() : HTMLElement()
{
}

HTMLImageElement::HTMLImageElement(const HTMLImageElement &other) : HTMLElement(other)
{
}

HTMLImageElement::HTMLImageElement(HTMLImageElementImpl *impl) : HTMLElement(impl)
{
}

HTMLImageElement &HTMLImageElement::operator = (const Node &other)
{
    assignOther( other, ID_IMG );
    return *this;
}

HTMLImageElement &HTMLImageElement::operator = (const HTMLImageElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLImageElement::~HTMLImageElement()
{
}

DOMString HTMLImageElement::name() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_NAME);
}

void HTMLImageElement::setName( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_NAME, value);
}

DOMString HTMLImageElement::align() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_ALIGN);
}

void HTMLImageElement::setAlign( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_ALIGN, value);
}

DOMString HTMLImageElement::alt() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_ALT);
}

void HTMLImageElement::setAlt( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_ALT, value);
}

#ifndef KDE_NO_DEPRECATED
long HTMLImageElement::border() const
{
    if(!impl) return 0;
    // ### return value in pixels
    return static_cast<HTMLImageElementImpl*>(impl)->getAttribute(ATTR_BORDER).toInt();
}
#endif

#ifndef KDE_NO_DEPRECATED
void HTMLImageElement::setBorder( long value )
{
    if (impl) static_cast<HTMLImageElementImpl*>(impl)->setAttribute(ATTR_BORDER, QString::number(value));
}
#endif

DOMString HTMLImageElement::getBorder() const
{
    if(!impl) return DOMString();
    return static_cast<HTMLImageElementImpl*>(impl)->getAttribute(ATTR_BORDER);
}

void HTMLImageElement::setBorder( const DOMString& value )
{
    if (impl) static_cast<HTMLImageElementImpl*>(impl)->setAttribute(ATTR_BORDER, value);
}


long HTMLImageElement::height() const
{
    if(!impl) return 0;
    return static_cast<HTMLImageElementImpl*>(impl)->height();
}

void HTMLImageElement::setHeight( long value )
{
    if(impl) ((HTMLImageElementImpl *)impl)->setHeight(value);
}

long HTMLImageElement::hspace() const
{
    if(!impl) return 0;
    // ### return actual value
    return ((ElementImpl *)impl)->getAttribute(ATTR_HSPACE).toInt();
}

void HTMLImageElement::setHspace( long value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_HSPACE, QString::number(value));
}

bool HTMLImageElement::isMap() const
{
    if(!impl) return 0;
    return !((ElementImpl *)impl)->getAttribute(ATTR_ISMAP).isNull();
}

void HTMLImageElement::setIsMap( bool _isMap )
{
    if(impl)
    {
	DOMString str;
	if( _isMap )
	    str = "";
	((ElementImpl *)impl)->setAttribute(ATTR_ISMAP, str);
    }
}

DOMString HTMLImageElement::longDesc() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_LONGDESC);
}

void HTMLImageElement::setLongDesc( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_LONGDESC, value);
}

DOMString HTMLImageElement::src() const
{
    if(!impl) return DOMString();
    DOMString s = ((ElementImpl *)impl)->getAttribute(ATTR_SRC);
    return !s.isNull() ? impl->document()->completeURL( s.string() ) : s;
}

void HTMLImageElement::setSrc( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_SRC, value);
}

DOMString HTMLImageElement::useMap() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_USEMAP);
}

void HTMLImageElement::setUseMap( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_USEMAP, value);
}

long HTMLImageElement::vspace() const
{
    if(!impl) return 0;
    // ### return actual vspace
    return ((ElementImpl *)impl)->getAttribute(ATTR_VSPACE).toInt();
}

void HTMLImageElement::setVspace( long value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_VSPACE, QString::number(value));
}

long HTMLImageElement::width() const
{
    if(!impl) return 0;
    return static_cast<HTMLImageElementImpl*>(impl)->width();
}

void HTMLImageElement::setWidth( long value )
{
    if(impl) ((HTMLImageElementImpl *)impl)->setWidth(value);
}

long HTMLImageElement::x() const
{
    if (impl) return static_cast<const HTMLImageElementImpl*>(impl)->x();
    return 0;
}

long HTMLImageElement::y() const
{
    if (impl) return static_cast<const HTMLImageElementImpl*>(impl)->y();
    return 0;
}

// --------------------------------------------------------------------------

HTMLMapElement::HTMLMapElement() : HTMLElement()
{
}

HTMLMapElement::HTMLMapElement(const HTMLMapElement &other) : HTMLElement(other)
{
}

HTMLMapElement::HTMLMapElement(HTMLMapElementImpl *impl) : HTMLElement(impl)
{
}

HTMLMapElement &HTMLMapElement::operator = (const Node &other)
{
    assignOther( other, ID_MAP );
    return *this;
}

HTMLMapElement &HTMLMapElement::operator = (const HTMLMapElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLMapElement::~HTMLMapElement()
{
}

HTMLCollection HTMLMapElement::areas() const
{
    if(!impl) return HTMLCollection();
    return HTMLCollection(((HTMLMapElementImpl*)impl)->areas());
}

DOMString HTMLMapElement::name() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_NAME);
}

void HTMLMapElement::setName( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_NAME, value);
}

