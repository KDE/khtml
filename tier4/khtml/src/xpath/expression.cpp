/*
 * expression.cc - Copyright 2005 Frerich Raabe <raabe@kde.org>
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
#include "expression.h"
#include "xml/dom_nodeimpl.h"
#include "xml/dom_nodelistimpl.h"
#include "dom/dom_exception.h"
#include "dom/dom3_xpath.h"
//#include "xml/dom_stringimpl.h"

#include <cmath>

using namespace DOM;
using namespace khtml;
using namespace khtml::XPath;
using namespace std;

// Use KJS's numeric FP stuff
#include "kjs/JSImmediate.h"
#include "kjs/operations.h"

Value::Value():
	m_type  ( Number ),
	m_number( KJS::NaN )
{
	// ### remove eventually
}

Value::Value( NodeImpl *value )
	: m_type( Nodeset )
{
	m_nodeset = new StaticNodeListImpl;
	m_nodeset->append( value );
}

Value::Value( const DomNodeList &value )
	: m_type( Nodeset ),
	m_nodeset( value )
{
}

Value::Value( bool value )
	: m_type( Boolean ),
	m_bool( value )
{
}

Value::Value( double value )
	: m_type( Number ),
	m_number( value )
{
}

Value::Value( const DOMString &value )
	: m_type( String ),
	m_string( value )
{
}

Value::Type Value::type() const
{
	return m_type;
}

bool Value::isNodeset() const
{
	return m_type == Nodeset;
}

bool Value::isBoolean() const
{
	return m_type == Boolean;
}

bool Value::isNumber() const
{
	return m_type == Number;
}

bool Value::isString() const
{
	return m_type == String;
}

DomNodeList &Value::toNodeset()
{
	if ( m_type != Nodeset ) {
		qWarning() << "Cannot convert anything to a nodeset.";
	}
	return m_nodeset;
}

const DomNodeList &Value::toNodeset() const
{
	if ( m_type != Nodeset ) {
		qWarning() << "Cannot convert anything to a nodeset.";
	}
	return m_nodeset;
}

bool Value::toBoolean() const
{
	switch ( m_type ) {
		case Nodeset:
			return m_nodeset && m_nodeset->length() != 0;
		case Boolean:
			return m_bool;
		case Number:
			return m_number != 0;
		case String:
			return !m_string.isEmpty();
	}
	return bool();
}

double Value::toNumber() const
{
	switch ( m_type ) {
		case Nodeset:
			return Value( toString() ).toNumber();
		case Number:
			return m_number;
		case String: {
			bool canConvert;
			QString s = m_string.string().simplified();
			double value = s.toDouble( &canConvert );
			if ( canConvert ) {
				return value;
			} else {
				return KJS::NaN;
			}
		}
		case Boolean:
			return m_bool ? 1 : 0;
	}
	return double();
}

DOMString Value::toString() const
{
	switch ( m_type ) {
		case Nodeset:
			if ( m_nodeset && m_nodeset->length() == 0 ) {
				return DOMString( "" );
			}
			m_nodeset->normalizeUpto( StaticNodeListImpl::AxisOrder );
			return stringValue( m_nodeset->item(0) );
		case String:
			return m_string;
		case Number:
			if ( KJS::isNaN( m_number ) ) {
				return DOMString( "NaN" );
			} else if ( m_number == 0 ) {
				return DOMString( "0" );
			} else if ( KJS::isInf( m_number ) ) {
				if ( signbit( m_number ) == 0 ) {
					return DOMString( "Infinity" );
				} else {
					return DOMString( "-Infinity" );
				}
			}
			return QString::number( m_number );
		case Boolean:
			return m_bool ? DOMString( "true" )
			              : DOMString( "false" );
	}
	return DOMString();
}

QString Value::dump() const
{
	QString s = "<value type=\"";
	switch ( m_type ) {
		case Nodeset:
			s += "nodeset";
			break;
		case String:
			s += "string";
			break;
		case Number:
			s += "number";
			break;
		case Boolean:
			s += "boolean";
			break;
	};
	s += "\">" + toString().string() + "</value>";
	return s;
}

EvaluationContext &Expression::evaluationContext()
{
	static EvaluationContext evaluationContext;
	return evaluationContext;
}

Expression::Expression()
	: m_constantValue( 0 )
{
}

Expression::~Expression()
{
	qDeleteAll( m_subExpressions );
	delete m_constantValue;
}

Value Expression::evaluate() const
{
	if ( m_constantValue ) {
		return *m_constantValue;
	}
	return doEvaluate();
}

void Expression::addSubExpression( Expression *expr )
{
	m_subExpressions.append( expr );
}

void Expression::optimize()
{
	bool allSubExpressionsConstant = true;
	foreach( Expression *expr, m_subExpressions ) {
		if ( expr->isConstant() ) {
			expr->optimize();
		} else {
			allSubExpressionsConstant = false;
		}
	}

	if ( allSubExpressionsConstant ) {
		m_constantValue = new Value( doEvaluate() );

		qDeleteAll( m_subExpressions );
		m_subExpressions.clear();
	}
}

unsigned int Expression::subExprCount() const
{
	return m_subExpressions.count();
}

Expression *Expression::subExpr( unsigned int i )
{
	Q_ASSERT( i < subExprCount() );
	return m_subExpressions.at( i );
}

const Expression *Expression::subExpr( unsigned int i ) const
{
	Q_ASSERT( i < subExprCount() );
	return m_subExpressions.at( i );
}

bool Expression::isConstant() const
{
	foreach( Expression *expr, m_subExpressions ) {
		if ( !expr->isConstant() ) {
			return false;
		}
	}
	return true;
}

void Expression::reportInvalidExpressionErr()
{
	Expression::evaluationContext().reportException(XPathException::toCode(INVALID_EXPRESSION_ERR));
}

void Expression::reportNamespaceErr()
{
	Expression::evaluationContext().reportException(DOMException::NAMESPACE_ERR);
}

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
