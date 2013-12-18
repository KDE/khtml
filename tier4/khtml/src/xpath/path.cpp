/*
 * path.cc - Copyright 2005 Frerich Raabe <raabe@kde.org>
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
#include "path.h"

#include "xml/dom_docimpl.h"
#include "xml/dom_nodeimpl.h"

using namespace DOM;
using namespace khtml;
using namespace khtml::XPath;

Filter::Filter( Expression *expr, const QList<Predicate *> &predicates )
	: m_expr( expr ),
	m_predicates( predicates )
{
}

Filter::~Filter()
{
	delete m_expr;
	qDeleteAll( m_predicates );
}

QString Filter::dump() const
{
	QString s = "<filter>";
	s += m_expr->dump();
	foreach( Predicate *predicate, m_predicates ) {
		s += predicate->dump();
	}
	s += "</filter>";
	return s;
}

Value Filter::doEvaluate() const
{
	Value v = m_expr->evaluate();
	if ( !v.isNodeset() ) {
		if ( !m_predicates.empty() ) {
			// qDebug() << "Ignoring predicates for filter since expression does not evaluate to a nodeset!";
		}
		return v;
	}

	DomNodeList inNodes = v.toNodeset(), outNodes;

	// Filter seems to work in document order, not axis order
	inNodes->normalizeUpto(StaticNodeListImpl::DocumentOrder);

	foreach( Predicate *predicate, m_predicates ) {
		outNodes = new StaticNodeListImpl();
		Expression::evaluationContext().size = int(inNodes->length());

		for ( unsigned long n = 0; n < inNodes->length(); ++n ) {
			NodeImpl *node = inNodes->item(n);
			Expression::evaluationContext().node = node;
			Expression::evaluationContext().position = n + 1;

			if ( predicate->evaluate() ) {
				outNodes->append( node );
			}
		}

		inNodes = outNodes;
		outNodes->setKnownNormalization(StaticNodeListImpl::DocumentOrder);

#ifdef XPATH_VERBOSE
		qDebug() << "Predicate within filter trims to:" << outNodes->length();
#endif
	}

	return Value( outNodes );
}

LocationPath::LocationPath()
	: m_absolute( false )
{
}

LocationPath::~LocationPath()
{
	qDeleteAll( m_steps );
}

void LocationPath::optimize()
{
	foreach( Step *step, m_steps ) {
		step->optimize();
	}
}

Value LocationPath::doEvaluate() const
{
#ifdef XPATH_VERBOSE
	if ( m_absolute ) {
		qDebug() << "Evaluating absolute path expression, steps:" << m_steps.count();
	} else {
		qDebug() << "Evaluating relative path expression, steps:" << m_steps.count();
	}
#endif

	DomNodeList inDomNodes  = new StaticNodeListImpl,
	            outDomNodes;

	/* For absolute location paths, the context node is ignored - the
	 * document's root node is used instead.
	 */
	NodeImpl *context = Expression::evaluationContext().node;
	if ( m_absolute ) {
		if ( context->nodeType() != Node::DOCUMENT_NODE ) {
			context = context->ownerDocument();
		}
	}

	inDomNodes->append( context );

	if ( m_steps.isEmpty() )
		return Value( inDomNodes );

	int s = 0;
	foreach( Step *step, m_steps ) {
#ifdef XPATH_VERBOSE
		qDebug() << "-------------------------------------";
		qDebug() << "Step " << s << "insize " << inDomNodes->length();
#endif		

		outDomNodes = new StaticNodeListImpl;
		for ( unsigned long i = 0; i < inDomNodes->length(); ++i ) {
			DomNodeList matches = step->evaluate( inDomNodes->item( i ) );
			for ( unsigned long j = 0; j < matches->length(); ++j )
				outDomNodes->append( matches->item( j ) );
		}
		inDomNodes = outDomNodes;

		++s;
	}

#ifdef XPATH_VERBOSE
	qDebug() << "-------------------------------------";
	qDebug() << "output:" <<outDomNodes->length();
	qDebug() << "=====================================";
#endif

	return Value( outDomNodes );
}

QString LocationPath::dump() const
{
	QString s = "<locationpath absolute=\"";
	s += m_absolute ? "true" : "false";
	s += "\">";
	foreach( Step *step, m_steps ) {
		s += step->dump();
	}
	s += "</locationpath>";
	return s;
}

Path::Path( Filter *filter, LocationPath *path )
	: m_filter( filter ),
	m_path( path )
{
}

Path::~Path()
{
	delete m_filter;
	delete m_path;
}

QString Path::dump() const
{
	if ( !m_filter && !m_path ) {
		return "<path/>";
	}

	QString s = "<path>";
	if ( m_filter ) {
		s += m_filter->dump();
	}
	if ( m_path ) {
		s += m_path->dump();
	}
	s += "</path>";

	return s;
}

Value Path::doEvaluate() const
{
	NodeImpl* saveCtx = Expression::evaluationContext().node;

	Value initial =  m_filter->evaluate();
	if ( initial.isNodeset() ) {
		// Pass in every output from the filter to the path, and union the results
		DomNodeList out = new StaticNodeListImpl();
		DomNodeList in  = initial.toNodeset();

		for (unsigned long i = 0; i < in->length(); ++i) {
			Expression::evaluationContext().node = in->item(i);

			DomNodeList singleSet = m_path->evaluate().toNodeset();
			for (unsigned long j = 0; j < singleSet->length(); ++ j)
				out->append(singleSet->item(j));
		}

		Expression::evaluationContext().node = saveCtx;
		return Value(out);
	} else {
		// ### what should happen in this case?
		Expression::reportInvalidExpressionErr();
		return Value();
	}
}

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
