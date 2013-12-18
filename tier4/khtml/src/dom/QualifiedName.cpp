/*
 * This file is part of the DOM implementation for KDE.
 *
 * Copyright (C) 2005, 2006 Apple Computer, Inc.
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

#include "QualifiedName.h"
#include "xml/dom_nodeimpl.h"

namespace DOM {

QualifiedName::QualifiedName(const DOMString& prefix, const DOMString& localName, const DOMString& namespaceURI)
{
    m_prefix = PrefixName::fromString(prefix);
    m_localName = LocalName::fromString(localName);
    m_namespace = NamespaceName::fromString(namespaceURI);
}

QualifiedName::QualifiedName(const QualifiedName& name)
{
    m_prefix = name.prefixId();
    m_namespace = name.namespaceNameId();
    m_localName = name.localNameId();
}

QualifiedName::QualifiedName(int prefix, int localName, int namespaceName)
{
    m_prefix = PrefixName::fromId(prefix);
    m_localName = LocalName::fromId(localName);
    m_namespace = NamespaceName::fromId(namespaceName);
}

QualifiedName::QualifiedName(quint32 id, PrefixName prefix) {
    m_prefix = prefix;
    m_localName = LocalName::fromId(localNamePart(id));
    m_namespace = NamespaceName::fromId(namespacePart(id));
}

const QualifiedName& QualifiedName::operator=(const QualifiedName& name)
{
    m_prefix = name.prefixId();
    m_namespace = name.namespaceNameId();
    m_localName = name.localNameId();
    return *this;
}

bool QualifiedName::operator==(const QualifiedName& other) const
{
    /*// qDebug() << m_prefix.id() << other.prefixId().id() << ((m_prefix == other.prefixId())) << endl;
    // qDebug() << (m_prefix == other.prefixId()) << (m_localName == other.localNameId()) << (m_namespace == other.namespaceNameId()) << endl;*/
    return (m_prefix == other.prefixId() && m_localName == other.localNameId() && m_namespace == other.namespaceNameId());
}

bool QualifiedName::hasPrefix() const
{
    return m_prefix.id() != 0/*emptyPrefix*/; 
}

bool QualifiedName::matches(const QualifiedName& other) const
{
    //FIXME: IMPLEMENT
    return *this == other || (m_localName == other.localNameId() && (m_prefix == other.prefixId() || m_namespace == other.namespaceNameId()));
}

void QualifiedName::setPrefix(const PrefixName& prefix) {
    m_prefix = prefix;
}

void QualifiedName::setPrefix(const DOMString& prefix) {
    m_prefix = PrefixName::fromString(prefix);
}

unsigned QualifiedName::id() const
{
    return (m_namespace.id() << 16) ^ (m_localName.id());
}

DOMString QualifiedName::tagName() const
{
    DOMString prefix = m_prefix.toString();
    DOMString localName = m_localName.toString();
    if (prefix.isEmpty())
        return localName;
    return prefix + DOMString(":") + localName;
}

DOMString QualifiedName::prefix() const
{
    return m_prefix.toString();
}

DOMString QualifiedName::localName() const
{
    return m_localName.toString();
}

DOMString QualifiedName::namespaceURI() const
{
    return m_namespace.toString();
}

DOMString QualifiedName::toString() const
{
    return prefix() + DOMString(":") + localName();
}

}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
