/*
 * util.cc - Copyright 2005 Frerich Raabe <raabe@kde.org>
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
#include "util.h"
#include "xml/dom_nodeimpl.h"
#include "xml/dom_elementimpl.h"
#include "xml/dom_nodelistimpl.h" 

using namespace DOM;

namespace khtml {
namespace XPath {

bool isRootDomNode( NodeImpl *node )
{
	return node && !xpathParentNode(node);
}

static QString stringValueImpl( NodeImpl *node )
{
	// ### how different is this from textContent?
	// ### "The string-value of a namespace node is the namespace URI that is being bound to the namespace prefix; if it is relative, it must be resolved just like a namespace URI in an expanded-name."
	switch ( node->nodeType() ) {
		case Node::ATTRIBUTE_NODE:
		case Node::PROCESSING_INSTRUCTION_NODE:
		case Node::COMMENT_NODE:
		case Node::TEXT_NODE:
		case Node::CDATA_SECTION_NODE:
			return node->nodeValue().string();
		default:
			if ( isRootDomNode( node )
					|| node->nodeType() == Node::ELEMENT_NODE ) {
				QString str;

				for ( NodeImpl *cur = node->firstChild(); cur; cur = cur->traverseNextNode(node) ) {
					// We only include the value of text kids here.
					int type = cur->nodeType();
					if (type == Node::TEXT_NODE || type == Node::CDATA_SECTION_NODE)
						str.append( stringValueImpl( cur ) );
				}
				return str;
			}
	}
	return QString();
}

DOMString stringValue( NodeImpl *node )
{
	return stringValueImpl( node );
}

void collectChildrenRecursively( SharedPtr<DOM::StaticNodeListImpl> out,
                                 DOM::NodeImpl *root )
{
	// ### probably beter to use traverseNext and the like
	
	NodeImpl *n = xpathFirstChild( root );
	while ( n ) {
		out->append( n );
		collectChildrenRecursively( out, n );
		n = n->nextSibling();
	}
}

void collectChildrenReverse( SharedPtr<DOM::StaticNodeListImpl> out,
                             DOM::NodeImpl *root )
{
	// ### probably beter to use traverseNext and the like

	NodeImpl *n = xpathLastChild( root );
	while ( n ) {
		collectChildrenReverse( out, n );
		out->append( n );
		n = n->previousSibling();
	}
}

bool isValidContextNode( NodeImpl *node )
{
	return node && ( 
	       node->nodeType() == Node::ELEMENT_NODE ||
	       node->nodeType() == Node::ATTRIBUTE_NODE ||
	       node->nodeType() == Node::TEXT_NODE ||
	       node->nodeType() == Node::CDATA_SECTION_NODE ||
	       node->nodeType() == Node::PROCESSING_INSTRUCTION_NODE ||
	       node->nodeType() == Node::COMMENT_NODE ||
	       node->nodeType() == Node::DOCUMENT_NODE ||
	       node->nodeType() == Node::XPATH_NAMESPACE_NODE );
}

DOM::NodeImpl *xpathParentNode( DOM::NodeImpl *node )
{
	DOM::NodeImpl *res = 0;
	if ( node ) {
	    if ( node->nodeType() == Node::ATTRIBUTE_NODE )
	        res = static_cast<DOM::AttrImpl*>(node)->ownerElement();
	else
	        res = node->parentNode();
	}
	return res;
}

DOM::NodeImpl *xpathFirstChild( DOM::NodeImpl *node )
{
	DOM::NodeImpl *res = 0;
	if ( node && node->nodeType() != Node::ATTRIBUTE_NODE )
	    res = node->firstChild();
	return res;
}

DOM::NodeImpl *xpathLastChild( DOM::NodeImpl *node )
{
	DOM::NodeImpl *res = 0;
	if ( node && node->nodeType() != Node::ATTRIBUTE_NODE )
	    res = node->lastChild();
	return res;
}

DOM::NodeImpl *nextSiblingForFollowing( DOM::NodeImpl *node )
{
	DOM::NodeImpl *res = 0;
	if ( node ) {
		if ( node->nodeType() == Node::ATTRIBUTE_NODE )
			res = static_cast<DOM::AttrImpl*>(node)->ownerElement()->firstChild();
	else
			res = node->nextSibling();
	}
	return res;
}

} // namespace khtml
} // namespace XPath

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
