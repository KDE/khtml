/*
 * parsedstatement.h - Copyright 2005 Frerich Raabe <raabe@kde.org>
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
#ifndef PARSEDSTATEMENT_H
#define PARSEDSTATEMENT_H

#include "util.h"

namespace DOM {
	class NodeImpl;
	class DOMString;
}

namespace khtml {

class XPathNSResolverImpl;

namespace XPath {

class Expression;
class Value;

class ParsedStatement
{
	public:
		ParsedStatement( const DOM::DOMString &statement, khtml::XPathNSResolverImpl* res );
		~ParsedStatement();
		void optimize();

		QString dump() const;

		Value evaluate( DOM::NodeImpl *context,
		                int& ec) const;

		// Any exception issued during parsing, or 0
		int exceptionCode() { return m_ec; }

	private:
		void parse( const DOM::DOMString &statement );

		SharedPtr<khtml::XPathNSResolverImpl> m_res;
		Expression *m_expr;
		int m_ec;
};

} // namespace XPath

} // namespace khtml


#endif // PARSEDSTATEMENT_H
// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;

