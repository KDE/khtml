/*
 * parsedstatement.cc - Copyright 2005 Frerich Raabe <raabe@kde.org>
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
#include "parsedstatement.h"
#include "path.h"

#include "xml/dom_nodeimpl.h"
#include "xml/dom_nodelistimpl.h"
#include "xml/dom3_xpathimpl.h"

using namespace DOM;

namespace khtml {
namespace XPath {

Expression *khtmlParseXPathStatement( const DOMString &statement, int& ec );

ParsedStatement::ParsedStatement( const DOMString &statement, khtml::XPathNSResolverImpl* res )
	: m_res( res ), m_expr( 0 ), m_ec( 0 ) 
{
	parse( statement );
}

ParsedStatement::~ParsedStatement()
{
	delete m_expr;
}

void ParsedStatement::parse( const DOMString &statement )
{
	// qDebug() << "parsing:" << statement.string();
	m_ec = 0;
	delete m_expr;
	Expression::evaluationContext().reset( 0, m_res.get() );
	
	m_expr = khtmlParseXPathStatement( statement, m_ec );
	
	// qDebug() << "AST:" << (m_expr ? m_expr->dump() : QString::fromLatin1("*** parse error ***"));
}

void ParsedStatement::optimize()
{
	if ( !m_expr ) {
		return;
	}
	m_expr->optimize();
}

Value ParsedStatement::evaluate(NodeImpl* context, int& ec) const
{
	Expression::evaluationContext().reset(context, m_res.get());
	Value res = m_expr->evaluate();
	ec = Expression::evaluationContext().exceptionCode;

	// If the result is a nodeset, we need to put it in document order
	// and remove duplicates.
	if ( res.isNodeset() )
        res.toNodeset()->normalizeUpto(StaticNodeListImpl::DocumentOrder);
    return res;
}

QString ParsedStatement::dump() const
{
	return m_expr->dump();
}

} // namespace XPath
} // namespace khtml

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
