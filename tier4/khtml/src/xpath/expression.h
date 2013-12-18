/*
 * expression.h - Copyright 2005 Frerich Raabe <raabe@kde.org>
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
#ifndef EXPRESSION_H
#define EXPRESSION_H

#include "util.h"

#include <QHash>
#include <QList>

#include <dom/dom_string.h>
#include <xml/dom_stringimpl.h>
#include <xml/dom_nodelistimpl.h>
#include <misc/shared.h>

namespace DOM {
	class NodeImpl;
}

namespace khtml {

class XPathNSResolverImpl;
namespace XPath {

struct EvaluationContext
{
	EvaluationContext() { reset(0, 0); }

	void reset(DOM::NodeImpl* ctx, XPathNSResolverImpl* res) {
		node     = ctx;
		resolver = res;
		size     = 1;
		position = 1;
		exceptionCode = 0;
	}

	DOM::NodeImpl *node;
	unsigned long size;
	unsigned long position;
	QHash<DOM::DOMString, DOM::DOMString> variableBindings;

	// reports only first one.
	void reportException(int ec) { if (!exceptionCode) exceptionCode = ec; } 

	int exceptionCode;
	
	/* The function library is globally accessible through
	 * FunctionLibrary::self()
	 */
	XPathNSResolverImpl *resolver;
};

class Value
{
	public:
		enum Type {
			Nodeset, Boolean, Number, String
		};

		Value();
		explicit Value( DOM::NodeImpl *value );
		explicit Value( const DomNodeList &value );
		explicit Value( bool value );
		explicit Value( double value );
		explicit Value( const DOM::DOMString &value );

		Type type() const;
		bool isNodeset() const;
		bool isBoolean() const;
		bool isNumber() const;
		bool isString() const;

		DomNodeList &toNodeset(); // may return 0
		const DomNodeList &toNodeset() const;
		bool toBoolean() const;
		double toNumber() const;
		DOM::DOMString toString() const;

		QString dump() const;

	private:
		// Catch undesired conversions -- this manages to go to bool otherwise!
		explicit Value( const char* );
	
		Type m_type;
		DomNodeList m_nodeset;
		bool m_bool;
		double m_number;
		DOM::DOMString m_string;
};

class Expression
{
	public:
		static EvaluationContext &evaluationContext();

		Expression();
		virtual ~Expression();
		virtual Value evaluate() const;

		void addSubExpression( Expression *expr );
		void optimize();
		virtual bool isConstant() const;

		virtual QString dump() const = 0;

		static void reportInvalidExpressionErr();
		static void reportNamespaceErr();
	protected:
		unsigned int subExprCount() const;
		Expression *subExpr( unsigned int i );
		const Expression *subExpr( unsigned int i ) const;

	private:
		virtual Value doEvaluate() const = 0;

		QList<Expression *> m_subExpressions;
		Value *m_constantValue;
};

} // namespace XPath

} // namespace khtml

#endif // EXPRESSION_H

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
