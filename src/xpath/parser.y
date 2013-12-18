%{
#include "functions.h"
#include "path.h"
#include "predicate.h"
#include "util.h"
#include "tokenizer.h"

#include "expression.h"
#include "util.h"
#include "variablereference.h"

#include "dom/dom_string.h"
#include "dom/dom_exception.h"
#include "dom/dom3_xpath.h"
#include "xml/dom_stringimpl.h"
#include "xml/dom3_xpathimpl.h"

using namespace DOM;
using namespace DOM::XPath;
using namespace khtml;
using namespace khtml::XPath;



#include <QList>
#include <QPair>
#include <QtDebug>

#define YYDEBUG 1

Expression * khtmlParseXPathStatement( const DOM::DOMString &statement, int& ec );

static Expression *_topExpr;
static int xpathParseException;

%}

%union
{
	khtml::XPath::Step::AxisType axisType;
	int        num;
	DOM::DOMString *str; // we use this and not DOMStringImpl*, so the
	                     // memory management for this is entirely manual,
	                     // and not an RC/manual hybrid
	khtml::XPath::Expression *expr;
	QList<khtml::XPath::Predicate *> *predList;
	QList<khtml::XPath::Expression *> *argList;
	khtml::XPath::Step *step;
	khtml::XPath::LocationPath *locationPath;
}

%{
%}

%left <num> MULOP RELOP EQOP
%left <str> PLUS MINUS
%left <str> OR AND
%token <axisType> AXISNAME
%token <str> NODETYPE PI FUNCTIONNAME LITERAL
%token <str> VARIABLEREFERENCE NUMBER
%token <str> DOTDOT SLASHSLASH NAMETEST
%token ERROR

%type <locationPath> LocationPath
%type <locationPath> AbsoluteLocationPath
%type <locationPath> RelativeLocationPath
%type <step> Step
%type <axisType> AxisSpecifier
%type <step> DescendantOrSelf
%type <str> NodeTest
%type <expr> Predicate
%type <predList> PredicateList
%type <step> AbbreviatedStep
%type <expr> Expr
%type <expr> PrimaryExpr
%type <expr> FunctionCall
%type <argList> ArgumentList
%type <expr> Argument
%type <expr> UnionExpr
%type <expr> PathExpr
%type <expr> FilterExpr
%type <expr> OrExpr
%type <expr> AndExpr
%type <expr> EqualityExpr
%type <expr> RelationalExpr
%type <expr> AdditiveExpr
%type <expr> MultiplicativeExpr
%type <expr> UnaryExpr

%%

Expr:
	OrExpr
	{
		_topExpr = $1;
	}
	;

LocationPath:
	RelativeLocationPath
	{
		$$->m_absolute = false;
	}
	|
	AbsoluteLocationPath
	{
		$$->m_absolute = true;
	}
	;

AbsoluteLocationPath:
	'/'
	{
		$$ = new LocationPath;
	}
	|
	'/' RelativeLocationPath
	{
		$$ = $2;
	}
	|
	DescendantOrSelf RelativeLocationPath
	{
		$$ = $2;
		$$->m_steps.prepend( $1 );
	}
	;

RelativeLocationPath:
	Step
	{
		$$ = new LocationPath;
		$$->m_steps.append( $1 );
	}
	|
	RelativeLocationPath '/' Step
	{
		$$->m_steps.append( $3 );
	}
	|
	RelativeLocationPath DescendantOrSelf Step
	{
		$$->m_steps.append( $2 );
		$$->m_steps.append( $3 );
	}
	;

Step:
	NodeTest
	{
		$$ = new Step( Step::ChildAxis, *$1 );
		delete $1;
	}
	|
	NodeTest PredicateList
	{
		$$ = new Step( Step::ChildAxis, *$1, *$2 );
		delete $1;
		delete $2;
	}
	|
	AxisSpecifier NodeTest
	{
		$$ = new Step( $1, *$2 );
		delete $2;
	}
	|
	AxisSpecifier NodeTest PredicateList
	{
		$$ = new Step( $1, *$2,  *$3 );
		delete $2;
		delete $3;
	}
	|
	AbbreviatedStep
	;

AxisSpecifier:
	AXISNAME
	|
	'@'
	{
		$$ = Step::AttributeAxis;
	}
	;

NodeTest:
	NAMETEST
	{
		const int colon = $1->find( ':' );
		if ( colon > -1 ) {
			DOMString prefix( $1->substring( 0, colon ) );
			XPathNSResolverImpl *resolver = Expression::evaluationContext().resolver;
			if ( !resolver || resolver->lookupNamespaceURI( prefix ).isNull() ) {
				qWarning() << "Found unknown namespace prefix " << prefix.string();
				xpathParseException = DOMException::NAMESPACE_ERR;
				YYABORT;
			}
		}
	}
	|
	NODETYPE '(' ')'
	{
		$$ = new DOMString( *$1 + DOMString("()") );
	}
	|
	PI '(' ')'
	|
	PI '(' LITERAL ')'
	{
		QString s = $1->string() + QLatin1Char(' ') + $3->string();
		s = s.trimmed();
		$$ = new DOMString( s );
		delete $1;
		delete $3;
	}
	;

PredicateList:
	Predicate
	{
		$$ = new QList<Predicate *>;
		$$->append( new Predicate( $1 ) );
	}
	|
	PredicateList Predicate
	{
		$$->append( new Predicate( $2 ) );
	}
	;

Predicate:
	'[' Expr ']'
	{
		$$ = $2;
	}
	;

DescendantOrSelf:
	SLASHSLASH
	{
		$$ = new Step( Step::DescendantOrSelfAxis, "node()" );
	}
	;

AbbreviatedStep:
	'.'
	{
		$$ = new Step( Step::SelfAxis, "node()" );
	}
	|
	DOTDOT
	{
		$$ = new Step( Step::ParentAxis, "node()" );
	}
	;

PrimaryExpr:
	VARIABLEREFERENCE
	{
		$$ = new VariableReference( *$1 );
		delete $1;
	}
	|
	'(' Expr ')'
	{
		$$ = $2;
	}
	|
	LITERAL
	{
		$$ = new String( *$1 );
		delete $1;
	}
	|
	NUMBER
	{
		$$ = new Number( $1->toFloat() );
		delete $1;
	}
	|
	FunctionCall
	;

FunctionCall:
	FUNCTIONNAME '(' ')'
	{
		Function* f = FunctionLibrary::self().getFunction( *$1 );
		delete $1;
		if (!f) {
			xpathParseException = XPathException::toCode(INVALID_EXPRESSION_ERR);
			YYABORT;
		}
		
		$$ = f;
	}
	|
	FUNCTIONNAME '(' ArgumentList ')'
	{
		Function* f = FunctionLibrary::self().getFunction( *$1, *$3 );
		delete $1;
		delete $3;
		if (!f) {
			xpathParseException = XPathException::toCode(INVALID_EXPRESSION_ERR);
			YYABORT;
		}
		$$ = f;
	}
	;

ArgumentList:
	Argument
	{
		$$ = new QList<Expression *>;
		$$->append( $1 );
	}
	|
	ArgumentList ',' Argument
	{
		$$->append( $3 );
	}
	;

Argument:
	Expr
	;


UnionExpr:
	PathExpr
	|
	UnionExpr '|' PathExpr
	{
		$$ = new Union;
		$$->addSubExpression( $1 );
		$$->addSubExpression( $3 );
	}
	;

PathExpr:
	LocationPath
	{
		$$ = $1;
	}
	|
	FilterExpr
	{
		$$ = $1;
	}
	|
	FilterExpr '/' RelativeLocationPath
	{
		$$ = new Path( static_cast<Filter *>( $1 ), $3 );
	}
	|
	FilterExpr DescendantOrSelf RelativeLocationPath
	{
		$3->m_steps.prepend( $2 );
		$$ = new Path( static_cast<Filter *>( $1 ), $3 );
	}
	;

FilterExpr:
	PrimaryExpr
	{
		$$ = $1;
	}
	|
	PrimaryExpr PredicateList
	{
		$$ = new Filter( $1, *$2 );
	}
	;

OrExpr:
	AndExpr
	|
	OrExpr OR AndExpr
	{
		$$ = new LogicalOp( LogicalOp::OP_Or, $1, $3 );
	}
	;

AndExpr:
	EqualityExpr
	|
	AndExpr AND EqualityExpr
	{
		$$ = new LogicalOp( LogicalOp::OP_And, $1, $3 );
	}
	;

EqualityExpr:
	RelationalExpr
	|
	EqualityExpr EQOP RelationalExpr
	{
		$$ = new RelationOp( $2, $1, $3 );
	}
	;

RelationalExpr:
	AdditiveExpr
	|
	RelationalExpr RELOP AdditiveExpr
	{
		$$ = new RelationOp( $2, $1, $3 );
	}
	;

AdditiveExpr:
	MultiplicativeExpr
	|
	AdditiveExpr PLUS MultiplicativeExpr
	{
		$$ = new NumericOp( NumericOp::OP_Add, $1, $3 );
	}
	|
	AdditiveExpr MINUS MultiplicativeExpr
	{
		$$ = new NumericOp( NumericOp::OP_Sub, $1, $3 );
	}
	;

MultiplicativeExpr:
	UnaryExpr
	|
	MultiplicativeExpr MULOP UnaryExpr
	{
		$$ = new NumericOp( $2, $1, $3 );
	}
	;

UnaryExpr:
	UnionExpr
	|
	MINUS UnaryExpr
	{
		$$ = new Negative;
		$$->addSubExpression( $2 );
	}
	;

%%

namespace khtml {
namespace XPath {

Expression *khtmlParseXPathStatement( const DOM::DOMString &statement, int& ec )
{
//	qDebug() << "Parsing " << statement;
	xpathParseException = 0;
	_topExpr = 0;
	initTokenizer( statement );
	yyparse();

	if (xpathParseException)
		ec = xpathParseException;
	return _topExpr;
}

void khtmlxpathyyerror(const char *str)
{
	// XXX Clean xpathyylval.str up to avoid leaking it.
	fprintf(stderr, "error: %s\n", str);
	xpathParseException = XPathException::toCode(INVALID_EXPRESSION_ERR);
}

} // namespace XPath
} // namespace khtml

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
