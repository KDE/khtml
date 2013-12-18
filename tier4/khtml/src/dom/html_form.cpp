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

#include "dom/html_form.h"
#include "dom/dom_exception.h"
#include "dom/dom_doc.h"

#include "html/html_formimpl.h"
#include "html/html_miscimpl.h"

#include "xml/dom_docimpl.h"

using namespace DOM;

HTMLButtonElement::HTMLButtonElement() : HTMLElement()
{
}

HTMLButtonElement::HTMLButtonElement(const HTMLButtonElement &other) : HTMLElement(other)
{
}

HTMLButtonElement::HTMLButtonElement(HTMLButtonElementImpl *impl) : HTMLElement(impl)
{
}

HTMLButtonElement &HTMLButtonElement::operator = (const Node &other)
{
    assignOther( other, ID_BUTTON );
    return *this;
}

HTMLButtonElement &HTMLButtonElement::operator = (const HTMLButtonElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLButtonElement::~HTMLButtonElement()
{
}

HTMLFormElement HTMLButtonElement::form() const
{
    return Element::form();
}

DOMString HTMLButtonElement::accessKey() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_ACCESSKEY);
}

void HTMLButtonElement::setAccessKey( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_ACCESSKEY, value);
}

bool HTMLButtonElement::disabled() const
{
    if(!impl) return 0;
    return !static_cast<ElementImpl*>(impl)->getAttribute(ATTR_DISABLED).isNull();
}

void HTMLButtonElement::setDisabled( bool _disabled )
{
    if (impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_DISABLED, _disabled ? "" : 0);
}

DOMString HTMLButtonElement::name() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_NAME);
}

void HTMLButtonElement::setName( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_NAME, value);
}

void HTMLButtonElement::focus(  )
{
    if(impl)
        static_cast<HTMLButtonElementImpl*>(impl)->focus();
}

void HTMLButtonElement::blur(  )
{
    if(impl)
        static_cast<HTMLButtonElementImpl*>(impl)->blur();
}

long HTMLButtonElement::tabIndex() const
{
    if(!impl) return 0;
    return static_cast<ElementImpl*>(impl)->tabIndex();
}

void HTMLButtonElement::setTabIndex( long _tabIndex )
{
    if (!impl) return;
    static_cast<ElementImpl*>(impl)->setTabIndex(_tabIndex);
}

DOMString HTMLButtonElement::type() const
{
    if(!impl) return DOMString();
    return static_cast<HTMLButtonElementImpl*>(impl)->type();
}

DOMString HTMLButtonElement::value() const
{
    if(!impl) return DOMString();
    DOMString s = static_cast<ElementImpl*>(impl)->getAttribute(ATTR_VALUE);
    if (s.isNull()) return DOMString("");
    return s;
}

void HTMLButtonElement::setValue( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_VALUE, value);
}

// --------------------------------------------------------------------------

HTMLFieldSetElement::HTMLFieldSetElement() : HTMLElement()
{
}

HTMLFieldSetElement::HTMLFieldSetElement(const HTMLFieldSetElement &other) : HTMLElement(other)
{
}

HTMLFieldSetElement::HTMLFieldSetElement(HTMLFieldSetElementImpl *impl) : HTMLElement(impl)
{
}

HTMLFieldSetElement &HTMLFieldSetElement::operator = (const Node &other)
{
    assignOther( other, ID_FIELDSET );
    return *this;
}

HTMLFieldSetElement &HTMLFieldSetElement::operator = (const HTMLFieldSetElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLFieldSetElement::~HTMLFieldSetElement()
{
}

HTMLFormElement HTMLFieldSetElement::form() const
{
    return Element::form();
}

// --------------------------------------------------------------------------

HTMLFormElement::HTMLFormElement() : HTMLElement()
{
}

HTMLFormElement::HTMLFormElement(const HTMLFormElement &other) : HTMLElement(other)
{
}

HTMLFormElement::HTMLFormElement(HTMLFormElementImpl *impl) : HTMLElement(impl)
{
}

HTMLFormElement &HTMLFormElement::operator = (const Node &other)
{
    assignOther( other, ID_FORM );
    return *this;
}

HTMLFormElement &HTMLFormElement::operator = (const HTMLFormElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLFormElement::~HTMLFormElement()
{
}

HTMLCollection HTMLFormElement::elements() const
{
    if(!impl) return HTMLCollection();
    return HTMLFormCollection(impl);
}

long HTMLFormElement::length() const
{
    if(!impl) return 0;
    return static_cast<HTMLFormElementImpl*>(impl)->length();
}

DOMString HTMLFormElement::name() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_NAME);
}

void HTMLFormElement::setName( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_NAME, value);
}

DOMString HTMLFormElement::acceptCharset() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_ACCEPT_CHARSET);
}

void HTMLFormElement::setAcceptCharset( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_ACCEPT_CHARSET, value);
}

DOMString HTMLFormElement::action() const
{
    if(!impl) return DOMString();
    return static_cast<HTMLFormElementImpl*>(impl)->action();
}

void HTMLFormElement::setAction( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_ACTION, value);
}

DOMString HTMLFormElement::enctype() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_ENCTYPE);
}

void HTMLFormElement::setEnctype( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_ENCTYPE, value);
}

DOMString HTMLFormElement::method() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_METHOD);
}

void HTMLFormElement::setMethod( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_METHOD, value);
}

DOMString HTMLFormElement::target() const
{
    if(!impl) return DOMString();
    return static_cast<HTMLFormElementImpl*>(impl)->target();
}

void HTMLFormElement::setTarget( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_TARGET, value);
}

void HTMLFormElement::submit(  )
{
    if(impl) static_cast<HTMLFormElementImpl*>(impl)->submit(  );
}

void HTMLFormElement::reset(  )
{
    if(impl) static_cast<HTMLFormElementImpl*>(impl)->reset(  );
}

// --------------------------------------------------------------------------

HTMLInputElement::HTMLInputElement() : HTMLElement()
{
}

HTMLInputElement::HTMLInputElement(const HTMLInputElement &other) : HTMLElement(other)
{
}

HTMLInputElement::HTMLInputElement(HTMLInputElementImpl *impl) : HTMLElement(impl)
{
}

HTMLInputElement &HTMLInputElement::operator = (const Node &other)
{
    assignOther( other, ID_INPUT );
    return *this;
}

HTMLInputElement &HTMLInputElement::operator = (const HTMLInputElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLInputElement::~HTMLInputElement()
{
}

DOMString HTMLInputElement::defaultValue() const
{
    if(!impl) return DOMString();
    DOMString s = static_cast<ElementImpl*>(impl)->getAttribute(ATTR_VALUE);
    if (s.isNull()) return DOMString("");
    return s;

}

void HTMLInputElement::setDefaultValue( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_VALUE, value);
}

bool HTMLInputElement::defaultChecked() const
{
    if(!impl) return 0;
    return !((ElementImpl *)impl)->getAttribute(ATTR_CHECKED).isNull();
}

void HTMLInputElement::setDefaultChecked( bool _defaultChecked )
{
    if(impl)
	((ElementImpl *)impl)->setAttribute(ATTR_CHECKED, _defaultChecked ? "" : 0);
}

HTMLFormElement HTMLInputElement::form() const
{
    return Element::form();
}

DOMString HTMLInputElement::accept() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_ACCEPT);
}

void HTMLInputElement::setAccept( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_ACCEPT, value);
}

DOMString HTMLInputElement::accessKey() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_ACCESSKEY);
}

void HTMLInputElement::setAccessKey( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_ACCESSKEY, value);
}

DOMString HTMLInputElement::align() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_ALIGN);
}

void HTMLInputElement::setAlign( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_ALIGN, value);
}

DOMString HTMLInputElement::alt() const
{
    if(!impl) return DOMString();
    return ((ElementImpl *)impl)->getAttribute(ATTR_ALT);
}

void HTMLInputElement::setAlt( const DOMString &value )
{
    if(impl) ((ElementImpl *)impl)->setAttribute(ATTR_ALT, value);
}

bool HTMLInputElement::checked() const
{
    if(!impl) return 0;
    return ((HTMLInputElementImpl*)impl)->checked();
}

void HTMLInputElement::setChecked( bool _checked )
{
    if(impl)
	((HTMLInputElementImpl*)impl)->setChecked(_checked);
}

bool HTMLInputElement::indeterminate() const
{
    if(!impl) return 0;
    return ((HTMLInputElementImpl*)impl)->indeterminate();
}

void HTMLInputElement::setIndeterminate( bool _indeterminate )
{
    if(impl)
	((HTMLInputElementImpl*)impl)->setIndeterminate(_indeterminate);
}

bool HTMLInputElement::disabled() const
{
    if(!impl) return 0;
    return !((ElementImpl *)impl)->getAttribute(ATTR_DISABLED).isNull();
}

void HTMLInputElement::setDisabled( bool _disabled )
{
    if(impl)
    {
	((ElementImpl *)impl)->setAttribute(ATTR_DISABLED, _disabled ? "" : 0);
    }
}

long HTMLInputElement::maxLength() const
{
    if(!impl) return 0;
    return ((HTMLInputElementImpl *)impl)->getAttribute(ATTR_MAXLENGTH).toInt();
}

void HTMLInputElement::setMaxLength( long _maxLength )
{
    if(impl) {
        DOMString value(QString::number(_maxLength));
        ((ElementImpl *)impl)->setAttribute(ATTR_MAXLENGTH,value);
    }
}

DOMString HTMLInputElement::name() const
{
    if(!impl) return DOMString();
    return static_cast<HTMLInputElementImpl* const>(impl)->name();
}

void HTMLInputElement::setName( const DOMString &value )
{
    if(impl) static_cast<HTMLInputElementImpl*>(impl)->setName(value);
}

bool HTMLInputElement::readOnly() const
{
    if(!impl) return 0;
    return !static_cast<ElementImpl*>(impl)->getAttribute(ATTR_READONLY).isNull();
}

void HTMLInputElement::setReadOnly( bool _readOnly )
{
    if(impl)
	static_cast<ElementImpl*>(impl)->setAttribute(ATTR_READONLY, _readOnly ? "" : 0);
}

/* The next two are provided for backwards compatibility. */
#ifndef KDE_NO_DEPRECATED
DOMString HTMLInputElement::size() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_SIZE);
}
#endif

#ifndef KDE_NO_DEPRECATED
void HTMLInputElement::setSize( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_SIZE, value);
}
#endif

long HTMLInputElement::getSize() const
{
    if(!impl) return 0;
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_SIZE).toInt();
}

void HTMLInputElement::setSize( long value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_SIZE, QString::number(value));
}

DOMString HTMLInputElement::src() const
{
    if(!impl) return DOMString();
    DOMString s = static_cast<ElementImpl*>(impl)->getAttribute(ATTR_SRC);
    return !s.isNull() ? impl->document()->completeURL( s.string() ) : s;
}

void HTMLInputElement::setSrc( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_SRC, value);
}

long HTMLInputElement::tabIndex() const
{
    if(!impl) return 0;
    return static_cast<ElementImpl*>(impl)->tabIndex();
}

void HTMLInputElement::setTabIndex( long _tabIndex )
{
    if (!impl) return;
    static_cast<ElementImpl*>(impl)->setTabIndex(_tabIndex);
}

DOMString HTMLInputElement::type() const
{
    if(!impl) return DOMString();
    return ((HTMLInputElementImpl *)impl)->type();
}

void HTMLInputElement::setType(const DOMString& _type)
{
    if (!impl) return;
    static_cast<HTMLInputElementImpl*>(impl)->setType(_type);
}

DOMString HTMLInputElement::useMap() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_USEMAP);
}

void HTMLInputElement::setUseMap( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_USEMAP, value);
}

DOMString HTMLInputElement::value() const
{
    if(!impl) return DOMString();
    return ((HTMLInputElementImpl*)impl)->value();
}

void HTMLInputElement::setValue( const DOMString &value )
{
    if (impl)
	((HTMLInputElementImpl*)impl)->setValue(value);

}

void HTMLInputElement::blur(  )
{
    if(impl)
	((HTMLInputElementImpl*)impl)->blur();
}

void HTMLInputElement::focus(  )
{
    if(impl)
	((HTMLInputElementImpl*)impl)->focus();
}

void HTMLInputElement::select(  )
{
    if(impl)
	((HTMLInputElementImpl *)impl)->select(  );
}

void HTMLInputElement::click(  )
{
    if(impl)
	((HTMLInputElementImpl *)impl)->click(  );
}

long HTMLInputElement::selectionStart()
{
    if (impl)
	return ((HTMLInputElementImpl *)impl)->selectionStart(  );
    return -1;
}

long HTMLInputElement::selectionEnd()
{
    if (impl)
	return ((HTMLInputElementImpl *)impl)->selectionEnd(  );
    return -1;
}

void HTMLInputElement::setSelectionStart(long pos)
{
    if (impl)
	((HTMLInputElementImpl *)impl)->setSelectionStart( pos );
}

void HTMLInputElement::setSelectionEnd(long pos)
{
    if (impl)
	((HTMLInputElementImpl *)impl)->setSelectionEnd( pos );
}

void HTMLInputElement::setSelectionRange(long start, long end)
{
    if (impl)
	((HTMLInputElementImpl *)impl)->setSelectionRange( start, end );
}

// --------------------------------------------------------------------------

HTMLLabelElement::HTMLLabelElement() : HTMLElement()
{
}

HTMLLabelElement::HTMLLabelElement(const HTMLLabelElement &other) : HTMLElement(other)
{
}

HTMLLabelElement::HTMLLabelElement(HTMLLabelElementImpl *impl) : HTMLElement(impl)
{
}

HTMLLabelElement &HTMLLabelElement::operator = (const Node &other)
{
    assignOther( other, ID_LABEL );
    return *this;
}

HTMLLabelElement &HTMLLabelElement::operator = (const HTMLLabelElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLLabelElement::~HTMLLabelElement()
{
}

DOMString HTMLLabelElement::accessKey() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_ACCESSKEY);
}

void HTMLLabelElement::setAccessKey( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_ACCESSKEY, value);
}

DOMString HTMLLabelElement::htmlFor() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_FOR);
}

void HTMLLabelElement::setHtmlFor( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_FOR, value);
}

// --------------------------------------------------------------------------

HTMLLegendElement::HTMLLegendElement() : HTMLElement()
{
}

HTMLLegendElement::HTMLLegendElement(const HTMLLegendElement &other) : HTMLElement(other)
{
}

HTMLLegendElement::HTMLLegendElement(HTMLLegendElementImpl *impl) : HTMLElement(impl)
{
}

HTMLLegendElement &HTMLLegendElement::operator = (const Node &other)
{
    assignOther( other, ID_LEGEND );
    return *this;
}

HTMLLegendElement &HTMLLegendElement::operator = (const HTMLLegendElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLLegendElement::~HTMLLegendElement()
{
}

HTMLFormElement HTMLLegendElement::form() const
{
    return Element::form();
}

DOMString HTMLLegendElement::accessKey() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_ACCESSKEY);
}

void HTMLLegendElement::setAccessKey( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_ACCESSKEY, value);
}

DOMString HTMLLegendElement::align() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_ALIGN);
}

void HTMLLegendElement::setAlign( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_ALIGN, value);
}

// --------------------------------------------------------------------------

HTMLOptGroupElement::HTMLOptGroupElement() : HTMLElement()
{
}

HTMLOptGroupElement::HTMLOptGroupElement(const HTMLOptGroupElement &other) : HTMLElement(other)
{
}

HTMLOptGroupElement::HTMLOptGroupElement(HTMLOptGroupElementImpl *impl) : HTMLElement(impl)
{
}

HTMLOptGroupElement &HTMLOptGroupElement::operator = (const Node &other)
{
    assignOther( other, ID_OPTGROUP );
    return *this;
}

HTMLOptGroupElement &HTMLOptGroupElement::operator = (const HTMLOptGroupElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLOptGroupElement::~HTMLOptGroupElement()
{
}

bool HTMLOptGroupElement::disabled() const
{
    if(!impl) return 0;
    return !static_cast<ElementImpl*>(impl)->getAttribute(ATTR_DISABLED).isNull();
}

void HTMLOptGroupElement::setDisabled( bool _disabled )
{
    if(impl)
	static_cast<ElementImpl*>(impl)->setAttribute(ATTR_DISABLED, _disabled ? "" : 0);
}

DOMString HTMLOptGroupElement::label() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_LABEL);
}

void HTMLOptGroupElement::setLabel( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_LABEL, value);
}

// --------------------------------------------------------------------------

HTMLSelectElement::HTMLSelectElement() : HTMLElement()
{
}

HTMLSelectElement::HTMLSelectElement(const HTMLSelectElement &other) : HTMLElement(other)
{
}

HTMLSelectElement::HTMLSelectElement(HTMLSelectElementImpl *impl) : HTMLElement(impl)
{
}

HTMLSelectElement &HTMLSelectElement::operator = (const Node &other)
{
    assignOther( other, ID_SELECT );
    return *this;
}

HTMLSelectElement &HTMLSelectElement::operator = (const HTMLSelectElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLSelectElement::~HTMLSelectElement()
{
}

DOMString HTMLSelectElement::type() const
{
    if(!impl) return DOMString();
    return ((HTMLSelectElementImpl *)impl)->type();
}

long HTMLSelectElement::selectedIndex() const
{
    if(!impl) return 0;
    return ((HTMLSelectElementImpl *)impl)->selectedIndex();
}

void HTMLSelectElement::setSelectedIndex( long _selectedIndex )
{
    if(impl)
        ((HTMLSelectElementImpl *)impl)->setSelectedIndex(_selectedIndex);
}

DOMString HTMLSelectElement::value() const
{
    if(!impl) return DOMString();
    return static_cast<HTMLSelectElementImpl*>(impl)->value();
}

void HTMLSelectElement::setValue( const DOMString &value )
{
    if(!impl || value.isNull()) return;
    static_cast<HTMLSelectElementImpl*>(impl)->setValue(value.implementation());
}

long HTMLSelectElement::length() const
{
    if(!impl) return 0;
    return ((HTMLSelectElementImpl *)impl)->length();
}

HTMLFormElement HTMLSelectElement::form() const
{
    return Element::form();
}

HTMLCollection HTMLSelectElement::options() const
{
    if(!impl) return HTMLCollection();
    return HTMLCollection(((HTMLSelectElementImpl*)impl)->options());
}

bool HTMLSelectElement::disabled() const
{
    if(!impl) return 0;
    return !static_cast<ElementImpl*>(impl)->getAttribute(ATTR_DISABLED).isNull();
}

void HTMLSelectElement::setDisabled( bool _disabled )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_DISABLED, _disabled ? "" : 0);
}


bool HTMLSelectElement::multiple() const
{
    if(!impl) return 0;
    return !static_cast<ElementImpl*>(impl)->getAttribute(ATTR_MULTIPLE).isNull();
}

void HTMLSelectElement::setMultiple( bool _multiple )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_MULTIPLE, _multiple ? "" : 0);
}

DOMString HTMLSelectElement::name() const
{
    if(!impl) return DOMString();
    return static_cast<HTMLSelectElementImpl* const>(impl)->name();
}

void HTMLSelectElement::setName( const DOMString &value )
{
    if(impl) static_cast<HTMLSelectElementImpl*>(impl)->setName(value);
}

long HTMLSelectElement::size() const
{
    if(!impl) return 0;
    return ((HTMLSelectElementImpl *)impl)->getAttribute(ATTR_SIZE).toInt();
}

void HTMLSelectElement::setSize( long _size )
{

    if(impl) {
	DOMString value(QString::number(_size));
        static_cast<ElementImpl*>(impl)->setAttribute(ATTR_SIZE,value);
    }
}

long HTMLSelectElement::tabIndex() const
{
    if(!impl) return 0;
    return static_cast<ElementImpl*>(impl)->tabIndex();
}

void HTMLSelectElement::setTabIndex( long _tabIndex )
{
    if (!impl) return;
    static_cast<ElementImpl*>(impl)->setTabIndex(_tabIndex);
}

void HTMLSelectElement::add( const HTMLElement &element, const HTMLElement &before )
{
    if (!impl)
        throw DOMException(DOMException::NOT_FOUND_ERR);

    int exceptioncode = 0;
    static_cast<HTMLSelectElementImpl*>(impl)->add(
      static_cast<HTMLElementImpl*>(element.handle()),
      static_cast<HTMLElementImpl*>(before.handle()), exceptioncode );
    if ( exceptioncode )
        throw DOMException( exceptioncode );
}

void HTMLSelectElement::remove( long index )
{
    if(impl) static_cast<HTMLSelectElementImpl*>(impl)->remove( index );
}

void HTMLSelectElement::blur(  )
{
    if(impl)
	((HTMLSelectElementImpl*)impl)->blur();
}

void HTMLSelectElement::focus(  )
{
    if(impl)
	((HTMLSelectElementImpl*)impl)->focus();
}

// --------------------------------------------------------------------------

HTMLTextAreaElement::HTMLTextAreaElement() : HTMLElement()
{
}

HTMLTextAreaElement::HTMLTextAreaElement(const HTMLTextAreaElement &other) : HTMLElement(other)
{
}

HTMLTextAreaElement::HTMLTextAreaElement(HTMLTextAreaElementImpl *impl) : HTMLElement(impl)
{
}

HTMLTextAreaElement &HTMLTextAreaElement::operator = (const Node &other)
{
    assignOther( other, ID_TEXTAREA );
    return *this;
}

HTMLTextAreaElement &HTMLTextAreaElement::operator = (const HTMLTextAreaElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLTextAreaElement::~HTMLTextAreaElement()
{
}

DOMString HTMLTextAreaElement::defaultValue() const
{
    if(!impl) return DOMString();
    return ((HTMLTextAreaElementImpl *)impl)->defaultValue();
}

void HTMLTextAreaElement::setDefaultValue( const DOMString &value )
{
    if (impl) ((HTMLTextAreaElementImpl *)impl)->setDefaultValue(value);
}

HTMLFormElement HTMLTextAreaElement::form() const
{
    return Element::form();
}

DOMString HTMLTextAreaElement::accessKey() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_ACCESSKEY);
}

void HTMLTextAreaElement::setAccessKey( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_ACCESSKEY, value);
}

long HTMLTextAreaElement::cols() const
{
    if(!impl) return 0;
    return ((HTMLTextAreaElementImpl *)impl)->getAttribute(ATTR_COLS).toInt();
}

void HTMLTextAreaElement::setCols( long _cols )
{

    if(impl) {
	DOMString value(QString::number(_cols));
        static_cast<ElementImpl*>(impl)->setAttribute(ATTR_COLS,value);
    }
}

bool HTMLTextAreaElement::disabled() const
{
    if(!impl) return 0;
    return !static_cast<ElementImpl*>(impl)->getAttribute(ATTR_DISABLED).isNull();
}

void HTMLTextAreaElement::setDisabled( bool _disabled )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_DISABLED, _disabled ? "" : 0);
}

DOMString HTMLTextAreaElement::name() const
{
    if(!impl) return DOMString();
    return static_cast<HTMLTextAreaElementImpl* const>(impl)->name();
}

void HTMLTextAreaElement::setName( const DOMString &value )
{
    if(impl) static_cast<HTMLTextAreaElementImpl*>(impl)->setName(value);
}

bool HTMLTextAreaElement::readOnly() const
{
    if(!impl) return 0;
    return !static_cast<ElementImpl*>(impl)->getAttribute(ATTR_READONLY).isNull();
}

void HTMLTextAreaElement::setReadOnly( bool _readOnly )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_READONLY, _readOnly ? "" : 0);
}

long HTMLTextAreaElement::rows() const
{
    if(!impl) return 0;
    return ((HTMLTextAreaElementImpl *)impl)->getAttribute(ATTR_ROWS).toInt();
}

void HTMLTextAreaElement::setRows( long _rows )
{

    if(impl) {
	DOMString value(QString::number(_rows));
        static_cast<ElementImpl*>(impl)->setAttribute(ATTR_ROWS,value);
    }
}

long HTMLTextAreaElement::tabIndex() const
{
    if(!impl) return 0;
    return static_cast<ElementImpl*>(impl)->tabIndex();
}

void HTMLTextAreaElement::setTabIndex( long _tabIndex )
{
    if (!impl) return;
    static_cast<ElementImpl*>(impl)->setTabIndex(_tabIndex);
}

DOMString HTMLTextAreaElement::type() const
{
    if(!impl) return DOMString();
    return ((HTMLTextAreaElementImpl *)impl)->type();
}

DOMString HTMLTextAreaElement::value() const
{
    if(!impl) return DOMString();
    return ((HTMLTextAreaElementImpl *)impl)->value();
}

void HTMLTextAreaElement::setValue( const DOMString &value )
{
    if(impl) ((HTMLTextAreaElementImpl *)impl)->setValue(value);
}

void HTMLTextAreaElement::blur(  )
{
    if(impl)
	((HTMLTextAreaElementImpl*)impl)->blur();
}

void HTMLTextAreaElement::focus(  )
{
    if(impl)
	((HTMLTextAreaElementImpl*)impl)->focus();
}

void HTMLTextAreaElement::select(  )
{
    if(impl)
	((HTMLTextAreaElementImpl *)impl)->select(  );
}

long HTMLTextAreaElement::selectionStart()
{
    if (impl)
	return ((HTMLTextAreaElementImpl *)impl)->selectionStart(  );
    return 0;
}

long HTMLTextAreaElement::selectionEnd()
{
    if (impl)
	return ((HTMLTextAreaElementImpl *)impl)->selectionEnd(  );
    return 0;
}

long HTMLTextAreaElement::textLength()
{
    if (impl)
	return ((HTMLTextAreaElementImpl *)impl)->textLength(  );
    return 0;
}

void HTMLTextAreaElement::setSelectionStart(long pos)
{
    if (impl)
	((HTMLTextAreaElementImpl *)impl)->setSelectionStart( pos );
}

void HTMLTextAreaElement::setSelectionEnd(long pos)
{
    if (impl)
	((HTMLTextAreaElementImpl *)impl)->setSelectionEnd( pos );
}

void HTMLTextAreaElement::setSelectionRange(long start, long end)
{
    if (impl)
	((HTMLTextAreaElementImpl *)impl)->setSelectionRange( start, end );
}

// --------------------------------------------------------------------------

HTMLOptionElement::HTMLOptionElement() : HTMLElement()
{
}

HTMLOptionElement::HTMLOptionElement(const HTMLOptionElement &other) : HTMLElement(other)
{
}

HTMLOptionElement::HTMLOptionElement(HTMLOptionElementImpl *impl) : HTMLElement(impl)
{
}

HTMLOptionElement &HTMLOptionElement::operator = (const Node &other)
{
    assignOther( other, ID_OPTION );
    return *this;
}

HTMLOptionElement &HTMLOptionElement::operator = (const HTMLOptionElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLOptionElement::~HTMLOptionElement()
{
}

HTMLFormElement HTMLOptionElement::form() const
{
    return Element::form();
}

bool HTMLOptionElement::defaultSelected() const
{
    if(!impl) return 0;
    return !static_cast<ElementImpl*>(impl)->getAttribute(ATTR_SELECTED).isNull();
}

void HTMLOptionElement::setDefaultSelected( bool _defaultSelected )
{
    if(impl) static_cast<HTMLOptionElementImpl*>(impl)->setDefaultSelected(_defaultSelected);
}

DOMString HTMLOptionElement::text() const
{
    if(!impl) return DOMString();
    return ((HTMLOptionElementImpl *)impl)->text();
}

long HTMLOptionElement::index() const
{
    if(!impl) return 0;
    return ((HTMLOptionElementImpl *)impl)->index();
}

void HTMLOptionElement::setIndex( long /*_index*/ )
{
    throw DOMException(DOMException::NO_MODIFICATION_ALLOWED_ERR);
}

bool HTMLOptionElement::disabled() const
{
    if(!impl) return 0;
    return !static_cast<ElementImpl*>(impl)->getAttribute(ATTR_DISABLED).isNull();
}

void HTMLOptionElement::setDisabled( bool _disabled )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_DISABLED, _disabled ? "" : 0);
}

DOMString HTMLOptionElement::label() const
{
    if(!impl) return DOMString();
    return static_cast<ElementImpl*>(impl)->getAttribute(ATTR_LABEL);
}

void HTMLOptionElement::setLabel( const DOMString &value )
{
    if(impl) static_cast<ElementImpl*>(impl)->setAttribute(ATTR_LABEL, value);
}

bool HTMLOptionElement::selected() const
{
    if(!impl) return 0;
    return ((HTMLOptionElementImpl *)impl)->selected();
}

void HTMLOptionElement::setSelected(bool _selected) {
    if(!impl) return;
    ((HTMLOptionElementImpl *)impl)->setSelected(_selected);
}

DOMString HTMLOptionElement::value() const
{
    if(!impl) return DOMString();
    return static_cast<HTMLOptionElementImpl*>(impl)->value();
}

void HTMLOptionElement::setValue( const DOMString &value )
{
    if(impl) static_cast<HTMLOptionElementImpl*>(impl)->setValue(value.implementation());
}

// -----------------------------------------------------------------------------

HTMLIsIndexElement::HTMLIsIndexElement() : HTMLElement()
{
}

HTMLIsIndexElement::HTMLIsIndexElement(const HTMLIsIndexElement &other) : HTMLElement(other)
{
}

HTMLIsIndexElement::HTMLIsIndexElement(HTMLIsIndexElementImpl *impl) : HTMLElement(impl)
{
}

HTMLIsIndexElement &HTMLIsIndexElement::operator = (const Node &other)
{
    assignOther( other, ID_ISINDEX );
    return *this;
}

HTMLIsIndexElement &HTMLIsIndexElement::operator = (const HTMLIsIndexElement &other)
{
    HTMLElement::operator = (other);
    return *this;
}

HTMLIsIndexElement::~HTMLIsIndexElement()
{
}

HTMLFormElement HTMLIsIndexElement::form() const
{
    return Element::form();
}

DOMString HTMLIsIndexElement::prompt() const
{
    if(!impl) return DOMString();
    return static_cast<HTMLIsIndexElementImpl*>(impl)->prompt();
}

void HTMLIsIndexElement::setPrompt( const DOMString &value )
{
    if(impl) static_cast<HTMLIsIndexElementImpl*>(impl)->setPrompt(value);
}
