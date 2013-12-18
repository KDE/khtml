/*
 * dom3_xpath.h - Copyright 2005 Frerich Raabe <raabe@kde.org>
 *                Copyright 2010 Maksim Orlovich <maksim@kde.org>
 *                Copyright 1999 Lars Knoll (knoll@kde.org)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 * Based on kdomxpath.h, which was licensed as follows:
 *   Redistribution and use in source and binary forms, with or without
 *   modification, are permitted provided that the following conditions
 *   are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 *   IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 *   OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *   IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 *   INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *   NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *   THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
#ifndef DOM3_XPATH_H
#define DOM3_XPATH_H

#include <dom_string.h>

namespace DOM {

enum XPathExceptionCode {
    INVALID_EXPRESSION_ERR = 51,
    TYPE_ERR = 52
};

namespace XPath {
    enum XPathResultType {
        ANY_TYPE = 0,
        NUMBER_TYPE = 1,
        STRING_TYPE = 2,
        BOOLEAN_TYPE = 3,
        UNORDERED_NODE_ITERATOR_TYPE = 4,
        ORDERED_NODE_ITERATOR_TYPE = 5,
        UNORDERED_NODE_SNAPSHOT_TYPE = 6,
        ORDERED_NODE_SNAPSHOT_TYPE = 7,
        ANY_UNORDERED_NODE_TYPE = 8,
        FIRST_ORDERED_NODE_TYPE = 9
    };
}

class KHTML_EXPORT XPathException
{
public:
    XPathException(unsigned short _code) { code = _code; }
    XPathException(const XPathException &other) { code = other.code; }

    XPathException & operator = (const XPathException &other)
        { code = other.code; return *this; }

    virtual ~XPathException() {}

    /**
     * An integer indicating the type of error generated, as given by DOM L3 XPath
     *
     */
    unsigned short   code;

    enum XPathExceptionCode
    {
        INVALID_EXPRESSION_ERR = 51,
        TYPE_ERR = 52,

        _EXCEPTION_OFFSET = 4000,
        _EXCEPTION_MAX = 4999
    };

    DOMString codeAsString() const;
    static DOMString codeAsString(int xpathCode);

    /** @internal - converts XPath exception code to internal code */
    static int toCode(int xpathCode);

    /** @internal - checks to see whether internal code is an XPath one */
    static bool isXPathExceptionCode(int exceptioncode);
};


}

#endif // DOM3_XPATH_H

