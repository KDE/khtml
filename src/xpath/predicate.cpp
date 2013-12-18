/*
 * predicate.cc - Copyright 2005 Frerich Raabe   <raabe@kde.org>
 *                Copyright 2010 Maksim Orlovich <maksim@kde.org>
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
#include "predicate.h"
#include "functions.h"

#include <QString>

#include "xml/dom_nodeimpl.h"
#include "xml/dom_nodelistimpl.h"
#include "kjs/operations.h"
#include "kjs/value.h"

#include <math.h>

using namespace DOM;
using namespace khtml;
using namespace khtml::XPath;

Number::Number( double value )
	: m_value( value )
{
}

bool Number::isConstant() const
{
	return true;
}

QString Number::dump() const
{
	return "<number>" + QString::number( m_value ) + "</number>";
}

Value Number::doEvaluate() const
{
	return Value( m_value );
}

String::String( const DOMString &value )
	: m_value( value )
{
}

bool String::isConstant() const
{
	return true;
}

QString String::dump() const
{
	return "<string>" + m_value.string() + "</string>";
}

Value String::doEvaluate() const
{
	return Value( m_value );
}

Value Negative::doEvaluate() const
{
	Value p( subExpr( 0 )->evaluate() );
	return Value( -p.toNumber() );
}

QString Negative::dump() const
{
	return "<negative>" + subExpr( 0 )->dump() + "</number>";
}

QString BinaryExprBase::dump() const
{
	QString s = "<" + opName() + ">";
	s += "<operand>" + subExpr( 0 )->dump() + "</operand>";
	s += "<operand>" + subExpr( 1 )->dump() + "</operand>";
	s += "</" + opName() + ">";
	return s;
}

NumericOp::NumericOp( int _opCode, Expression* lhs, Expression* rhs ) :
	opCode( _opCode )
{
	addSubExpression( lhs );
	addSubExpression( rhs );
}

Value NumericOp::doEvaluate() const
{
	Value lhs( subExpr( 0 )->evaluate() );
	Value rhs( subExpr( 1 )->evaluate() );
	double leftVal = lhs.toNumber(), rightVal = rhs.toNumber();

	switch (opCode) {
		case OP_Add:
			return Value( leftVal + rightVal );
		case OP_Sub:
			return Value( leftVal - rightVal );
		case OP_Mul:
			return Value( leftVal * rightVal );
		case OP_Div:
			if ( rightVal == 0.0 || rightVal == -0.0 ) {
				if ( leftVal == 0.0 || leftVal == -0.0) {
					return Value(); // 0/0 = NaN
				} else {
					// +/- Infinity.
					using namespace std; // for signbit
					if (signbit(leftVal) == signbit(rightVal))
						return Value( KJS::Inf );
					else
						return Value( -KJS::Inf );
				}
			} else {
				return Value( leftVal / rightVal );
            }
		case OP_Mod:
			if ( rightVal == 0.0 || rightVal == -0.0 )
				return Value(); //Divide by 0;
			else
				return Value( remainder( leftVal, rightVal ) );
		default:
			assert(0);
		return Value();
	}
}

QString NumericOp::opName() const
{
	switch (opCode) {
		case OP_Add:
			return QLatin1String( "addition" );
		case OP_Sub:
			return QLatin1String( "subtraction" );
		case OP_Mul:
			return QLatin1String( "multiplication" );
		case OP_Div:
			return QLatin1String( "division" );
		case OP_Mod:
			return QLatin1String( "modulo" );
		default:
			assert(0);
			return QString();
	}
}

RelationOp::RelationOp( int _opCode, Expression* lhs, Expression* rhs ) :
	opCode( _opCode )
{
	addSubExpression( lhs );
	addSubExpression( rhs );
}

static void stringify(const Value& val, WTF::Vector<DOMString>* out)
{
	if (val.isString()) {
		out->append(val.toString());
	} else {
		assert(val.isNodeset());

		const DomNodeList& set = val.toNodeset();
		for (unsigned long i = 0; i < set->length(); ++i) {
			DOM::DOMString stringVal = stringValue(set->item(i));
			out->append(stringVal);
		}
	}
}

static void numify(const Value& val, WTF::Vector<double>* out)
{
	if (val.isNumber()) {
		out->append(val.toNumber());
	} else {
		assert(val.isNodeset());

		const DomNodeList& set = val.toNodeset();
		for (unsigned long i = 0; i < set->length(); ++i) {
			DOM::DOMString stringVal = stringValue(set->item(i));
			out->append(Value(stringVal).toNumber());
		}
	}
}

Value RelationOp::doEvaluate() const
{
	Value lhs( subExpr( 0 )->evaluate() );
	Value rhs( subExpr( 1 )->evaluate() );

	if (lhs.isNodeset() || rhs.isNodeset())
	{
		// If both are nodesets, or one is a string our
		// comparisons are based on strings.
		if ((lhs.isNodeset() && rhs.isNodeset()) ||
		    (lhs.isString()  || rhs.isString())) {

			WTF::Vector<DOM::DOMString> leftStrings;
			WTF::Vector<DOM::DOMString> rightStrings;

			stringify(lhs, &leftStrings);
			stringify(rhs, &rightStrings);

			for (unsigned pl = 0; pl < leftStrings.size(); ++pl) {
				for (unsigned pr = 0; pr < rightStrings.size(); ++pr) {
					if (compareStrings(leftStrings[pl], rightStrings[pr]))
						return Value(true);
				} // pr
			} // pl
			return Value(false);
		}

		// If one is a number, we do a number-based comparison
		if (lhs.isNumber() || rhs.isNumber()) {
			WTF::Vector<double> leftNums;
			WTF::Vector<double> rightNums;

			numify(lhs, &leftNums);
			numify(rhs, &rightNums);

			for (unsigned pl = 0; pl < leftNums.size(); ++pl) {
				for (unsigned pr = 0; pr < rightNums.size(); ++pr) {
					if (compareNumbers(leftNums[pl], rightNums[pr]))
						return Value(true);
				} // pr
			} // pl
			return Value(false);
		}

		// Has to be a boolean-based comparison.
		// These ones are simpler, since we just convert the nodeset to a bool		
		assert(lhs.isBoolean() || rhs.isBoolean());

		if (lhs.isNodeset())
			lhs = Value(lhs.toBoolean());
		else
			rhs = Value(rhs.toBoolean());
	} // nodeset comparisons
	

	if (opCode == OP_EQ || opCode == OP_NE) {
		bool equal;
		if ( lhs.isBoolean() || rhs.isBoolean() ) {
			equal = ( lhs.toBoolean() == rhs.toBoolean() );
		} else if ( lhs.isNumber() || rhs.isNumber() ) {
			equal = ( lhs.toNumber() == rhs.toNumber() );
		} else {
			equal = ( lhs.toString() == rhs.toString() );
		}

		if ( opCode == OP_EQ )
			return Value( equal );
		else
			return Value( !equal );

	}

	// For other ops, we always convert to numbers
	double leftVal = lhs.toNumber(), rightVal = rhs.toNumber();
	return Value(compareNumbers(leftVal, rightVal));
}


bool RelationOp::compareNumbers(double leftVal, double rightVal) const
{
	switch (opCode) {
		case OP_GT:
			return leftVal > rightVal;
		case OP_GE:
			return leftVal >= rightVal;
		case OP_LT:
			return leftVal < rightVal;
		case OP_LE:
			return leftVal <= rightVal;
		case OP_EQ:
			return leftVal == rightVal;
		case OP_NE:
			return leftVal != rightVal;
		default:
			assert(0);
			return false;
	}
}

bool RelationOp::compareStrings(const DOM::DOMString& l, const DOM::DOMString& r) const
{
	// String comparisons, as invoked within the nodeset case, are string-based
	// only for == and !=. Everything else still goes to numbers.
	if (opCode == OP_EQ)
		return (l == r);
	if (opCode == OP_NE)
		return (l != r);

	return compareNumbers(Value(l).toNumber(), Value(r).toNumber());
}

QString RelationOp::opName() const
{
	switch (opCode) {
		case OP_GT:
			return QLatin1String( "relationGT" );
		case OP_GE:
			return QLatin1String( "relationGE" );
		case OP_LT:
			return QLatin1String( "relationLT" );
		case OP_LE:
			return QLatin1String( "relationLE" );
		case OP_EQ:
			return QLatin1String( "relationEQ" );
		case OP_NE:
			return QLatin1String( "relationNE" );
		default:
			assert(0);
			return QString();
	}
}

LogicalOp::LogicalOp( int _opCode, Expression* lhs, Expression* rhs ) :
	opCode( _opCode )
{
	addSubExpression( lhs );
	addSubExpression( rhs );
}

bool LogicalOp::shortCircuitOn() const
{
	if (opCode == OP_And)
		return false; //false and foo
	else
		return true;  //true or bar
}

bool LogicalOp::isConstant() const
{
	return subExpr( 0 )->isConstant() &&
	       subExpr( 0 )->evaluate().toBoolean() == shortCircuitOn();
}

QString LogicalOp::opName() const
{
	if ( opCode == OP_And )
		return QLatin1String( "conjunction" );
	else
		return QLatin1String( "disjunction" );
}

Value LogicalOp::doEvaluate() const
{
	Value lhs( subExpr( 0 )->evaluate() );

	// This is not only an optimization, http://www.w3.org/TR/xpath
	// dictates that we must do short-circuit evaluation
	bool lhsBool = lhs.toBoolean();
	if ( lhsBool == shortCircuitOn() ) {
		return Value( lhsBool );
	}

	return Value( subExpr( 1 )->evaluate().toBoolean() );
}

QString Union::opName() const
{
	return QLatin1String("union");
}

Value Union::doEvaluate() const
{
	Value lhs = subExpr( 0 )->evaluate();
	Value rhs = subExpr( 1 )->evaluate();
	if ( !lhs.isNodeset() || !rhs.isNodeset() ) {
		qWarning() << "Union operator '|' works only with nodesets.";
		Expression::reportInvalidExpressionErr();
		return Value( new StaticNodeListImpl );
	}

	DomNodeList lhsNodes = lhs.toNodeset();
	DomNodeList rhsNodes = rhs.toNodeset();
	DomNodeList result = new StaticNodeListImpl;

	for ( unsigned long n = 0; n < lhsNodes->length(); ++n )
		result->append( lhsNodes->item( n ) );

	for ( unsigned long n = 0; n < rhsNodes->length(); ++n )
		result->append( rhsNodes->item( n ) );
	
	return Value( result );
}

Predicate::Predicate( Expression *expr )
	: m_expr( expr )
{
}

Predicate::~Predicate()
{
	delete m_expr;
}

bool Predicate::evaluate() const
{
	Q_ASSERT( m_expr != 0 );

	Value result( m_expr->evaluate() );

	// foo[3] really means foo[position()=3]
	if ( result.isNumber() ) {
		return double( Expression::evaluationContext().position ) == result.toNumber();
	}

	return result.toBoolean();
}

void Predicate::optimize()
{
	m_expr->optimize();
}

QString Predicate::dump() const
{
	return QString() + "<predicate>" + m_expr->dump() + "</predicate>";
}

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
