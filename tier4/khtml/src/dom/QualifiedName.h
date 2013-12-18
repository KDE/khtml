/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2005 Apple Computer, Inc.
 * Copyright (C) 2008 Vyacheslav Tokarev (tsjoker@gmail.com)
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
#ifndef _DOM_QUALIFIEDNAME_h_
#define _DOM_QUALIFIEDNAME_h_

#include "misc/idstring.h"
#include "misc/htmlnames.h"

namespace DOM {

/*class DOMString;
class PrefixName;
class LocalName;
class NamespaceName;*/

class QualifiedName {
public:
    QualifiedName() {}
    QualifiedName(PrefixName prefix, LocalName localName, NamespaceName namespaceURI) : m_namespace(namespaceURI), m_prefix(prefix), m_localName(localName) {}
    QualifiedName(const DOMString& prefix, const DOMString& localName, const DOMString& namespaceURI);
    QualifiedName(int prefix, int localName, int namespaceName);
    //QualifiedName(DOMString namespaceURI, DOMString prefix, DOMString localName);
    QualifiedName(quint32 id, PrefixName prefix);
    ~QualifiedName() {}

    QualifiedName(const QualifiedName& name);
    const QualifiedName& operator=(const QualifiedName& name);

    bool operator==(const QualifiedName& other) const;
    //inline bool operator!=(const QualifiedName& other) const { return (m_prefix != other.prefixId() || m_localName != other.localNameId() || m_namespace != other.namespaceNameId()); }

    bool matches(const QualifiedName& other) const;

    inline bool hasPrefix() const;
    void setPrefix(const PrefixName& prefix);
    void setPrefix(const DOMString& prefix);

    inline PrefixName prefixId() const { return m_prefix; }
    inline LocalName localNameId() const { return m_localName; }
    inline NamespaceName namespaceNameId() const { return m_namespace; }
    unsigned id() const;

    DOMString tagName() const;

    DOMString prefix() const;
    DOMString localName() const;
    DOMString namespaceURI() const;

    DOMString toString() const;

private:
    NamespaceName m_namespace;
    PrefixName m_prefix;
    LocalName m_localName;
};

}

#endif
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
