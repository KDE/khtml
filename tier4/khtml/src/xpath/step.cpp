/*
 * step.cc - Copyright 2005 Frerich Raabe   <raabe@kde.org>
 *           Copyright 2010 Maksim Orlovich <maksim@kde.org>
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
#include "step.h"

#include "dom/dom3_xpath.h"
#include "xml/dom_docimpl.h"
#include "xml/dom_nodeimpl.h"
#include "xml/dom_textimpl.h"
#include "xml/dom3_xpathimpl.h"

#include "step.h"

#include <QtDebug>

using namespace DOM;
using namespace DOM::XPath;
using namespace khtml;
using namespace khtml::XPath;


static bool areAdjacentTextNodes( NodeImpl *n, NodeImpl *m )
{
	if ( !n || !m ) {
		return false;
	}

	if ( n->nodeType() != Node::TEXT_NODE && n->nodeType() != Node::CDATA_SECTION_NODE ) {
		return false;
	}
	if ( m->nodeType() != Node::TEXT_NODE && m->nodeType() != Node::CDATA_SECTION_NODE ) {
		return false;
	}

	// ###
#ifdef __GNUC__
#warning "Might need more generic adjacency -- check"
#endif

	return ( n->nextSibling() && ( n->nextSibling() == m ) ) ||
	       ( m->nextSibling() && ( m->nextSibling() == n ) );
}

static DomNodeList compressTextNodes( const DomNodeList &nodes )
{
	DomNodeList outNodes = new StaticNodeListImpl;

	for ( unsigned long n = 0; n < nodes->length(); ++n) {
		NodeImpl* node = nodes->item( n );
		NodeImpl* next = n+1 < nodes->length() ? nodes->item( n+1 ) : 0;
		
		if ( !next || !areAdjacentTextNodes( node, next ) ) {
			outNodes->append( node );
		} else if ( areAdjacentTextNodes( node, next ) ) {
			QString s = node->nodeValue().string();

			// n2 is a potential successor, and is always in-range
			unsigned long n2 = n+1;
			while (n2 < nodes->length() && areAdjacentTextNodes( nodes->item( n2 ), nodes->item( n2-1) ) ) {
				s += nodes->item( n2 )->nodeValue().string();
				++n2;
			}
			outNodes->append( node->document()->createTextNode( new DOMStringImpl( s.data(), s.length() ) ) );
		}
	}
	return outNodes;
}

QString Step::axisAsString( AxisType axis )
{
	switch ( axis ) {
		case AncestorAxis: return "ancestor";
		case AncestorOrSelfAxis: return "ancestor-or-self";
		case AttributeAxis: return "attribute";
		case ChildAxis: return "child";
		case DescendantAxis: return "descendant";
		case DescendantOrSelfAxis: return "descendant-or-self";
		case FollowingAxis: return "following";
		case FollowingSiblingAxis: return "following-sibling";
		case NamespaceAxis: return "namespace";
		case ParentAxis: return "parent";
		case PrecedingAxis: return "preceding";
		case PrecedingSiblingAxis: return "preceding-sibling";
		case SelfAxis: return "self";
	}
	return QString();
}

Step::Step(): m_compileState( NotCompiled )
{
}

Step::Step( AxisType axis, const DOMString &nodeTest,
            const QList<Predicate *> &predicates )
	: m_axis( axis ),
	m_nodeTest( nodeTest ),
	m_compileState( NotCompiled ),	
	m_predicates( predicates )
{
}

Step::~Step()
{
	qDeleteAll( m_predicates );
}

DomNodeList Step::evaluate( NodeImpl *context ) const
{
	assert( context );
#ifdef XPATH_VERBOSE
	qDebug() << "Evaluating step, axis='" << axisAsString( m_axis ) <<"'"
	             << ", nodetest='" << m_nodeTest << "'"
	             << ", " << m_predicates.count() << " predicates";
	qDebug() << DOM::getPrintableName( context->id() );
#endif

	DomNodeList inNodes = nodesInAxis( context ), outNodes;

	// ### optimization opportunity: can say DocumentOrder for most
	StaticNodeListImpl::NormalizationKind known = StaticNodeListImpl::AxisOrder;
	inNodes->setKnownNormalization(known);

#ifdef XPATH_VERBOSE
	qDebug() << "Axis " << axisAsString( m_axis ) << " matches " << inNodes->length() << " nodes.";
#endif
	
	inNodes = nodeTestMatches( context, inNodes );
	inNodes->setKnownNormalization(known); // nodeTest doesn't change order

#ifdef XPATH_VERBOSE
	qDebug() << "\tNodetest " << m_nodeTest << " trims this number to " << inNodes->length();
#endif

	if ( m_predicates.isEmpty() )
		return inNodes;

	foreach( Predicate *predicate, m_predicates ) {
		outNodes = new StaticNodeListImpl;
		Expression::evaluationContext().size = int(inNodes->length());
		Expression::evaluationContext().position = 1;

		for ( unsigned long n = 0; n < inNodes->length(); ++n ) {
			NodeImpl* node = inNodes->item( n );
			Expression::evaluationContext().node = node;
			EvaluationContext backupCtx = Expression::evaluationContext();
#ifdef XPATH_VERBOSE			
			qDebug() << Expression::evaluationContext().position << "/" << node;
#endif
			if ( predicate->evaluate() ) {
				outNodes->append( node );
			}
			Expression::evaluationContext() = backupCtx;
			++Expression::evaluationContext().position;
		}
#ifdef XPATH_VERBOSE
		qDebug() << "\tPredicate trims this number to " << outNodes->length();
#endif
		inNodes = outNodes;
		inNodes->setKnownNormalization(known); // predicates don't change order
	}

	return outNodes;
}

DomNodeList Step::nodesInAxis( NodeImpl *context ) const
{
	//assert(context);
	DomNodeList nodes = new StaticNodeListImpl;
	switch ( m_axis ) {
		case ChildAxis: {
			NodeImpl *n = xpathFirstChild( context );
			while ( n ) {
				nodes->append( n );
				n = n->nextSibling();
			}
			return nodes;
		}
		case DescendantAxis: {
			collectChildrenRecursively( nodes, context );
			return nodes;
		}
		case ParentAxis: {
			NodeImpl* p = xpathParentNode( context );

			if ( p )
				nodes->append( p );
			return nodes;
		}
		case AncestorAxis: {
			NodeImpl *n = xpathParentNode( context );
			while ( n ) {
				nodes->append( n );
				n = xpathParentNode( n );
			}
			return nodes;
		}
		case FollowingSiblingAxis: {
			if ( context->nodeType() == Node::ATTRIBUTE_NODE ||
			     context->nodeType() == Node::XPATH_NAMESPACE_NODE ) {
				return nodes; // empty
			}

			NodeImpl *n = context->nextSibling();
			while ( n ) {
				nodes->append( n );
				n = n->nextSibling();
			}
			return nodes;
		}
		case PrecedingSiblingAxis: {
			if ( context->nodeType() == Node::ATTRIBUTE_NODE ||
			     context->nodeType() == Node::XPATH_NAMESPACE_NODE ) {
				return nodes; // empty
			}

			NodeImpl *n = context->previousSibling();
			while ( n ) {
				nodes->append( n );
				n = n->previousSibling();
			}
			return nodes;
		}
		case FollowingAxis: {
			NodeImpl *p = context;
			while ( !isRootDomNode( p ) ) {
				NodeImpl *n = nextSiblingForFollowing( p );
				while ( n ) {
					nodes->append( n );
					collectChildrenRecursively( nodes, n );
					n = n->nextSibling();
				}
				p = xpathParentNode( p );
			}
			return nodes;
		}
		case PrecedingAxis: {
			NodeImpl *p = context;
			while ( !isRootDomNode( p ) ) {
				NodeImpl *n = p->previousSibling();
				while ( n ) {
					collectChildrenReverse( nodes, n );				
					nodes->append( n );
					n = n->previousSibling();
				}
				p = xpathParentNode( p );
			}
			return nodes;
		}
		case AttributeAxis: {
			if ( context->nodeType() != Node::ELEMENT_NODE ) {
				return nodes; // empty
			}

			NamedAttrMapImpl *attrs = static_cast<ElementImpl*>(context)->attributes( true /*read-only*/ );
			if ( !attrs ) {
				return nodes;
			}

			for ( unsigned long i = 0; i < attrs->length(); ++i ) {
				nodes->append( attrs->item( i ) );
			}
			return nodes;
		}
		case NamespaceAxis: {
			// ### TODO: Need to implement this, but not a priority, since
			// other impls don't.
#if 0
			if ( context->nodeType() != Node::ELEMENT_NODE ) {
				return nodes;
			}

			bool foundXmlNsNode = false;
			NodeImpl *node = context;
			while ( node ) {
				NamedAttrMapImpl *attrs =
						node->isElementNode() ? static_cast<ElementImpl*>(node)->attributes() : 0;
				if ( !attrs ) {
					node = xpathParentNode( node );
					continue;
				}

				for ( unsigned long i = 0; i < attrs->length(); ++i ) {
					NodeImpl *n = attrs->item( i );
					if ( n->nodeName().string().startsWith( "xmlns:" ) ) {
						nodes->append( n );
					} else if ( n->nodeName() == "xmlns" &&
					            !foundXmlNsNode
					            ) {
						foundXmlNsNode = true;
						if ( !n->nodeValue().string().isEmpty() ) {
							nodes->append( n );
						}
					}
				}
				node = xpathParentNode( node );
			}
#endif
			return nodes;
		}
		case SelfAxis:
			nodes->append( context );
			return nodes;
		case DescendantOrSelfAxis:
			nodes->append( context );
			collectChildrenRecursively( nodes, context );
			return nodes;
		case AncestorOrSelfAxis: {
			nodes->append( context );
			NodeImpl *n = xpathParentNode( context );
			while ( n ) {
				nodes->append( n );
				n = xpathParentNode( n );
			}
			return nodes;
		}
		default:
			qWarning() << "Unknown axis " << axisAsString( m_axis ) << " passed to Step::nodesInAxis";
	}

	return nodes; // empty
}

void Step::compileNodeTest(bool htmlCompat) const
{
	m_compileState = htmlCompat ? CompiledForHTML : CompiledForXML;

	if ( m_nodeTest == "*" ) {
		m_nodeTestType = NT_Star;
	} else if ( m_nodeTest == "text()" ) {
		m_nodeTestType = NT_Text;
	} else if ( m_nodeTest == "comment()" ) {
		m_nodeTestType = NT_Comment;
	} else if ( m_nodeTest.startsWith( "processing-instruction" ) ) {
		DOMString param;

		// ### is this right? (parens?)
		const int space = m_nodeTest.find( ' ' );
		if ( space > -1 ) {
			param = m_nodeTest.substring( space + 1 );
		}

		if ( param.isEmpty() ) {
			m_nodeTestType = NT_PI;
		} else {
			m_nodeTestType = NT_PI_Lit;
			m_piInfo = param;
		}
	} else if ( m_nodeTest == "node()" ) {
		m_nodeTestType = NT_AnyNode;
	} else {
		// Some sort of name combo.
		PrefixName prefix;
		LocalName localName;

		splitPrefixLocalName( m_nodeTest,  prefix, localName, htmlCompat );

		if ( prefix.id() == DOM::emptyPrefix ) {
			// localname only
			m_nodeTestType = NT_LocalName;
			m_localName    = localName;
		} else if ( localName.toString() == "*" ) {
			// namespace only
			m_nodeTestType = NT_Namespace;
			m_namespace    = NamespaceName::fromString(namespaceFromNodetest( m_nodeTest ) );
		} else {
			// Both parts.
			m_nodeTestType = NT_QName;
			m_localName    = localName;
			m_namespace    = NamespaceName::fromString(namespaceFromNodetest( m_nodeTest ) );
		}
	}
}

DomNodeList Step::nodeTestMatches( NodeImpl* ctx, const DomNodeList &nodes ) const
{
	CompileState desired = ctx->htmlCompat() ? CompiledForHTML : CompiledForXML;
	if ( m_compileState != desired )
		compileNodeTest( ctx->htmlCompat() );

	if ( m_nodeTestType == NT_AnyNode) // no need for a new set
		return nodes;

	DomNodeList matches = new StaticNodeListImpl;

	// A lot of things can be handled by just checking the type,
	// and maybe a hair more special handling
	int matchType;
	switch ( m_nodeTestType ) {
	case NT_Star:
		matchType = primaryNodeType( m_axis );
		break;
	case NT_Comment:
		matchType = Node::COMMENT_NODE;
		break;
	case NT_Text:
		matchType = Node::TEXT_NODE;
		break;
	case NT_PI:
	case NT_PI_Lit:	
		matchType = Node::PROCESSING_INSTRUCTION_NODE;
		break;
	default:
		matchType = -1;
	}

	if ( matchType != -1 ) {
		for ( unsigned long n = 0; n < nodes->length(); ++n ) {
			NodeImpl *node = nodes->item( n );
			int nodeType   = node->nodeType();
			if ( nodeType == matchType ) {
				if ( m_nodeTestType == NT_PI_Lit && node->nodeName() != m_piInfo )
					continue;
					
				matches->append( node );
			}

			if ( matchType == NT_Text && nodeType == Node::CDATA_SECTION_NODE )
				matches->append( node );
		}
		return matches;
	}

	// Now we are checking by name. We always want to filter out
	// the primary axis here
	// ### CHECK: this used to have special handling for some axes.
	matchType = primaryNodeType( m_axis );
	for ( unsigned long n = 0; n < nodes->length(); ++n ) {
		NodeImpl *node = nodes->item( n );
		if ( node->nodeType() != matchType )
			continue;

		if ( m_nodeTestType == NT_LocalName ) {
			if ( localNamePart( node->id() ) == m_localName.id() )
				matches->append( node );
		} else if ( m_nodeTestType == NT_Namespace ) {
			if ( namespacePart( node->id() ) == m_namespace.id() )
				matches->append( node );
		} else {
			// NT_QName
			if ( node->id() == makeId( m_namespace.id(), m_localName.id() ) )
				matches->append( node );
		}
	}
	return matches;
}

void Step::optimize()
{
	foreach( Predicate *predicate, m_predicates ) {
		predicate->optimize();
	}
}

QString Step::dump() const
{
	QString s = QString( "<step axis=\"%1\" nodetest=\"%2\">" ).arg( axisAsString( m_axis ) ).arg( m_nodeTest.string() );
	foreach( Predicate *predicate, m_predicates ) {
		s += predicate->dump();
	}
	s += "</step>";
	return s;
}

DOMString Step::namespaceFromNodetest( const DOMString &nodeTest ) const
{
	int i = nodeTest.find( ':' );
	if ( i == -1 ) {
		return DOMString();
	}

	DOMString prefix( nodeTest.substring( 0, i ) );
	XPathNSResolverImpl *resolver = Expression::evaluationContext().resolver;

	DOM::DOMString ns;
	if (resolver)
		ns = resolver->lookupNamespaceURI( prefix );

	if ( ns.isNull() )
		Expression::reportNamespaceErr();
	
	return ns;
}

unsigned int Step::primaryNodeType( AxisType axis ) const
{
	switch ( axis ) {
		case AttributeAxis:
			return Node::ATTRIBUTE_NODE;
		case NamespaceAxis:
			return Node::XPATH_NAMESPACE_NODE;
		default:
			return Node::ELEMENT_NODE;
	}
}

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
