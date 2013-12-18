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
 */

#include "dom/dom_string.h"
#include "xml/dom_stringimpl.h"

#include <wtf/Vector.h>

using namespace DOM;


DOMString::DOMString(const QChar *str, uint len)
{
    if (!str) {
       impl = 0;
       return;
    }
    impl = new DOMStringImpl( str, len );
    impl->ref();
}

DOMString::DOMString(const QString &str)
{
    if (str.isNull()) {
	impl = 0;
	return;
    }

    impl = new DOMStringImpl( str.unicode(), str.length() );
    impl->ref();
}

DOMString::DOMString(const char *str)
{
    if (!str) {
	impl = 0;
	return;
    }

    impl = new DOMStringImpl( str );
    impl->ref();
}

DOMString::DOMString(const char *str, uint len)
{
    if (!str) {
        impl = 0;
        return;
    }
    impl = new DOMStringImpl(str, len);
    impl->ref();
}

DOMString::DOMString(DOMStringImpl *i)
{
    impl = i;
    if(impl) impl->ref();
}

DOMString::DOMString(const DOMString &other)
{
    impl = other.impl;
    if(impl) impl->ref();
}

DOMString::~DOMString()
{
    if(impl) impl->deref();
}

DOMString &DOMString::operator =(const DOMString &other)
{
    if ( impl != other.impl ) {
        if(impl) impl->deref();
        impl = other.impl;
        if(impl) impl->ref();
    }
    return *this;
}

DOMString &DOMString::operator += (const DOMString &str)
{
    if(!impl)
    {
	// ### FIXME!!!
	impl = str.impl;
	if (impl)
	     impl->ref();
	return *this;
    }
    if(str.impl)
    {
	DOMStringImpl *i = impl->copy();
	impl->deref();
	impl = i;
	impl->ref();
	impl->append(str.impl);
    }
    return *this;
}

DOMString DOMString::operator + (const DOMString &str)
{
    if(!impl) return str.copy();
    if(str.impl)
    {
	DOMString s = copy();
	s += str;
	return s;
    }

    return copy();
}

void DOMString::insert(DOMString str, uint pos)
{
    if(!impl)
    {
	impl = str.impl->copy();
	impl->ref();
    }
    else
	impl->insert(str.impl, pos);
}


const QChar &DOMString::operator [](unsigned int i) const
{
    static const QChar nullChar = 0;

    if(!impl || i >= impl->l ) return nullChar;

    return *(impl->s+i);
}

int DOMString::find(const QChar c, int start) const
{
    unsigned int l = start;
    if(!impl || l >= impl->l ) return -1;
    while( l < impl->l )
    {
	if( *(impl->s+l) == c ) return l;
	l++;
    }
    return -1;
}

int DOMString::reverseFind(const QChar c, int start) const
{
    unsigned int l = start;
    if (!impl || l < -impl->l) return -1;
    l += impl->l;
    while (1) {
        if (*(impl->s + l) == c) return l;
        l--;
        if (l == 0)
            return -1;
    }
    return -1;
}

DOMString DOMString::substring(unsigned pos, unsigned len) const
{
    return (impl) ? impl->substring(pos, len) : DOMString();
}

uint DOMString::length() const
{
    if(!impl) return 0;
    return impl->l;
}

void DOMString::truncate( unsigned int len )
{
    if(impl) impl->truncate(len);
}

void DOMString::remove(unsigned int pos, int len)
{
  if(impl) impl->remove(pos, len);
}

DOMString DOMString::split(unsigned int pos)
{
  if(!impl) return DOMString();
  return impl->split(pos);
}

DOMString DOMString::lower() const
{
  if(!impl) return DOMString();
  return impl->lower();
}

DOMString DOMString::upper() const
{
  if(!impl) return DOMString();
  return impl->upper();
}

bool DOMString::percentage(int &_percentage) const
{
    if(!impl || !impl->l) return false;

    if ( *(impl->s+impl->l-1) != QChar('%'))
       return false;

    _percentage = QString::fromRawData(impl->s, impl->l-1).toInt();
    return true;
}

QChar *DOMString::unicode() const
{
    if(!impl) return 0;
    return impl->unicode();
}

QString DOMString::string() const
{
    if(!impl) return QString();

    return impl->string();
}

int DOMString::toInt() const
{
    if(!impl) return 0;

    return impl->toInt();
}

int DOMString::toInt(bool* ok) const
{
    if (!impl) {
        *ok = false;
        return 0;
    }

    return impl->toInt(ok);
}

float DOMString::toFloat(bool* ok) const
{
    if (!impl) {
	if (ok)
	    *ok = false;
        return 0;
    }
    return impl->toFloat(ok);
}

DOMString DOMString::number(float f)
{
    return DOMString(QString::number(f));
}

DOMString DOMString::copy() const
{
    if(!impl) return DOMString();
    return impl->copy();
}

bool DOMString::endsWith(const DOMString& str) const 
{
    if (str.length() > length()) return false;
    return impl->endsWith(str.implementation());
}

bool DOMString::startsWith(const DOMString& str) const 
{
    if (str.length() > length()) return false;
    return impl->startsWith(str.implementation());
}

// ------------------------------------------------------------------------

bool DOM::strcasecmp( const DOMString &as, const DOMString &bs )
{
    return strcasecmp(as.implementation(), bs.implementation());
}

bool DOM::strcasecmp( const DOMString &as, const char* bs )
{
    const QChar *a = as.unicode();
    int l = as.length();
    if ( !bs ) return ( l != 0 );
    while ( l-- ) {
        if ( a->toLatin1() != *bs ) {
            char cc = ( ( *bs >= 'A' ) && ( *bs <= 'Z' ) ) ? ( ( *bs ) + 'a' - 'A' ) : ( *bs );
            if ( a->toLower().toLatin1() != cc ) return true;
        }
        a++, bs++;
    }
    return ( *bs != '\0' );
}

bool DOMString::isEmpty() const
{
    return (!impl || impl->l == 0);
}

DOMString DOMString::format(const char *format, ...)
{
    va_list args;
    va_start(args, format);

    Vector<char, 256> buffer;

    // Do the format once to get the length.
#if COMPILER(MSVC)
    int result = _vscprintf(format, args);
#else
    char ch;
    int result = qvsnprintf(&ch, 1, format, args);
    // We need to call va_end() and then va_start() again here, as the
    // contents of args is undefined after the call to vsnprintf
    // according to http://man.cx/snprintf(3)
    //
    // Not calling va_end/va_start here happens to work on lots of
    // systems, but fails e.g. on 64bit Linux.
    va_end(args);
    va_start(args, format);
#endif

    if (result == 0) {
        va_end(args);
        return DOMString("");
    }
    if (result < 0) {
        va_end(args);
        return DOMString();
    }
    unsigned len = result;
    buffer.grow(len + 1);
    
    // Now do the formatting again, guaranteed to fit.
    qvsnprintf(buffer.data(), buffer.size(), format, args);

    va_end(args);

    buffer[len] = 0; // we don't really need this I guess
    return new DOMStringImpl(buffer.data()/*, len*/);
}

//-----------------------------------------------------------------------------

bool DOM::operator==( const DOMString &a, const DOMString &b )
{
    return !strcmp(a.implementation(), b.implementation());
}

bool DOM::operator==( const DOMString &a, const QString &b )
{
    int l = a.length();

    if( l != b.length() ) return false;

    if(!memcmp(a.unicode(), b.unicode(), l*sizeof(QChar)))
	return true;
    return false;
}

bool DOM::operator==( const DOMString &a, const char *b )
{
    DOMStringImpl* aimpl = a.impl;
    if ( !b ) return !aimpl;

    if ( aimpl ) {
        int alen = aimpl->l;
        const QChar *aptr = aimpl->s;
        while ( alen-- ) {
            unsigned char c = *b++;
            if ( !c || ( *aptr++ ).unicode() != c )
                return false;
        }
    }

    return !*b;    
}
