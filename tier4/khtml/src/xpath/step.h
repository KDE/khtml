/*
 * step.h - Copyright 2005 Frerich Raabe <raabe@kde.org>
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
#ifndef STEP_H
#define STEP_H

#include "predicate.h"
#include "util.h"

#include <QList>

#include <dom/dom_string.h>

namespace DOM {
	class NodeImpl;
}

namespace khtml {
namespace XPath {

class Step
{
	public:
		enum AxisType {
			AncestorAxis=1, AncestorOrSelfAxis, AttributeAxis,
			ChildAxis, DescendantAxis, DescendantOrSelfAxis,
			FollowingAxis, FollowingSiblingAxis, NamespaceAxis,
			ParentAxis, PrecedingAxis, PrecedingSiblingAxis,
			SelfAxis
		};

		static QString axisAsString( AxisType axis );

		Step();
		Step( AxisType axis,
		      const DOM::DOMString &nodeTest,
		      const QList<Predicate *> &predicates = QList<Predicate *>() );
		~Step();

		DomNodeList evaluate( DOM::NodeImpl *context ) const;

		void optimize();
		QString dump() const;

	private:
		DomNodeList nodesInAxis( DOM::NodeImpl *context ) const;
		DomNodeList nodeTestMatches( DOM::NodeImpl* ctx, const DomNodeList &nodes ) const;
		DOM::DOMString namespaceFromNodetest( const DOM::DOMString &nodeTest ) const;
		unsigned int primaryNodeType( AxisType axis ) const;

		// Original axis + nodetest specification
		AxisType m_axis;
		DOM::DOMString m_nodeTest;

		enum CompileState {
			NotCompiled,
			CompiledForHTML,
			CompiledForXML
		};

		mutable CompileState m_compileState;

		void compileNodeTest(bool htmlCompat) const;

		// Compiled nodetest information. We do this jit'ish due to the
		// case sensitivity mess.
		mutable enum {
			NT_Star,        // *
			NT_LocalName,   // NCName
			NT_Namespace,   // NCName:*
			NT_QName,       // Prefix:LocalName
			NT_Comment,     // 'comment'
			NT_Text,        // 'text'
			NT_PI,          // 'processing-instruction'
			NT_AnyNode,     // 'node'
			NT_PI_Lit       // 'processing-instruction' '(' Literal ')'
		} m_nodeTestType;
		mutable DOM::LocalName     m_localName;
		mutable DOM::NamespaceName m_namespace;
		mutable DOM::DOMString m_piInfo;

		QList<Predicate *> m_predicates;
};

} // namespace XPath

} // namespace khtml


#endif // STEP_H

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
