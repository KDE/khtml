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
#include "kjs_xpath.h"
#include "kjs_dom.h"

#include "dom/dom3_xpath.h"
#include "xml/dom3_xpathimpl.h"

using KJS::XPathResult;

#include "kjs_xpath.lut.h"

namespace KJS {

// -------------------------------------------------------------------------
/* 
@begin XPathResultConstantsTable  13
  ANY_TYPE                      DOM::XPath::ANY_TYPE      DontDelete|ReadOnly
  NUMBER_TYPE                   DOM::XPath::NUMBER_TYPE   DontDelete|ReadOnly
  STRING_TYPE                   DOM::XPath::STRING_TYPE   DontDelete|ReadOnly
  BOOLEAN_TYPE                  DOM::XPath::BOOLEAN_TYPE  DontDelete|ReadOnly
  UNORDERED_NODE_ITERATOR_TYPE  DOM::XPath::UNORDERED_NODE_ITERATOR_TYPE DontDelete|ReadOnly
  ORDERED_NODE_ITERATOR_TYPE    DOM::XPath::ORDERED_NODE_ITERATOR_TYPE   DontDelete|ReadOnly
  UNORDERED_NODE_SNAPSHOT_TYPE  DOM::XPath::UNORDERED_NODE_SNAPSHOT_TYPE DontDelete|ReadOnly
  ORDERED_NODE_SNAPSHOT_TYPE    DOM::XPath::ORDERED_NODE_SNAPSHOT_TYPE   DontDelete|ReadOnly
  ANY_UNORDERED_NODE_TYPE       DOM::XPath::ANY_UNORDERED_NODE_TYPE      DontDelete|ReadOnly
  FIRST_ORDERED_NODE_TYPE       DOM::XPath::FIRST_ORDERED_NODE_TYPE      DontDelete|ReadOnly
@end
*/
DEFINE_CONSTANT_TABLE(XPathResultConstants)
IMPLEMENT_CONSTANT_TABLE(XPathResultConstants,"XPathResultConstants")
// -------------------------------------------------------------------------
/*
@begin XPathResultProtoTable 3
  iterateNext   XPathResult::IterateNext   DontDelete|Function 0
  snapshotItem  XPathResult::SnapshotItem  DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE   (XPathResultProto)
KJS_IMPLEMENT_PROTOFUNC(XPathResultProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("XPathResult", XPathResultProto, XPathResultProtoFunc, XPathResultConstants)
IMPLEMENT_PSEUDO_CONSTRUCTOR_WITH_PARENT(XPathResultPseudoCtor, "XPathResult", XPathResultProto, XPathResultConstants)

/*
@begin XPathResultTable 11
    resultType   XPathResult::ResultType  DontDelete|ReadOnly
    numberValue  XPathResult::NumberValue DontDelete|ReadOnly
    stringValue  XPathResult::StringValue DontDelete|ReadOnly
    booleanValue XPathResult::BooleanValue DontDelete|ReadOnly
    singleNodeValue XPathResult::SingleNodeValue DontDelete|ReadOnly
    invalidIteratorState XPathResult::InvalidIteratorState DontDelete|ReadOnly
    snapshotLength  XPathResult::SnapshotLength DontDelete|ReadOnly
@end
*/
const ClassInfo XPathResult::info = { "XPathResult", 0, &XPathResultTable, 0 };

XPathResult::XPathResult(ExecState* exec, khtml::XPathResultImpl* impl):
    WrapperBase(XPathResultProto::self(exec), impl)
{}

JSValue* XPathResultProtoFunc::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    KJS_CHECK_THIS(XPathResult, thisObj);

    khtml::XPathResultImpl* imp = static_cast<XPathResult*>(thisObj)->impl();
    DOMExceptionTranslator exception(exec);

    switch (id) {
    case XPathResult::IterateNext:
        return getDOMNode(exec, imp->iterateNext(exception));
    case XPathResult::SnapshotItem:
        return getDOMNode(exec, imp->snapshotItem(args[0]->toInt32(exec), exception));
    }

    return jsUndefined();
}

bool XPathResult::getOwnPropertySlot(ExecState* exec, const Identifier& p, PropertySlot& slot)
{
    return getStaticValueSlot<XPathResult, DOMObject>(exec, &XPathResultTable,
                                                      this, p, slot);
}

JSValue* XPathResult::getValueProperty(ExecState* exec, int token) const
{
    DOMExceptionTranslator exception(exec);
    switch (token) {
    case ResultType:
        return jsNumber(impl()->resultType());
    case NumberValue:
        return jsNumber(impl()->numberValue(exception));
    case StringValue:
        return jsString(impl()->stringValue(exception));
    case BooleanValue:
        return jsBoolean(impl()->booleanValue(exception));
    case SingleNodeValue:
        return getDOMNode(exec, impl()->singleNodeValue(exception));
    case InvalidIteratorState:
        return jsBoolean(impl()->invalidIteratorState());
    case SnapshotLength:
        return jsNumber(impl()->snapshotLength(exception));
    default:
        assert(0);
        return jsUndefined();
    }    
}

// -------------------------------------------------------------------------
/*
@begin XPathExpressionProtoTable 3
  evaluate   XPathExpression::Evaluate   DontDelete|Function 2
@end
*/
KJS_DEFINE_PROTOTYPE   (XPathExpressionProto)
KJS_IMPLEMENT_PROTOFUNC(XPathExpressionProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("XPathExpression", XPathExpressionProto, XPathExpressionProtoFunc, ObjectPrototype)
IMPLEMENT_PSEUDO_CONSTRUCTOR(XPathExpressionPseudoCtor, "XPathExpression", XPathExpressionProto)

const ClassInfo XPathExpression::info = { "XPathExpression", 0, 0, 0 };

XPathExpression::XPathExpression(ExecState* exec, khtml::XPathExpressionImpl* impl):
    WrapperBase(XPathExpressionProto::self(exec), impl), jsResolver(0)
{}

void XPathExpression::mark()
{
    DOMObject::mark();

    if (jsResolver && !jsResolver->marked())
        jsResolver->mark();
}

JSValue* XPathExpressionProtoFunc::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    KJS_CHECK_THIS(XPathExpression, thisObj);

    khtml::XPathExpressionImpl* imp = static_cast<XPathExpression*>(thisObj)->impl();
    DOMExceptionTranslator exception(exec);

    switch (id) {
    case XPathExpression::Evaluate:
        return getWrapper<XPathResult>(exec, imp->evaluate(toNode(args[0]),
                                                          args[1]->toInt32(exec),
                                                          0,
                                                          exception));
    }

    return jsUndefined();
}

// -------------------------------------------------------------------------
/*
@begin XPathNSResolverProtoTable 3
  lookupNamespaceURI   XPathNSResolver::LookupNamespaceURI   DontDelete|Function 1
@end
*/
KJS_DEFINE_PROTOTYPE   (XPathNSResolverProto)
KJS_IMPLEMENT_PROTOFUNC(XPathNSResolverProtoFunc)
KJS_IMPLEMENT_PROTOTYPE("XPathNSResolver", XPathNSResolverProto, XPathNSResolverProtoFunc, ObjectPrototype)
IMPLEMENT_PSEUDO_CONSTRUCTOR(XPathNSResolverPseudoCtor, "XPathNSResolver", XPathNSResolverProto)

const ClassInfo XPathNSResolver::info = { "XPathNSResolver", 0, 0, 0 };

XPathNSResolver::XPathNSResolver(ExecState* exec, khtml::XPathNSResolverImpl* impl):
    WrapperBase(XPathNSResolverProto::self(exec), impl)
{}

JSValue* XPathNSResolverProtoFunc::callAsFunction(ExecState* exec, JSObject* thisObj, const List& args)
{
    KJS_CHECK_THIS(XPathNSResolver, thisObj);

    khtml::XPathNSResolverImpl* imp = static_cast<XPathNSResolver*>(thisObj)->impl();
    DOMExceptionTranslator exception(exec);

    switch (id) {
    case XPathNSResolver::LookupNamespaceURI:
        return jsString(imp->lookupNamespaceURI(args[0]->toString(exec).qstring()));
    }

    return jsUndefined();
}


// -------------------------------------------------------------------------

JSXPathNSResolver::JSXPathNSResolver(Interpreter* ctx, JSObject* impl): impl(impl), ctx(ctx)
{}

khtml::XPathNSResolverImpl::Type JSXPathNSResolver::type()
{
    return XPathNSResolverImpl::JS;
}

DOM::DOMString JSXPathNSResolver::lookupNamespaceURI( const DOM::DOMString& prefix )
{
    // ### this is "heavily inspired" by JSNodeFilter::acceptNode ---
    // gotta be a way of reducing the dupe. This one doesn't
    // propagate exceptions, however --- should it?
    ExecState* exec = ctx->globalExec();

    JSObject* function = 0;
    if (impl->implementsCall()) {
        function = impl;
    } else {
        // See if we have an object with a lookupNamespaceURI method.
        JSObject* cand = impl->get(exec, "lookupNamespaceURI")->getObject();
        if (cand && cand->implementsCall())
            function = cand;
    }

    if (function) {
        List args;
        args.append(jsString(prefix));

        JSValue* result = function->call(exec, impl, args);
        if (!exec->hadException()) {
            if (result->isUndefinedOrNull())
                return DOMString();
            else
                return result->toString(exec).domString();
        } else {
            exec->clearException();
        }
    }

    return DOM::DOMString();
}

// Convert JS -> DOM. Might make a new JSXPathNSResolver. It does not
// protect the JS resolver from collection in any way.
khtml::XPathNSResolverImpl* toResolver(ExecState* exec, JSValue* impl)
{
    JSObject* o = impl->getObject();
    if (!o)
        return 0;

    // Wrapped native object -> unwrap
    if (o->inherits(&XPathNSResolver::info)) {
        return static_cast<XPathNSResolver*>(o)->impl();
    }

    // A JS object -> wrap it.
    return new JSXPathNSResolver(exec->dynamicInterpreter(), o);
}

} // namespace KJS
 
// kate: indent-width 4; replace-tabs on; tab-width 4; space-indent on;
