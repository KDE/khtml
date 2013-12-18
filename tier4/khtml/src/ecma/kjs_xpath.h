/*
 *  This file is part of the KDE libraries
 *  Copyright (C) 2010 Maksim Orlovich <maksim@kde.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "kjs_binding.h"
#include "xml/dom3_xpathimpl.h"

namespace KJS {

DEFINE_PSEUDO_CONSTRUCTOR(XPathResultPseudoCtor)

class XPathResult: public DOMWrapperObject<khtml::XPathResultImpl>
{
public:
    XPathResult(ExecState* exec, khtml::XPathResultImpl* impl);

    virtual bool getOwnPropertySlot(ExecState *, const Identifier&, PropertySlot&);
    using JSObject::getOwnPropertySlot;
    JSValue *getValueProperty(ExecState *exec, int token) const;

    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    // The various constants are in separate constant table node
    enum {
        // properties:
        ResultType, NumberValue, StringValue, BooleanValue,
        SingleNodeValue, InvalidIteratorState, SnapshotLength,
        // functions:
        IterateNext, SnapshotItem
    };
};

DEFINE_PSEUDO_CONSTRUCTOR(XPathExpressionPseudoCtor)

class XPathExpression: public DOMWrapperObject<khtml::XPathExpressionImpl>
{
public:
    XPathExpression(ExecState* exec, khtml::XPathExpressionImpl* impl);
    
    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    enum {
        // functions:
        Evaluate
    };

    virtual void mark();

    void setAssociatedResolver(JSObject* res) { jsResolver = res; }
private:
    JSObject* jsResolver; // see notes below.
};

// For NS resolver, we need to do two things:
// 1) wrap native NS resolvers, such as those returned by
//    Document::createNSResolver
//
// 2) Pass in JS-implemented resolvers to DOM methods, which might retain them.
// This is a bit tricky memory management-wise, as it involves the DOM
// referring to a JS object. The solution we take is to have the wrapper
// for XPathExpression mark the corresponding resolver. 
//
// The class XPathNSResolver   does (1)
// The class JSXPathNSResolver does (2).
//
// Further, the method ... to avoid having wrappers-inside-wrappers
DEFINE_PSEUDO_CONSTRUCTOR(XPathNSResolverPseudoCtor)
class XPathNSResolver: public DOMWrapperObject<khtml::XPathNSResolverImpl>
{
public:
    XPathNSResolver(ExecState* exec, khtml::XPathNSResolverImpl* impl);

    virtual const ClassInfo* classInfo() const { return &info; }
    static const ClassInfo info;

    enum {
        // functions:
        LookupNamespaceURI
    };
};

class JSXPathNSResolver: public khtml::XPathNSResolverImpl
{
public:
    JSXPathNSResolver(Interpreter* ctx, JSObject* impl);
    virtual Type type();
    DOM::DOMString lookupNamespaceURI( const DOM::DOMString& prefix );

    JSObject* resolverObject() { return impl; }
private:
    JSObject* impl;
    Interpreter* ctx;
};

// Convert JS -> DOM. Might make a new JSXPathNSResolver. It does not
// protect the JS resolver from collection in any way.
khtml::XPathNSResolverImpl* toResolver(ExecState* exec, JSValue* impl);


}

// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
