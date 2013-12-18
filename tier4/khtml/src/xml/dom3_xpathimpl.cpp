/*
 * dom3_xpathimpl.cpp - Copyright 2005 Frerich Raabe <raabe@kde.org>
 *                      Copyright 2010 Maksim Orlovich <maksim@kde.org>
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
#include <dom/dom_exception.h>
#include <dom/dom3_xpath.h>
#include <xml/dom3_xpathimpl.h>
#include <xml/dom_nodeimpl.h>
#include <xml/dom_nodelistimpl.h>

#include "xpath/expression.h"
#include "xpath/util.h"

using namespace DOM;
using namespace khtml;
using namespace khtml::XPath;
using namespace DOM::XPath;

XPathResultImpl::XPathResultImpl()
{
}

XPathResultImpl::XPathResultImpl( const Value &value )
	: m_value( value )
{
	switch ( m_value.type() ) {
		case Value::Boolean:
			m_resultType = BOOLEAN_TYPE;
			break;
		case Value::Number:
			m_resultType = NUMBER_TYPE;
			break;
		case Value::String:
			m_resultType = STRING_TYPE;
			break;
		case Value::Nodeset:
			m_resultType = UNORDERED_NODE_ITERATOR_TYPE;
			m_nodeIterator = 0;
	}
}

void XPathResultImpl::convertTo( unsigned short type, int &exceptioncode )
{
	switch ( type ) {
		case ANY_TYPE:
			break;
		case NUMBER_TYPE:
			m_resultType = type;
			m_value = Value( m_value.toNumber() );
			break;
		case STRING_TYPE:
			m_resultType = type;
			m_value = Value( m_value.toString() );
			break;
		case BOOLEAN_TYPE:
			m_resultType = type;
			m_value = Value( m_value.toBoolean() );
			break;
		case UNORDERED_NODE_ITERATOR_TYPE:
		case ORDERED_NODE_ITERATOR_TYPE:
		case UNORDERED_NODE_SNAPSHOT_TYPE:
		case ORDERED_NODE_SNAPSHOT_TYPE:
		case ANY_UNORDERED_NODE_TYPE:
		case FIRST_ORDERED_NODE_TYPE:
			if ( !m_value.isNodeset() ) {
				exceptioncode = XPathException::toCode(XPathException::TYPE_ERR);
				return;
			}
			m_resultType = type;
			break;
		default:
			// qDebug() << "Cannot convert XPathResultImpl to unknown type" << type;
			exceptioncode = XPathException::toCode(XPathException::TYPE_ERR);
	}
}

unsigned short XPathResultImpl::resultType() const
{
	return m_resultType;
}

double XPathResultImpl::numberValue(int &exceptioncode) const
{
	if ( resultType() != NUMBER_TYPE ) {
		exceptioncode = XPathException::toCode(XPathException::TYPE_ERR);
		return 0.0;
	}
	return m_value.toNumber();
}

DOM::DOMString XPathResultImpl::stringValue(int &exceptioncode) const
{
	if ( resultType() != STRING_TYPE ) {
		exceptioncode = XPathException::toCode(XPathException::TYPE_ERR);
		return DOMString();
	}
	return m_value.toString();
}

bool XPathResultImpl::booleanValue(int &exceptioncode) const
{
	if ( resultType() != BOOLEAN_TYPE ) {
		exceptioncode = XPathException::toCode(XPathException::TYPE_ERR);
		return false;
	}
	return m_value.toBoolean();
}

NodeImpl *XPathResultImpl::singleNodeValue(int &exceptioncode) const
{
	if ( resultType() != ANY_UNORDERED_NODE_TYPE &&
	     resultType() != FIRST_ORDERED_NODE_TYPE ) {
		exceptioncode = XPathException::toCode(XPathException::TYPE_ERR);
		return 0;
	}
	DomNodeList nodes = m_value.toNodeset();

	if (nodes && nodes->length())
		return nodes->item(0);
	else
		return 0;
}

bool XPathResultImpl::invalidIteratorState() const
{
	if ( resultType() != UNORDERED_NODE_ITERATOR_TYPE &&
	     resultType() != ORDERED_NODE_ITERATOR_TYPE ) {
		return false;
	}
	// XXX How to tell whether the document was changed since this
	// result was returned?
	return true;
}

unsigned long XPathResultImpl::snapshotLength(int &exceptioncode) const
{
	if ( resultType() != UNORDERED_NODE_SNAPSHOT_TYPE &&
	     resultType() != ORDERED_NODE_SNAPSHOT_TYPE ) {
		exceptioncode = XPathException::toCode(XPathException::TYPE_ERR);
		return 0;
	}

	SharedPtr<DOM::StaticNodeListImpl> nodes = m_value.toNodeset();
	return nodes ? nodes->length() : 0;
}

NodeImpl *XPathResultImpl::iterateNext(int &exceptioncode)
{
	if ( resultType() != UNORDERED_NODE_ITERATOR_TYPE &&
	     resultType() != ORDERED_NODE_ITERATOR_TYPE ) {
		exceptioncode = XPathException::toCode(XPathException::TYPE_ERR);
		return 0;
	}
	// XXX How to tell whether the document was changed since this
	// result was returned? We need to throw an INVALID_STATE_ERR if that
	// is the case.
	SharedPtr<DOM::StaticNodeListImpl> nodes = m_value.toNodeset();
	if ( !nodes || m_nodeIterator >= nodes->length() ) {
		return 0;
	} else {
		NodeImpl* n = nodes->item(m_nodeIterator);
		++m_nodeIterator;
		return n;
	}
}

NodeImpl *XPathResultImpl::snapshotItem( unsigned long index, int &exceptioncode )
{
	if ( resultType() != UNORDERED_NODE_SNAPSHOT_TYPE &&
	     resultType() != ORDERED_NODE_SNAPSHOT_TYPE ) {
		exceptioncode = XPathException::toCode(XPathException::TYPE_ERR);
		return 0;
	}
	DomNodeList nodes = m_value.toNodeset();
	if ( !nodes || index >= nodes->length() ) {
		return 0;
	}
	return nodes->item( index );
}

// ---------------------------------------------------------------------------

DefaultXPathNSResolverImpl::DefaultXPathNSResolverImpl( NodeImpl *node )
	: m_node( node )
{
}

DOMString DefaultXPathNSResolverImpl::lookupNamespaceURI( const DOMString& prefix )
{
	// Apparently Node::lookupNamespaceURI doesn't do this.
	// ### check
	if ( prefix.string() == "xml" ) {
		return DOMString( "http://www.w3.org/XML/1998/namespace" );
	}
	return m_node->lookupNamespaceURI( prefix );
}

// ---------------------------------------------------------------------------
XPathExpressionImpl::XPathExpressionImpl( const DOMString& expression, XPathNSResolverImpl *resolver ): m_statement(expression, resolver)
{
}

XPathResultImpl *XPathExpressionImpl::evaluate( NodeImpl *contextNode,
                                                unsigned short type,
                                                XPathResultImpl* /*result_*/,
                                                int &exceptioncode )
{
	if ( !isValidContextNode( contextNode ) ) {
		exceptioncode = DOMException::NOT_SUPPORTED_ERR;
		return 0;
	}

	// We are permitted, but not required, to re-use result_. We don't.
	Value xpathRes = m_statement.evaluate( contextNode, exceptioncode );
	XPathResultImpl* result = new XPathResultImpl( exceptioncode ? Value() : xpathRes );
	
	if ( type != ANY_TYPE ) {
		result->convertTo( type, exceptioncode );
		if( exceptioncode ) {
			// qDebug() << "couldn't convert XPathResult to" <<  type << "from" << xpathRes.type();
			delete result;
			return 0;
		}
	}

	return result;
}

int XPathExpressionImpl::parseExceptionCode()
{
	return m_statement.exceptionCode();
}

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
