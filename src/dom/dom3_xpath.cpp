/*
 * XPathExceptionImpl.cpp - Copyright 2005 Frerich Raabe <raabe@kde.org>
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
#include "dom3_xpath.h"

using namespace DOM;

DOMString XPathException::codeAsString(int code) {
	switch ( code ) {
		case INVALID_EXPRESSION_ERR:
			return DOMString( "INVALID_EXPRESSION_ERR" );
		case TYPE_ERR:
			return DOMString( "TYPE_ERR" );
	}
	return DOMString( "(unknown exception code)" );
}

DOMString XPathException::codeAsString() const
{
	return codeAsString(code);
}

int XPathException::toCode( int xpathCode )
{
	return xpathCode + _EXCEPTION_OFFSET;
}

bool XPathException::isXPathExceptionCode(int exceptioncode)
{
	return exceptioncode >= _EXCEPTION_OFFSET && exceptioncode < _EXCEPTION_MAX;
}

// kate: indent-width 4; replace-tabs off; tab-width 4; space-indent off;
