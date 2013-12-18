/*
 * functions.cc - Copyright 2005 Frerich Raabe <raabe@kde.org>
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
#include "functions.h"

#include "xml/dom_docimpl.h"
#include "xml/dom_nodeimpl.h"
#include "xml/dom_nodelistimpl.h"
#include "xml/dom_elementimpl.h"
#include "kjs/operations.h"

#include <QtDebug>

#include <math.h>

using namespace DOM;

namespace khtml {
namespace XPath {

#define DEFINE_FUNCTION_CREATOR(Class) \
static Function *create##Class() { return new Class; }

class Interval
{
	public:
		static const int Inf =-1;

		Interval();
		Interval( int value );
		Interval( int min, int max );

		bool contains( int value ) const;

		QString asString() const;

	private:
		int m_min;
		int m_max;
};

class FunLast : public Function
{
	public:
		virtual bool isConstant() const;

	private:
		virtual Value doEvaluate() const;
};

class FunPosition : public Function
{
	public:
		virtual bool isConstant() const;

	private:
		virtual Value doEvaluate() const;
};

class FunCount : public Function
{
	public:
		virtual bool isConstant() const;

	private:
		virtual Value doEvaluate() const;
};

// Base for various node-property functions, that have
// the same node picking logic. It passes the proper node,
// if any, or otherwise returns an empty string by itself
class NodeFunction : public Function
{
	private:
		virtual Value doEvaluate() const;
		virtual Value evaluateOnNode( DOM::NodeImpl* node ) const = 0;
};

class FunLocalName : public NodeFunction
{
	public:
		virtual bool isConstant() const;

	private:
		virtual Value evaluateOnNode( DOM::NodeImpl* node ) const;
};

class FunNamespaceURI : public NodeFunction
{
	public:
		virtual bool isConstant() const;

	private:
		virtual Value evaluateOnNode( DOM::NodeImpl* node ) const;
};

class FunName : public NodeFunction
{
	public:
		virtual bool isConstant() const;

	private:
		virtual Value evaluateOnNode( DOM::NodeImpl* node ) const;
};

class FunId : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunString : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunConcat : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunStartsWith : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunContains : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunSubstringBefore : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunSubstringAfter : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunSubstring : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunStringLength : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunNormalizeSpace : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunTranslate : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunBoolean : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunNot : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunTrue : public Function
{
	public:
		virtual bool isConstant() const;

	private:
		virtual Value doEvaluate() const;
};

class FunFalse : public Function
{
	public:
		virtual bool isConstant() const;

	private:
		virtual Value doEvaluate() const;
};

class FunLang : public Function
{
	public:
		virtual bool isConstant() const;

	private:
		virtual Value doEvaluate() const;
};

class FunNumber : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunSum : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunFloor : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunCeiling : public Function
{
	private:
		virtual Value doEvaluate() const;
};

class FunRound : public Function
{
	private:
		virtual Value doEvaluate() const;
};

DEFINE_FUNCTION_CREATOR( FunLast )
DEFINE_FUNCTION_CREATOR( FunPosition )
DEFINE_FUNCTION_CREATOR( FunCount )
DEFINE_FUNCTION_CREATOR( FunLocalName )
DEFINE_FUNCTION_CREATOR( FunNamespaceURI )
DEFINE_FUNCTION_CREATOR( FunName )
DEFINE_FUNCTION_CREATOR( FunId )

DEFINE_FUNCTION_CREATOR( FunString )
DEFINE_FUNCTION_CREATOR( FunConcat )
DEFINE_FUNCTION_CREATOR( FunStartsWith )
DEFINE_FUNCTION_CREATOR( FunContains )
DEFINE_FUNCTION_CREATOR( FunSubstringBefore )
DEFINE_FUNCTION_CREATOR( FunSubstringAfter )
DEFINE_FUNCTION_CREATOR( FunSubstring )
DEFINE_FUNCTION_CREATOR( FunStringLength )
DEFINE_FUNCTION_CREATOR( FunNormalizeSpace )
DEFINE_FUNCTION_CREATOR( FunTranslate )

DEFINE_FUNCTION_CREATOR( FunBoolean )
DEFINE_FUNCTION_CREATOR( FunNot )
DEFINE_FUNCTION_CREATOR( FunTrue )
DEFINE_FUNCTION_CREATOR( FunFalse )
DEFINE_FUNCTION_CREATOR( FunLang )

DEFINE_FUNCTION_CREATOR( FunNumber )
DEFINE_FUNCTION_CREATOR( FunSum )
DEFINE_FUNCTION_CREATOR( FunFloor )
DEFINE_FUNCTION_CREATOR( FunCeiling )
DEFINE_FUNCTION_CREATOR( FunRound )

#undef DEFINE_FUNCTION_CREATOR

Interval::Interval()
	: m_min( Inf ),
	m_max( Inf )
{
}

Interval::Interval( int value )
	: m_min( value ),
	m_max( value )
{
}

Interval::Interval( int min, int max )
	: m_min( min ),
	m_max( max )
{
}

bool Interval::contains( int value ) const
{
	if ( m_min == Inf && m_max == Inf ) {
		return true;
	}

	if ( m_min == Inf ) {
		return value <= m_max;
	}

	if ( m_max == Inf ) {
		return value >= m_min;
	}

	return value >= m_min && value <= m_max;
}

QString Interval::asString() const
{
	QString s = "[";

	if ( m_min == Inf ) {
		s += "-Infinity";
	} else {
		s += QString::number( m_min );
	}

	s += "..";

	if ( m_max == Inf ) {
		s += "Infinity";
	} else {
		s += QString::number( m_max );
	}

	s += "]";

	return s;
}

void Function::setArguments( const QList<Expression *> &args )
{
	foreach( Expression *arg, args ) {
		addSubExpression( arg );
	}
}

void Function::setName( const DOM::DOMString &name )
{
	m_name = name;
}

QString Function::dump() const
{
	if ( argCount() == 0 ) {
		return QString( "<function name=\"%1\"/>" ).arg( name().string() );
	}

	QString s = QString( "<function name=\"%1\">" ).arg( name().string() );
	for ( unsigned int i = 0; i < argCount(); ++i ) {
		s += "<operand>" + arg( i )->dump() + "</operand>";
	}
	s += "</function>";
	return s;
}


Expression *Function::arg( int i )
{
	return subExpr( i );
}

const Expression *Function::arg( int i ) const
{
	return subExpr( i );
}

unsigned int Function::argCount() const
{
	return subExprCount();
}

DOM::DOMString Function::name() const
{
	return m_name;
}

Value FunLast::doEvaluate() const
{
	return Value( double( Expression::evaluationContext().size ) );
}

bool FunLast::isConstant() const
{
	return false;
}

Value FunPosition::doEvaluate() const
{
	return Value( double( Expression::evaluationContext().position ) );
}

bool FunPosition::isConstant() const
{
	return false;
}

Value NodeFunction::doEvaluate() const
{
	NodeImpl *node = 0;
	if ( argCount() > 0 ) {
		Value a = arg( 0 )->evaluate();
		if ( a.isNodeset() && a.toNodeset()->length() ) {
			node = a.toNodeset()->first();
		}
	} else {
		// no argument -> default to context node
		node = evaluationContext().node;
	}

	if ( !node )
		return Value( DOMString() );

	return evaluateOnNode( node );
}

bool FunLocalName::isConstant() const
{
	return false;
}

Value FunLocalName::evaluateOnNode( DOM::NodeImpl* node ) const
{
	DOM::DOMString n;
	switch ( node->nodeType() ) {
	case Node::PROCESSING_INSTRUCTION_NODE:
		n = node->nodeName(); // target name
		break;
	default:
		n = node->localName();
	}
	return Value( n );
}

bool FunNamespaceURI::isConstant() const
{
	return false;
}

Value FunNamespaceURI::evaluateOnNode( DOM::NodeImpl* node ) const
{
	return Value( node->namespaceURI() );
}

Value FunId::doEvaluate() const
{
	Value a = arg( 0 )->evaluate();

	WTF::Vector<DOM::DOMString> ids;

	QString queryString; // whitespace-separated IDs
	if ( a.isNodeset() ) {
		DomNodeList set = a.toNodeset();
		for ( unsigned long i = 0; i < set->length(); ++i)
			queryString += stringValue( set->item(i) ).string() + QLatin1Char(' ');
	} else {
		queryString = a.toString().string();
	}
	
	QStringList qids = queryString.simplified().split(' ');
	for ( int i = 0; i < qids.size(); ++i)
			ids.append( DOM::DOMString( qids[i] ) );

	DomNodeList out = new StaticNodeListImpl();
	DOM::DocumentImpl* doc = Expression::evaluationContext().node->document();

	for ( unsigned i = 0; i < ids.size(); ++i ) {
		DOM::ElementImpl* e = doc->getElementById( ids[i] );

		if ( e )
			out->append( e );
	}

	return Value( out );
}

bool FunName::isConstant() const
{
	return false;
}

Value FunName::evaluateOnNode( DOM::NodeImpl* node ) const
{
	DOM::DOMString n;
	switch ( node->nodeType() ) {
	case Node::TEXT_NODE:
	case Node::CDATA_SECTION_NODE:
	case Node::COMMENT_NODE:
	case Node::DOCUMENT_NODE:
		// All of these have an empty XPath name
		break;
	case Node::ELEMENT_NODE: {
		n = static_cast<DOM::ElementImpl*>( node )->nonCaseFoldedTagName();
		break;
	}
	default:
		n = node->nodeName();
	}
	return Value( n );
}

Value FunCount::doEvaluate() const
{
	Value a = arg( 0 )->evaluate();
	if ( !a.isNodeset() ) {
		Expression::reportInvalidExpressionErr();
		qWarning() << "count() expects <nodeset>";
		return Value( );
	}
	a.toNodeset()->normalizeUpto(StaticNodeListImpl::AxisOrder);

	return Value( double( a.toNodeset()->length() ) );
}

bool FunCount::isConstant() const
{
	return false;
}

Value FunString::doEvaluate() const
{
	if ( argCount() == 0 ) {
		DOMString s = Value( Expression::evaluationContext().node ).toString();
		return Value( s );
	}
	return Value( arg( 0 )->evaluate().toString() );
}

Value FunConcat::doEvaluate() const
{
	QString str;
	for ( unsigned int i = 0; i < argCount(); ++i ) {
		str.append( arg( i )->evaluate().toString().string() );
	}
	return Value( DOMString( str ) );
}

Value FunStartsWith::doEvaluate() const
{
	DOMString s1 = arg( 0 )->evaluate().toString();
	DOMString s2 = arg( 1 )->evaluate().toString();

	if ( s2.isEmpty() ) {
		return Value( true );
	}

	return Value( s1.startsWith( s2 ) );
}

Value FunContains::doEvaluate() const
{
	QString s1 = arg( 0 )->evaluate().toString().string();
	QString s2 = arg( 1 )->evaluate().toString().string();

	if ( s2.isEmpty() ) {
		return Value( true );
	}

	return Value( s1.contains( s2 ) );
}

Value FunSubstringBefore::doEvaluate() const
{
	QString s1 = arg( 0 )->evaluate().toString().string();
	QString s2 = arg( 1 )->evaluate().toString().string();

	if ( s2.isEmpty() ) {
		return Value( DOMString() );
	}

	int i = s1.indexOf( s2 );
	if ( i == -1 ) {
		return Value( DOMString() );
	}

	return Value( DOMString( s1.left( i ) ) );
}

Value FunSubstringAfter::doEvaluate() const
{
	QString s1 = arg( 0 )->evaluate().toString().string();
	QString s2 = arg( 1 )->evaluate().toString().string();

	if ( s2.isEmpty() ) {
		return Value( s1 );
	}

	int i = s1.indexOf( s2 );
	if ( i == -1 ) {
		return Value( DOMString() );
	}

	return Value( DOMString( s1.mid( i + s2.length() ) ) );
}

Value FunSubstring::doEvaluate() const
{
	QString s = arg( 0 )->evaluate().toString().string();
	long pos = long( qRound( arg( 1 )->evaluate().toNumber() ) );
	bool haveLength = argCount() == 3;
	long len = -1;
	if ( haveLength ) {
		len = long( qRound( arg( 2 )->evaluate().toNumber() ) );
	}

	if ( pos > long( s.length() ) ) {
		return Value( DOMString() );
	}

	if ( haveLength && pos < 1 ) {
		len -= 1 - pos;
		pos = 1;
		if ( len < 1 ) {
			return Value( DOMString() );
		}
	}

	return Value( DOMString( s.mid( pos - 1, len ) ) );
}

Value FunStringLength::doEvaluate() const
{
	if ( argCount() == 0 ) {
		DOMString s = Value( Expression::evaluationContext().node ).toString();
		return Value( double( s.length() ) );
	}

	return Value( double( arg( 0 )->evaluate().toString().length() ) );
}

Value FunNormalizeSpace::doEvaluate() const
{
	if ( argCount() == 0 ) {
		DOMString s = Value( Expression::evaluationContext().node ).toString();
		return Value( DOMString( s.string().simplified() ) );
	}

	QString s = arg( 0 )->evaluate().toString().string();
	s = s.simplified();
	return Value( DOMString( s ) );
}

Value FunTranslate::doEvaluate() const
{
	QString s1 = arg( 0 )->evaluate().toString().string();
	QString s2 = arg( 1 )->evaluate().toString().string();
	QString s3 = arg( 2 )->evaluate().toString().string();
	QString newString;

	for ( int i1 = 0; i1 < s1.length(); ++i1 ) {
		QChar ch = s1[ i1 ];
		int i2 = s2.indexOf( ch );
		if ( i2 == -1 ) {
			newString += ch;
		} else if ( i2 < s3.length() ) {
			newString += s3[ i2 ];
		}
	}

	return Value( DOMString( newString ) );
}

Value FunBoolean::doEvaluate() const
{
	return Value( arg( 0 )->evaluate().toBoolean() );
}

Value FunNot::doEvaluate() const
{
	return Value( !arg( 0 )->evaluate().toBoolean() );
}

Value FunTrue::doEvaluate() const
{
	return Value( true );
}

bool FunTrue::isConstant() const
{
	return true;
}

#ifdef __GNUC__
#warning "This looks bogus"
#endif

Value FunLang::doEvaluate() const
{
	QString lang = arg( 0 )->evaluate().toString().string();

	NodeImpl* node = evaluationContext().node;

	DOMString langNodeValue;

	while ( node ) {
		if (node->isElementNode()) {
			langNodeValue = static_cast<ElementImpl*>(node)->getAttribute("xml:lang");
			if ( !langNodeValue.isNull() )
				break;
		}
		node = xpathParentNode( node );		
	}

	if ( langNodeValue.isNull() ) {
		return Value( false );
	}

	// extract 'en' out of 'en-us'
	QString langNodeValueString = langNodeValue.string();
	QString langNodeBaseString = langNodeValueString.left( langNodeValueString.indexOf( '-' ) );

	return Value( langNodeValueString.toLower() == lang.toLower() ||
	              langNodeBaseString.toLower()  == lang.toLower() );
}

bool FunLang::isConstant() const
{
	return false;
}

Value FunFalse::doEvaluate() const
{
	return Value( false );
}

bool FunFalse::isConstant() const
{
	return true;
}

Value FunNumber::doEvaluate() const
{
	Value vi;
	if ( argCount() == 0 ) {
		// Spec'd: convert context node to singleton nodeset, call
		// string on that --> that's just stringValue on that node.
		// then we call number on that string
		vi = Value(stringValue(evaluationContext().node));
	} else {
		vi = arg( 0 )->evaluate();
	}
	
	return Value( vi.toNumber() );
}

Value FunSum::doEvaluate() const
{
	Value a = arg( 0 )->evaluate();
	if ( !a.isNodeset() ) {
		Expression::reportInvalidExpressionErr();
		qWarning() << "sum() expects <nodeset>";
		return Value( 0.0 );
	}

	double sum = 0.0;
	const DomNodeList nodes = a.toNodeset();
	for (unsigned long n = 0; n < nodes->length(); ++n) {
		NodeImpl* node = nodes->item(n);
		sum += Value( stringValue( node ) ).toNumber();
	}
	return Value( sum );
}

Value FunFloor::doEvaluate() const
{
	const double num = arg( 0 )->evaluate().toNumber();

	if ( KJS::isNaN( num ) || KJS::isInf( num ) ) {
		return Value( num );
	}

	return Value( floor( num ) );
}

Value FunCeiling::doEvaluate() const
{
	const double num = arg( 0 )->evaluate().toNumber();

	if ( KJS::isNaN( num ) || KJS::isInf( num ) ) {
		return Value( num );
	}

	return Value( ceil( num ) );
}

Value FunRound::doEvaluate() const
{
	return Value( double( qRound( arg( 0 )->evaluate().toNumber() ) ) );
}

struct FunctionLibrary::FunctionRec
{
	typedef Function *(*FactoryFn )();

	FactoryFn factoryFn;
	Interval args;
};

struct FunctionMapping
{
	const char *name;
	FunctionLibrary::FunctionRec function;
};

static FunctionMapping functions[] = {
	{ "last", { &createFunLast, 0 } },
	{ "last", { &createFunLast, 0 } },
	{ "position", { &createFunPosition, 0 } },
	{ "count", { &createFunCount, 1 } },
	{ "sum", { &createFunSum, 1 } },
	{ "local-name", { &createFunLocalName, Interval( 0, 1 ) } },
	{ "namespace-uri", { &createFunNamespaceURI, Interval( 0, 1 ) } },
	{ "id",   { &createFunId, 1 } },
	{ "name", { &createFunName, Interval( 0, 1 ) } },
	

	{ "string", { &createFunString, Interval( 0, 1 ) } },
	{ "concat", { &createFunConcat, Interval( 2, Interval::Inf ) } },
	{ "starts-with", { &createFunStartsWith, 2 } },
	{ "contains", { &createFunContains, 2 } },
	{ "substring-before", { &createFunSubstringBefore, 2 } },
	{ "substring-after", { &createFunSubstringAfter, 2 } },
	{ "substring", { &createFunSubstring, Interval( 2, 3 ) } },
	{ "string-length", { &createFunStringLength, Interval( 0, 1 ) } },
	{ "normalize-space", { &createFunNormalizeSpace, Interval( 0, 1 ) } },
	{ "translate", { &createFunTranslate, 3 } },

	{ "boolean", { &createFunBoolean, 1 } },
	{ "not", { &createFunNot, 1 } },
	{ "true", { &createFunTrue, 0 } },
	{ "false", { &createFunFalse, 0 } },
	{ "lang", { &createFunLang, 1 } },

	{ "number", { &createFunNumber, Interval( 0, 1 ) } },
	{ "floor", { &createFunFloor, 1 } },
	{ "ceiling", { &createFunCeiling, 1 } },
	{ "round", { &createFunRound, 1 } }
};
static const unsigned int numFunctions = sizeof( functions ) / sizeof( functions[ 0 ] );

FunctionLibrary &FunctionLibrary::self()
{
	static FunctionLibrary instance;
	return instance;
}

FunctionLibrary::FunctionLibrary()
{
	for ( unsigned int i = 0; i < numFunctions; ++i ) {
		m_functionDict.insert( functions[ i ].name, functions[ i ].function );
	}
}

Function *FunctionLibrary::getFunction( const DOM::DOMString& name,
                                        const QList<Expression *> &args ) const
{
	if ( !m_functionDict.contains( name ) ) {
		qWarning() << "Function '" << name << "' not supported by this implementation.";

		return 0;
	}

	FunctionRec functionRec = m_functionDict[ name ];
	if ( !functionRec.args.contains( args.count() ) ) {
		qWarning() << "Function '" << name << "' requires " << functionRec.args.asString() << " arguments, but " << args.count() << " given.";
		return 0;
	}

	Function *function = functionRec.factoryFn();
	function->setArguments( args );
	function->setName( name );
	return function;
}

} //namespace XPath
} //namespace khtml

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
