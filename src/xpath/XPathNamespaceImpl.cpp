/*
 * XPathNamespaceImpl.cpp - Copyright 2005 Frerich Raabe <raabe@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "XPathNamespaceImpl.h"
#include "DocumentImpl.h"

#include "kdomxpath.h"

using namespace DOM;

XPathNamespaceImpl::XPathNamespaceImpl( DocumentImpl *ptr, DOMStringImpl *prefix, DOMStringImpl *uri )
	: NodeImpl( ptr ),
	m_ownerDocument( ptr ),
	m_prefix( prefix ),
	m_uri( uri )
{
}

XPathNamespaceImpl::XPathNamespaceImpl( const XPathNamespaceImpl &other )
	: NodeImpl( other )
{
	NodeImpl::operator=( other );
	*this = other;
}

XPathNamespaceImpl::~XPathNamespaceImpl()
{
}

XPathNamespaceImpl &XPathNamespaceImpl::operator=( const XPathNamespaceImpl &other )
{
	m_prefix = other.m_prefix;
	m_uri = other.m_uri;
	return *this;
}

DocumentImpl *XPathNamespaceImpl::ownerDocument() const
{
	return m_ownerDocument;
}

ElementImpl *XPathNamespaceImpl::ownerElement() const
{
	// XXX
	return 0;
}

const AtomicString &XPathNamespaceImpl::prefix() const
{
    return m_prefix;
}

DOMString XPathNamespaceImpl::nodeName() const
{
    return DOMString(m_prefix);
}

const AtomicString &XPathNamespaceImpl::namespaceURI() const
{
    return m_uri;
}

unsigned short XPathNamespaceImpl::nodeType() const
{
	return XPATH_NAMESPACE_NODE;
}

